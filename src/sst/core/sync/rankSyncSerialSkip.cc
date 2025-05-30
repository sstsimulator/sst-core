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

#include "sst/core/sync/rankSyncSerialSkip.h"

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
SimTime_t RankSyncSerialSkip::myNextSyncTime = 0;

RankSyncSerialSkip::RankSyncSerialSkip(RankInfo num_ranks) :
    RankSync(num_ranks),
    mpiWaitTime(0.0),
    deserializeTime(0.0)
{
    max_period     = Simulation_impl::getSimulation()->getMinPartTC();
    myNextSyncTime = max_period.getFactor();
}

RankSyncSerialSkip::~RankSyncSerialSkip()
{
    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        delete i->second.squeue;
    }
    comm_map.clear();

    if ( mpiWaitTime > 0.0 || deserializeTime > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0,
            "RankSyncSerialSkip mpiWait: %lg sec  deserializeWait:  %lg sec\n", mpiWaitTime, deserializeTime);
}

ActivityQueue*
RankSyncSerialSkip::registerLink(
    const RankInfo& to_rank, const RankInfo& UNUSED(from_rank), const std::string& name, Link* link)
{
    std::lock_guard<Core::ThreadSafe::Spinlock> slock(lock);

    RankSyncQueue* queue;
    if ( comm_map.count(to_rank.rank) == 0 ) {
        queue = comm_map[to_rank.rank].squeue = new RankSyncQueue(to_rank);
        comm_map[to_rank.rank].rbuf           = new char[4096];
        comm_map[to_rank.rank].local_size     = 4096;
        comm_map[to_rank.rank].remote_size    = 4096;
    }
    else {
        queue = comm_map[to_rank.rank].squeue;
    }

    link_maps[to_rank.rank][name] = reinterpret_cast<uintptr_t>(link);
#ifdef __SST_DEBUG_EVENT_TRACKING__
    link->setSendingComponentInfo("SYNC", "SYNC", "");
#endif
    return queue;
}

void
RankSyncSerialSkip::setRestartTime(SimTime_t time)
{
    if ( Simulation_impl::getSimulation()->getRank().thread == 0 ) {
        myNextSyncTime = time;
    }
}

void
RankSyncSerialSkip::finalizeLinkConfigurations()
{}

void
RankSyncSerialSkip::prepareForComplete()
{}

void
RankSyncSerialSkip::setSignals(int end, int usr, int alrm)
{
    sig_end_  = end;
    sig_usr_  = usr;
    sig_alrm_ = alrm;
}

bool
RankSyncSerialSkip::getSignals(int& end, int& usr, int& alrm)
{
    end  = sig_end_;
    usr  = sig_usr_;
    alrm = sig_alrm_;
    return sig_end_ || sig_usr_ || sig_alrm_;
}

uint64_t
RankSyncSerialSkip::getDataSize() const
{
    size_t count = 0;
    for ( comm_map_t::const_iterator it = comm_map.begin(); it != comm_map.end(); ++it ) {
        count += (it->second.squeue->getDataSize() + it->second.local_size);
    }
    return count;
}

void
RankSyncSerialSkip::execute(int thread)
{
    if ( thread == 0 ) {
        exchange();
    }
}

