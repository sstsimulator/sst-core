// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/sync/rankSyncParallelSkip.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/profile.h"
#include "sst/core/serialization/serializer.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_mpi.h"
#include "sst/core/sync/syncQueue.h"
#include "sst/core/timeConverter.h"

#if SST_EVENT_PROFILING
#define SST_EVENT_PROFILE_START auto event_profile_start = std::chrono::high_resolution_clock::now();

#define SST_EVENT_PROFILE_STOP                                                                                  \
    auto event_profile_stop = std::chrono::high_resolution_clock::now();                                        \
    auto event_profile_count =                                                                                  \
        std::chrono::duration_cast<std::chrono::nanoseconds>(event_profile_stop - event_profile_start).count(); \
    sim->incrementSerialCounters(event_profile_count);
#else
#define SST_EVENT_PROFILE_START
#define SST_EVENT_PROFILE_STOP
#endif

namespace SST {

// Static Data Members
SimTime_t RankSyncParallelSkip::myNextSyncTime = 0;

///// RankSyncParallelSkip class /////

RankSyncParallelSkip::RankSyncParallelSkip(RankInfo num_ranks) :
    RankSync(num_ranks),
    mpiWaitTime(0.0),
    deserializeTime(0.0),
    send_count(0),
    serializeReadyBarrier(num_ranks.thread),
    slaveExchangeDoneBarrier(num_ranks.thread),
    allDoneBarrier(num_ranks.thread)
{
    max_period     = Simulation_impl::getSimulation()->getMinPartTC();
    myNextSyncTime = max_period.getFactor();
    recv_count     = new int[num_ranks_.thread];
    for ( uint32_t i = 0; i < num_ranks_.thread; i++ ) {
        recv_count[i] = 0;
    }
    link_send_queue = new SST::Core::ThreadSafe::UnboundedQueue<comm_recv_pair*>[num_ranks_.thread];
}

RankSyncParallelSkip::~RankSyncParallelSkip()
{
    for ( auto i = comm_send_map.begin(); i != comm_send_map.end(); ++i ) {
        delete i->second.squeue;
    }
    comm_send_map.clear();

    for ( auto i = comm_recv_map.begin(); i != comm_recv_map.end(); ++i ) {
        delete[] i->second.rbuf;
    }
    comm_recv_map.clear();

    delete[] recv_count;
    delete[] link_send_queue;

    if ( mpiWaitTime > 0.0 || deserializeTime > 0.0 )
        Output::getDefaultObject().verbose(
            CALL_INFO, 1, 0, "RankSyncParallelSkip mpiWait: %lg sec  deserializeWait:  %lg sec\n", mpiWaitTime,
            deserializeTime);
}

ActivityQueue*
RankSyncParallelSkip::registerLink(
    const RankInfo& to_rank, const RankInfo& from_rank, const std::string& name, Link* link)
{
    std::lock_guard<Core::ThreadSafe::Spinlock> slock(lock);

    // For sends, we track the remote rank and thread ID
    RankSyncQueue* queue;
    if ( comm_send_map.count(to_rank) == 0 ) {
        send_count++;
        comm_send_map[to_rank].to_rank = to_rank;
        queue = comm_send_map[to_rank].squeue = new RankSyncQueue(to_rank);
        comm_send_map[to_rank].remote_size    = 4096;
    }
    else {
        queue = comm_send_map[to_rank].squeue;
    }

    // For recv's, we track the remote rank and the local thread.
    // Need to create a RankInfo object with this data to use in the
    // map.
    RankInfo remote_rank_local_thread(to_rank.rank, from_rank.thread);
    if ( comm_recv_map.count(remote_rank_local_thread) == 0 ) {
        recv_count[from_rank.thread]++;
        comm_recv_map[remote_rank_local_thread].remote_rank  = to_rank.rank;
        comm_recv_map[remote_rank_local_thread].local_thread = from_rank.thread;
        comm_recv_map[remote_rank_local_thread].rbuf         = new char[4096];
        comm_recv_map[remote_rank_local_thread].local_size   = 4096;
    }

    link_maps[to_rank.rank][name] = reinterpret_cast<uintptr_t>(link);
#ifdef __SST_DEBUG_EVENT_TRACKING__
    link->setSendingComponentInfo("SYNC", "SYNC", "");
#endif
    return queue;
}

void
RankSyncParallelSkip::setRestartTime(SimTime_t time)
{
    if ( Simulation_impl::getSimulation()->getRank().thread == 0 ) { myNextSyncTime = time; }
}

void
RankSyncParallelSkip::finalizeLinkConfigurations()
{
    // Set the size of the BoundedQueue that is the work queue for
    // serializations
    deserialize_queue.initialize(comm_recv_map.size());
    serialize_queue.initialize(comm_send_map.size());
    send_queue.initialize(comm_send_map.size());
}

void
RankSyncParallelSkip::prepareForComplete()
{}

void
RankSyncParallelSkip::setSignals(int end, int usr, int alrm)
{
    sig_end_  = end;
    sig_usr_  = usr;
    sig_alrm_ = alrm;
}

bool
RankSyncParallelSkip::getSignals(int& end, int& usr, int& alrm)
{
    end  = sig_end_;
    usr  = sig_usr_;
    alrm = sig_alrm_;
    return sig_end_ || sig_usr_ || sig_alrm_;
}

uint64_t
RankSyncParallelSkip::getDataSize() const
{
    size_t count = 0;
    for ( auto it = comm_send_map.begin(); it != comm_send_map.end(); ++it ) {
        count += it->second.squeue->getDataSize();
    }

    for ( auto it = comm_recv_map.begin(); it != comm_recv_map.end(); ++it ) {
        count += it->second.local_size;
    }
    return count;
}

void
RankSyncParallelSkip::execute(int thread)
{
    if ( thread == 0 ) {
        exchange_master(thread);
        allDoneBarrier.wait(); /* Sync up with slave finish below */
    }
    else {
        serializeReadyBarrier.wait(); /* Wait for exchange_master() to start up */
        exchange_slave(thread);       /* Waits at the end */
        allDoneBarrier.wait();        /* Wait for exchange_master to finish */
    }
}

void
RankSyncParallelSkip::exchange_slave(int thread)
{

    // Check the serialize_queue for work.
    comm_send_pair* ser;

    Simulation_impl* sim = Simulation_impl::getSimulation();

    while ( serialize_queue.try_remove(ser) ) {
        // Measures serialization time
        SST_EVENT_PROFILE_START

        // Serialize the events
        ser->sbuf = ser->squeue->getData();

        SST_EVENT_PROFILE_STOP

        // Send back to master to do MPI send
        send_queue.try_insert(ser);
    }

    // After serialization is done, start processing the receives.

    int my_recv_count = recv_count[thread];

    // Do nothing until there are events to be sent on this thread's
    // links
    SimTime_t current_cycle = sim->getCurrentSimCycle();

    // Two things left to do.  Deserialize receives and send
    // deserialized events on the proper link.  Will preferentially
    // send first.
    comm_recv_pair* recv;
    while ( my_recv_count != 0 || remaining_deser.load() != 0 ) {
        // Check to see if there are sends to be done.
        if ( link_send_queue[thread].try_remove(recv) ) {

            // comm_recv_pair* recv = link_send_queue[thread].remove();
            my_recv_count--;

            for ( size_t i = 0; i < recv->activity_vec.size(); i++ ) {
                Event*    ev    = static_cast<Event*>(recv->activity_vec[i]);
                SimTime_t delay = ev->getDeliveryTime() - current_cycle;
                getDeliveryLink(ev)->send(delay, ev);
            }
            recv->activity_vec.clear();
        }
        else if ( deserialize_queue.try_remove(recv) ) {
            remaining_deser--;
            deserializeMessage(recv);
            link_send_queue[recv->local_thread].insert(recv);
        }
    }
    slaveExchangeDoneBarrier.wait();
}

void
RankSyncParallelSkip::exchange_master(int UNUSED(thread))
{
#ifdef SST_CONFIG_HAVE_MPI

    // Maximum number of outstanding requests is 3 times the number
    // of ranks I communicate with (1 recv, 2 sends per rank)
    auto sreqs      = std::make_unique<MPI_Request[]>(2 * comm_send_map.size());
    int  sreq_count = 0;
    // First thing to do is fill the serialize_queue.
    for ( auto i = comm_send_map.begin(); i != comm_send_map.end(); ++i ) {
        serialize_queue.try_insert(&(i->second));
    }

    remaining_deser = comm_recv_map.size();

    serializeReadyBarrier.wait(); /* Wait for / release slaves to serialize */

    for ( auto i = comm_recv_map.begin(); i != comm_recv_map.end(); ++i ) {
        // Post all the receives
        int tag             = 2 * i->second.local_thread;
        i->second.recv_done = false;
        MPI_Irecv(
            i->second.rbuf, i->second.local_size, MPI_BYTE, i->second.remote_rank, tag, MPI_COMM_WORLD, &i->second.req);
    }

    // Do all the sends, but if there are no sends to do, then help
    // with serialization
    int             my_send_count = send_count;
    comm_send_pair* send;

#if SST_EVENT_PROFILING
    Simulation_impl* sim = Simulation_impl::getSimulation();
#endif

    while ( my_send_count != 0 ) {
        if ( send_queue.try_remove(send) ) {
            my_send_count--;

            char*                  send_buffer = send->sbuf;
            // Cast to Header so we can get/fill in data
            RankSyncQueue::Header* hdr         = reinterpret_cast<RankSyncQueue::Header*>(send_buffer);
            int                    tag         = 2 * send->to_rank.thread;
            // Check to see if remote queue is big enough for data
            if ( send->remote_size < hdr->buffer_size ) {
                // not big enough, send message that will tell remote side to get larger buffer
                hdr->mode = 1;
                MPI_Isend(
                    send_buffer, sizeof(RankSyncQueue::Header), MPI_BYTE, send->to_rank.rank /*dest*/, tag,
                    MPI_COMM_WORLD, &sreqs[sreq_count++]);
                send->remote_size = hdr->buffer_size;
                tag               = 2 * send->to_rank.thread + 1;
            }
            else {
                hdr->mode = 0;
            }
            MPI_Isend(
                send_buffer, hdr->buffer_size, MPI_BYTE, send->to_rank.rank /*dest*/, tag, MPI_COMM_WORLD,
                &sreqs[sreq_count++]);
        }
        else if ( serialize_queue.try_remove(send) ) {
            // Serialize the events
            SST_EVENT_PROFILE_START
            send->sbuf = send->squeue->getData();
            SST_EVENT_PROFILE_STOP

            // Send back to master to do MPI send
            send_queue.try_insert(send);
        }
        else {
            sst_pause();
        }
    }

    // Do all the receives as they arrive
    int receives_to_process = comm_recv_map.size();
    while ( receives_to_process != 0 ) {
        for ( auto i = comm_recv_map.begin(); i != comm_recv_map.end(); ++i ) {
            if ( !i->second.recv_done ) {
                int flag;
                MPI_Test(&i->second.req, &flag, MPI_STATUS_IGNORE);
                if ( flag ) {
                    receives_to_process--;
                    i->second.recv_done = true;

                    // Get the buffer and deserialize all the events
                    char* buffer = i->second.rbuf;

                    RankSyncQueue::Header* hdr  = reinterpret_cast<RankSyncQueue::Header*>(buffer);
                    unsigned int           size = hdr->buffer_size;
                    int                    mode = hdr->mode;

                    if ( mode == 1 ) {
                        // May need to resize the buffer
                        if ( size > i->second.local_size ) {
                            delete[] i->second.rbuf;
                            i->second.rbuf       = new char[size];
                            i->second.local_size = size;
                        }
                        MPI_Recv(
                            i->second.rbuf, i->second.local_size, MPI_BYTE, i->second.remote_rank,
                            2 * i->second.local_thread + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        buffer = i->second.rbuf;
                    }

                    deserialize_queue.try_insert(&(i->second));
                }
            }
        }
    }

    // For now simply call exchange_slave() to deliver events
    exchange_slave(0); /* Barriers at end */

    // Clear the RankSyncQueues used to send the data after all the sends have completed
    // waitStart = SST::Core::Profile::now();
    MPI_Waitall(sreq_count, sreqs.get(), MPI_STATUSES_IGNORE);
    // mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);

    for ( auto i = comm_send_map.begin(); i != comm_send_map.end(); ++i ) {
        i->second.squeue->clear();
    }

    // Check to see when the next event is scheduled, then do an
    // all_reduce with min operator and set next sync time to be
    // min + max_period.

    // Need to get the local minimum, then do a global minimum
    // SimTime_t input = Simulation_impl::getSimulation()->getNextActivityTime();
    SimTime_t input = Simulation_impl::getLocalMinimumNextActivityTime();
    SimTime_t min_time;
    MPI_Allreduce(&input, &min_time, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD);

    myNextSyncTime = min_time + max_period.getFactor();

    /* Exchange signals */
    int32_t local_signals[3]  = { sig_end_, sig_usr_, sig_alrm_ };
    int32_t global_signals[3] = { 0, 0, 0 };
    MPI_Allreduce(&local_signals, &global_signals, 3, MPI_INT32_T, MPI_MAX, MPI_COMM_WORLD);

    sig_end_  = global_signals[0];
    sig_usr_  = global_signals[1];
    sig_alrm_ = global_signals[2];

#endif
}

void
RankSyncParallelSkip::exchangeLinkUntimedData(int UNUSED_WO_MPI(thread), std::atomic<int>& UNUSED_WO_MPI(msg_count))
{
#ifdef SST_CONFIG_HAVE_MPI
    if ( thread != 0 ) { return; }
    // Maximum number of outstanding requests is 3 times the number
    // of ranks I communicate with (1 recv, 2 sends per rank)
    auto sreqs      = std::make_unique<MPI_Request[]>(2 * comm_send_map.size());
    auto rreqs      = std::make_unique<MPI_Request[]>(comm_recv_map.size());
    int  rreq_count = 0;
    int  sreq_count = 0;

    for ( auto i = comm_recv_map.begin(); i != comm_recv_map.end(); ++i ) {
        // Post all the receives
        int tag = 2 * i->second.local_thread;
        MPI_Irecv(
            i->second.rbuf, i->second.local_size, MPI_BYTE, i->second.remote_rank, tag, MPI_COMM_WORLD,
            &rreqs[rreq_count++]);
    }

    for ( auto i = comm_send_map.begin(); i != comm_send_map.end(); ++i ) {

        // Do all the sends
        // Get the buffer from the syncQueue
        char* send_buffer = i->second.squeue->getData();

        // Cast to Header so we can get/fill in data
        RankSyncQueue::Header* hdr = reinterpret_cast<RankSyncQueue::Header*>(send_buffer);
        int                    tag = 2 * i->second.to_rank.thread;
        // Check to see if remote queue is big enough for data
        if ( i->second.remote_size < hdr->buffer_size ) {
            // not big enough, send message that will tell remote side to get larger buffer
            hdr->mode = 1;
            MPI_Isend(
                send_buffer, sizeof(RankSyncQueue::Header), MPI_BYTE, i->second.to_rank.rank /*dest*/, tag,
                MPI_COMM_WORLD, &sreqs[sreq_count++]);
            i->second.remote_size = hdr->buffer_size;
            tag                   = 2 * i->second.to_rank.thread + 1;
        }
        else {
            hdr->mode = 0;
        }
        MPI_Isend(
            send_buffer, hdr->buffer_size, MPI_BYTE, i->second.to_rank.rank /*dest*/, tag, MPI_COMM_WORLD,
            &sreqs[sreq_count++]);
    }

    // Wait for all recvs to complete
    MPI_Waitall(rreq_count, rreqs.get(), MPI_STATUSES_IGNORE);

    for ( auto i = comm_recv_map.begin(); i != comm_recv_map.end(); ++i ) {

        // Get the buffer and deserialize all the events
        char* buffer = i->second.rbuf;

        RankSyncQueue::Header* hdr  = reinterpret_cast<RankSyncQueue::Header*>(buffer);
        unsigned int           size = hdr->buffer_size;
        int                    mode = hdr->mode;

        if ( mode == 1 ) {
            // May need to resize the buffer
            if ( size > i->second.local_size ) {
                delete[] i->second.rbuf;
                i->second.rbuf       = new char[size];
                i->second.local_size = size;
            }
            MPI_Recv(
                i->second.rbuf, i->second.local_size, MPI_BYTE, i->second.remote_rank, 2 * i->second.local_thread + 1,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buffer = i->second.rbuf;
        }

        SST::Core::Serialization::serializer ser;
        ser.start_unpacking(&buffer[sizeof(RankSyncQueue::Header)], size - sizeof(RankSyncQueue::Header));

        std::vector<Activity*> activities;
        SST_SER(activities);

        for ( unsigned int j = 0; j < activities.size(); j++ ) {

            Event* ev = static_cast<Event*>(activities[j]);
            sendUntimedData_sync(getDeliveryLink(ev), ev);
        }
    }

    // Clear the RankSyncQueues used to send the data after all the sends have completed
    MPI_Waitall(sreq_count, sreqs.get(), MPI_STATUSES_IGNORE);

    for ( auto i = comm_send_map.begin(); i != comm_send_map.end(); ++i ) {
        i->second.squeue->clear();
    }

    // Do an allreduce to see if there were any messages sent
    int input = msg_count;

    int count;
    MPI_Allreduce(&input, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    msg_count = count;
#endif
}

void
RankSyncParallelSkip::deserializeMessage(comm_recv_pair* msg)
{
    char*                  buffer = msg->rbuf;
    RankSyncQueue::Header* hdr    = reinterpret_cast<RankSyncQueue::Header*>(buffer);
    unsigned int           size   = hdr->buffer_size;

    auto deserialStart = SST::Core::Profile::now();

    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(&buffer[sizeof(RankSyncQueue::Header)], size - sizeof(RankSyncQueue::Header));
    SST_SER(msg->activity_vec);

    deserializeTime += SST::Core::Profile::getElapsed(deserialStart);
}

int RankSyncParallelSkip::sig_end_(0);
int RankSyncParallelSkip::sig_usr_(0);
int RankSyncParallelSkip::sig_alrm_(0);

} // namespace SST