void
RankSyncSerialSkip::exchange()
{
#ifdef SST_CONFIG_HAVE_MPI
    // Maximum number of outstanding requests is 3 times the number
    // of ranks I communicate with (1 recv, 2 sends per rank)
    auto sreqs      = std::make_unique<MPI_Request[]>(2 * comm_map.size());
    auto rreqs      = std::make_unique<MPI_Request[]>(comm_map.size());
    int  sreq_count = 0;
    int  rreq_count = 0;

    Simulation_impl* sim = Simulation_impl::getSimulation();

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {

        SST_EVENT_PROFILE_START

        // Do all the sends
        // Get the buffer from the syncQueue
        char* send_buffer = i->second.squeue->getData();

        SST_EVENT_PROFILE_STOP

        // Cast to Header so we can get/fill in data
        RankSyncQueue::Header* hdr = reinterpret_cast<RankSyncQueue::Header*>(send_buffer);
        // Simulation_impl::getSimulation()->getSimulationOutput().output("Data size = %d\n", hdr->buffer_size);
        int                    tag = 1;
        // Check to see if remote queue is big enough for data
        if ( i->second.remote_size < hdr->buffer_size ) {
            // not big enough, send message that will tell remote side to get larger buffer
            hdr->mode = 1;
            MPI_Isend(send_buffer, sizeof(RankSyncQueue::Header), MPI_BYTE, i->first /*dest*/, tag, MPI_COMM_WORLD,
                &sreqs[sreq_count++]);
            i->second.remote_size = hdr->buffer_size;
            tag                   = 2;
        }
        else {
            hdr->mode = 0;
        }
        MPI_Isend(
            send_buffer, hdr->buffer_size, MPI_BYTE, i->first /*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);

        // Post all the receives
        MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first, 1, MPI_COMM_WORLD, &rreqs[rreq_count++]);
    }

    // Wait for all sends and recvs to complete
    SimTime_t current_cycle = sim->getCurrentSimCycle();

    auto waitStart = SST::Core::Profile::now();
    MPI_Waitall(rreq_count, rreqs.get(), MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
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
            MPI_Recv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buffer = i->second.rbuf;
        }

        auto deserialStart = SST::Core::Profile::now();

        SST::Core::Serialization::serializer ser;
        ser.start_unpacking(&buffer[sizeof(RankSyncQueue::Header)], size - sizeof(RankSyncQueue::Header));

        std::vector<Activity*> activities;
        activities.clear();
        SST_SER(activities);

        deserializeTime += SST::Core::Profile::getElapsed(deserialStart);

        for ( unsigned int j = 0; j < activities.size(); j++ ) {

            Event*    ev    = static_cast<Event*>(activities[j]);
            SimTime_t delay = ev->getDeliveryTime() - current_cycle;
            getDeliveryLink(ev)->send(delay, ev);
        }

        activities.clear();
    }

    // Clear the RankSyncQueues used to send the data after all the sends have completed
    waitStart = SST::Core::Profile::now();
    MPI_Waitall(sreq_count, sreqs.get(), MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        i->second.squeue->clear();
    }

    // If we have an Exit object, fire it to see if we need end simulation
    // if ( exit != nullptr ) exit->check();

    // Check to see when the next event is scheduled, then do an
    // all_reduce with min operator and set next sync time to be
    // min + max_period.

    // Need to get the local minimum, then do a global minimum
    // SimTime_t input = Simulation_impl::getSimulation()->getNextActivityTime();
    SimTime_t input = Simulation_impl::getLocalMinimumNextActivityTime();
    SimTime_t min_time;
    MPI_Allreduce(&input, &min_time, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD);

    myNextSyncTime = min_time + max_period.getFactor();

    int32_t local_signals[3]  = { sig_end_, sig_usr_, sig_alrm_ };
    int32_t global_signals[3] = { 0, 0, 0 };
    MPI_Allreduce(&local_signals, &global_signals, 3, MPI_INT32_T, MPI_MAX, MPI_COMM_WORLD);

    sig_end_  = global_signals[0];
    sig_usr_  = global_signals[1];
    sig_alrm_ = global_signals[2];
#endif
}

void
RankSyncSerialSkip::exchangeLinkUntimedData(int UNUSED_WO_MPI(thread), std::atomic<int>& UNUSED_WO_MPI(msg_count))
{
#ifdef SST_CONFIG_HAVE_MPI
    if ( thread != 0 ) {
        return;
    }
    // Maximum number of outstanding requests is 3 times the number of
    // ranks I communicate with (1 recv, 2 sends per rank)
    auto sreqs      = std::make_unique<MPI_Request[]>(2 * comm_map.size());
    auto rreqs      = std::make_unique<MPI_Request[]>(comm_map.size());
    int  rreq_count = 0;
    int  sreq_count = 0;

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {

        // Do all the sends
        // Get the buffer from the syncQueue
        char*                  send_buffer = i->second.squeue->getData();
        // Cast to Header so we can get/fill in data
        RankSyncQueue::Header* hdr         = reinterpret_cast<RankSyncQueue::Header*>(send_buffer);
        int                    tag         = 1;
        // Check to see if remote queue is big enough for data
        if ( i->second.remote_size < hdr->buffer_size ) {
            // not big enough, send message that will tell remote side to get larger buffer
            hdr->mode = 1;
            MPI_Isend(send_buffer, sizeof(RankSyncQueue::Header), MPI_BYTE, i->first /*dest*/, tag, MPI_COMM_WORLD,
                &sreqs[sreq_count++]);
            i->second.remote_size = hdr->buffer_size;
            tag                   = 2;
        }
        else {
            hdr->mode = 0;
        }
        MPI_Isend(
            send_buffer, hdr->buffer_size, MPI_BYTE, i->first /*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);

        // Post all the receives
        MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first, 1, MPI_COMM_WORLD, &rreqs[rreq_count++]);
    }

    // Wait for all recvs to complete
    MPI_Waitall(rreq_count, rreqs.get(), MPI_STATUSES_IGNORE);

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {

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
            MPI_Recv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        i->second.squeue->clear();
    }

    // Do an allreduce to see if there were any messages sent
    int input = msg_count;

    int count;
    MPI_Allreduce(&input, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    msg_count = count;
#endif
}

int RankSyncSerialSkip::sig_end_(0);
int RankSyncSerialSkip::sig_usr_(0);
int RankSyncSerialSkip::sig_alrm_(0);

} // namespace SST
