// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/rankSyncParallelSkip.h"

#include "sst/core/serialization.h"
#include "sst/core/sync.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"
#include "sst/core/profile.h"

#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>

#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif


namespace SST {

// Static Data Members
SimTime_t RankSyncParallelSkip::myNextSyncTime = 0;


///// RankSyncParallelSkip class /////
    
RankSyncParallelSkip::RankSyncParallelSkip(RankInfo num_ranks, Core::ThreadSafe::Barrier& barrier, TimeConverter* minPartTC) :
    NewRankSync(),
    minPartTC(minPartTC),
    mpiWaitTime(0.0),
    deserializeTime(0.0),
    send_count(0),
    barrier(barrier)
{
    max_period = Simulation::getSimulation()->getMinPartTC();
    myNextSyncTime = max_period->getFactor();
    recv_count = new int[num_ranks.thread];
    for ( int i = 0; i < num_ranks.thread; i++ ) {
        recv_count[i] = 0;
    }
    deserialize_queue = new SST::Core::ThreadSafe::UnboundedQueue<comm_recv_pair*>[num_ranks.thread];
}

RankSyncParallelSkip::~RankSyncParallelSkip()
{
    for (auto i = comm_send_map.begin() ; i != comm_send_map.end() ; ++i) {
        delete i->second.squeue;
    }
    comm_send_map.clear();
    
    for (auto i = comm_recv_map.begin() ; i != comm_recv_map.end() ; ++i) {
        delete[] i->second.rbuf;
    }
    comm_recv_map.clear();
    
    for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
        delete i->second;
    }
    link_map.clear();

    delete[] recv_count;
    delete[] deserialize_queue;

    if ( mpiWaitTime > 0.0 || deserializeTime > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0, "RankSyncParallelSkip mpiWait: %lg sec  deserializeWait:  %lg sec\n", mpiWaitTime, deserializeTime);
}
    
ActivityQueue* RankSyncParallelSkip::registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link)
{
    // TraceFunction trace(CALL_INFO_LONG);

    // For sends, we track the remote rank and thread ID
    SyncQueue* queue;
    if ( comm_send_map.count(to_rank) == 0 ) {
        send_count++;
        comm_send_map[to_rank].to_rank = to_rank;
        queue = comm_send_map[to_rank].squeue = new SyncQueue();
        comm_send_map[to_rank].remote_size = 4096;
    } else {
        queue = comm_send_map[to_rank].squeue;
    }

    // For recv's, we track the remote rank and the local thread.
    // Need to create a RankInfo object with this data to use in the
    // map.
    RankInfo remote_rank_local_thread(to_rank.rank, from_rank.thread);
    if ( comm_recv_map.count(remote_rank_local_thread) == 0 ) {
        recv_count[from_rank.thread]++;
        comm_recv_map[remote_rank_local_thread].remote_rank = to_rank.rank;
        comm_recv_map[remote_rank_local_thread].local_thread = from_rank.thread;
        comm_recv_map[remote_rank_local_thread].rbuf = new char[4096];
        comm_recv_map[remote_rank_local_thread].local_size = 4096;
    }
	
    link_map[link_id] = link;
#ifdef __SST_DEBUG_EVENT_TRACKING__
    link->setSendingComponentInfo("SYNC", "SYNC", "");
#endif
    // trace.getOutput().output(CALL_INFO,"queue = %p\n",queue);
    return queue;
}

void
RankSyncParallelSkip::finalizeLinkConfigurations() {
    // TraceFunction trace(CALL_INFO_LONG);
    for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
        // i->second->finalizeConfiguration();
        finalizeConfiguration(i->second);
    }

    // Set the size of the BoundedQueue that is the work queue for
    // serializations
    serialize_queue.initialize(comm_send_map.size());
    send_queue.initialize(comm_send_map.size());
}

uint64_t
RankSyncParallelSkip::getDataSize() const {
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
    // TraceFunction trace(CALL_INFO_LONG);
    if ( thread == 0 ) {
        // totalWait += barrier.wait();
        // barrier.wait();
        exchange_master(thread);
        // totalWait += barrier.wait();
        barrier.wait();
        // SimTime_t next = Simulation::getSimulation()->getCurrentSimCycle() + period->getFactor();
        // Simulation::getSimulation()->insertActivity( next, this );
    }
    else {
        // totalWait += barrier.wait();
        // totalWait += barrier.wait();
        barrier.wait();
        exchange_slave(thread);
        barrier.wait();
        // SimTime_t next = Simulation::getSimulation()->getCurrentSimCycle() + period->getFactor();
        // Simulation::getSimulation()->insertActivity( next, this );
    }
}

void
RankSyncParallelSkip::exchange_slave(int thread)
{

    // Check the serialize_queue for work.
    comm_send_pair* ser;
    while ( serialize_queue.try_remove(ser) ) {
        // Serialize the events
        ser->sbuf = ser->squeue->getData();
        // Send back to master to do MPI send
        send_queue.try_insert(ser);
    }        

    // After serialization is done, start processing the receives.
    
    // TraceFunction trace(CALL_INFO_LONG);
    int my_recv_count = recv_count[thread];

    // Do nothing until there are events to be sent on this thread's
    // links
    Simulation* sim = Simulation::getSimulation();
    SimTime_t current_cycle = sim->getCurrentSimCycle();
    while ( my_recv_count != 0 ) {
        // while ( deserialize_queue[thread].empty() ) _mm_pause();
        comm_recv_pair* recv = deserialize_queue[thread].remove();        
        my_recv_count--;

        deserializeMessage(recv);
        std::vector<Activity*>& activities = recv->activity_vec;
        
        for ( int i = 0; i < recv->activity_vec.size(); i++ ) {
            Event* ev = static_cast<Event*>(recv->activity_vec[i]);
            link_map_t::iterator link = link_map.find(ev->getLinkId());
            if (link == link_map.end()) {
                printf("Link not found in map!\n");
                abort();
            } else {
                // Need to figure out what the "delay" is for this event.
                SimTime_t delay = ev->getDeliveryTime() - current_cycle;
                link->second->send(delay,ev);
            }
        }
        recv->activity_vec.clear();
    }
    barrier.wait();
    
}

void
RankSyncParallelSkip::exchange_master(int thread)
{
    // TraceFunction trace(CALL_INFO_LONG);
    // Simulation::getSimulation()->getSimulationOutput().output("Entering RankSyncParallelSkip::execute()\n");
    // static Output tmp_debug("@r: @t:  ",5,-1,Output::FILE);
#ifdef SST_CONFIG_HAVE_MPI
    // std::vector<boost::mpi::request> pending_requests;
    
    //Maximum number of outstanding requests is 3 times the number
    // of ranks I communicate with (1 recv, 2 sends per rank)
    MPI_Request sreqs[2 * comm_send_map.size()];
    MPI_Request rreqs[comm_recv_map.size()];
    int sreq_count = 0;
    int rreq_count = 0;

    // First thing to do is fill the serialize_queue.
    for (auto i = comm_send_map.begin() ; i != comm_send_map.end() ; ++i) {
        serialize_queue.try_insert(&(i->second));
    }    

    barrier.wait();
    
    for (auto i = comm_recv_map.begin() ; i != comm_recv_map.end() ; ++i) {
        // Post all the receives
        int tag = 2 * i->second.local_thread;
        MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE,
                  i->second.remote_rank, tag, MPI_COMM_WORLD, &rreqs[rreq_count++]);
    }

    // Do all the sends, but if there are no sends to do, then help
    // with serialization
    int my_send_count = send_count;
    comm_send_pair* send;
    while ( my_send_count != 0 ) {
        if ( send_queue.try_remove(send) ) {
            my_send_count--;
    
            char* send_buffer = send->sbuf;
            // Cast to Header so we can get/fill in data
            SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
            int tag = 2 * send->to_rank.thread;
            // Check to see if remote queue is big enough for data
            if ( send->remote_size < hdr->buffer_size ) {
                // not big enough, send message that will tell remote side to get larger buffer
                hdr->mode = 1;
                MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE,
                          send->to_rank.rank/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
                send->remote_size = hdr->buffer_size;
                tag = 2 * send->to_rank.thread + 1;
            }
            else {
                hdr->mode = 0;
            }
            MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE,
                      send->to_rank.rank/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
        }
        else if ( serialize_queue.try_remove(send) ) {
            // Serialize the events
            send->sbuf = send->squeue->getData();
            // Send back to master to do MPI send
            send_queue.try_insert(send);
        }
        else {
            _mm_pause();
        }
    }
    
    // Wait for all sends and recvs to complete
    Simulation* sim = Simulation::getSimulation();
    SimTime_t current_cycle = sim->getCurrentSimCycle();
    
    auto waitStart = SST::Core::Profile::now();
    MPI_Waitall(rreq_count, rreqs, MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);
    
    for (auto i = comm_recv_map.begin() ; i != comm_recv_map.end() ; ++i) {
        // Get the buffer and deserialize all the events
        char* buffer = i->second.rbuf;
        
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(buffer);
//            int count = hdr->count;
        unsigned int size = hdr->buffer_size;
        int mode = hdr->mode;
        
        if ( mode == 1 ) {
            // May need to resize the buffer
            if ( size > i->second.local_size ) {
                delete[] i->second.rbuf;
                i->second.rbuf = new char[size];
                i->second.local_size = size;
            }
            MPI_Recv(i->second.rbuf, i->second.local_size, MPI_BYTE,
                     i->second.remote_rank, 2 * i->second.local_thread + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buffer = i->second.rbuf;

        }
        
        // deserializeMessage(&(i->second));
        // std::vector<Activity*>& activities = i->second.activity_vec;

        deserialize_queue[i->second.local_thread].insert(&(i->second));
        
    }

    // For now simply call exchange_slave() to deliver events
    exchange_slave(0);

    // Clear the SyncQueues used to send the data after all the sends have completed
    waitStart = SST::Core::Profile::now();
    MPI_Waitall(sreq_count, sreqs, MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);
    
    for (auto i = comm_send_map.begin() ; i != comm_send_map.end() ; ++i) {
        i->second.squeue->clear();
    }
    
    // If we have an Exit object, fire it to see if we need end simulation
    // if ( exit != NULL ) exit->check();
    
    // Check to see when the next event is scheduled, then do an
    // all_reduce with min operator and set next sync time to be
    // min + max_period.

    // Need to get the local minimum, then do a global minimum
    // SimTime_t input = Simulation::getSimulation()->getNextActivityTime();
    SimTime_t input = Simulation::getLocalMinimumNextActivityTime();
    SimTime_t min_time;
    MPI_Allreduce( &input, &min_time, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD );

    myNextSyncTime = min_time + max_period->getFactor();
    
    // tmp_debug.output(CALL_INFO,"  my_time: %" PRIu64 ", min_time: %" PRIu64 "\n",input, min_time);
    
    // SimTime_t next = min_time + max_period->getFactor();
    // sim->insertActivity( next, this );
#endif
}

void
RankSyncParallelSkip::exchangeLinkInitData(int thread, std::atomic<int>& msg_count)
{
    // TraceFunction trace(CALL_INFO_LONG);
#ifdef SST_CONFIG_HAVE_MPI
    if ( thread != 0 ) {
        return;
    }
    //Maximum number of outstanding requests is 3 times the number
    // of ranks I communicate with (1 recv, 2 sends per rank)
    MPI_Request sreqs[2 * comm_send_map.size()];
    MPI_Request rreqs[comm_recv_map.size()];
    int rreq_count = 0;
    int sreq_count = 0;

    for ( auto i = comm_recv_map.begin(); i != comm_recv_map.end(); ++i ) {
        // Post all the receives
        int tag = 2 * i->second.local_thread;
        MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE,
                  i->second.remote_rank, tag, MPI_COMM_WORLD, &rreqs[rreq_count++]);
    }
    
    for (auto i = comm_send_map.begin() ; i != comm_send_map.end() ; ++i) {
        
        // Do all the sends
        // Get the buffer from the syncQueue
        char* send_buffer = i->second.squeue->getData();
        // Cast to Header so we can get/fill in data
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
        int tag = 2 * i->second.to_rank.thread;
        // Check to see if remote queue is big enough for data
        if ( i->second.remote_size < hdr->buffer_size ) {
            // std::cout << i->second.remote_size << ", " << hdr->buffer_size << std::endl;
            // not big enough, send message that will tell remote side to get larger buffer
            hdr->mode = 1;
            // std::cout << "MPI_Isend of header only to " << i->first << " with tag " << tag << std::endl;
            MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE,
                      i->second.to_rank.rank/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
            i->second.remote_size = hdr->buffer_size;
            tag = 2 * i->second.to_rank.thread + 1;
        }
        else {
            hdr->mode = 0;
        }
        // std::cout << "MPI_Isend of whole buffer to " << i->first << " with tag " << tag << std::endl;
        MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE,
                  i->second.to_rank.rank/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
        
        
    }
    
    // Wait for all recvs to complete
    // std::cout << "Start waitall" << std::endl;
    MPI_Waitall(rreq_count, rreqs, MPI_STATUSES_IGNORE);
    // std::cout << "End waitall" << std::endl;
    
    
    for (auto i = comm_recv_map.begin() ; i != comm_recv_map.end() ; ++i) {
        
        // Get the buffer and deserialize all the events
        char* buffer = i->second.rbuf;
        
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(buffer);
//            int count = hdr->count;
        unsigned int size = hdr->buffer_size;
        int mode = hdr->mode;
        
        if ( mode == 1 ) {
            // May need to resize the buffer
            if ( size > i->second.local_size ) {
                delete[] i->second.rbuf;
                i->second.rbuf = new char[size];
                i->second.local_size = size;
            }
            MPI_Recv(i->second.rbuf, i->second.local_size, MPI_BYTE,
                     i->second.remote_rank, 2 * i->second.local_thread + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buffer = i->second.rbuf;
        }
        
        boost::iostreams::basic_array_source<char> source(&buffer[sizeof(SyncQueue::Header)],size-sizeof(SyncQueue::Header));
        boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
        boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header | boost::archive::no_tracking );
        
        std::vector<Activity*> activities;
        ia >> activities;
        for ( unsigned int j = 0; j < activities.size(); j++ ) {
            
            Event* ev = static_cast<Event*>(activities[j]);
            link_map_t::iterator link = link_map.find(ev->getLinkId());
            if (link == link_map.end()) {
                printf("Link not found in map!\n");
                abort();
            } else {
                sendInitData_sync(link->second,ev);
            }
        }
        
        
        // for ( int j = 0; j < count; j++ ) {
        //     Event* ev;
        //     ia >> ev;
        
        
        //     link_map_t::iterator link = link_map.find(ev->getLinkId());
        //     if (link == link_map.end()) {
        //         printf("Link not found in map!\n");
        //         abort();
        //     } else {
        //         sendInitData_sync(link->second,ev);
        //     }
        // }
        
        // Clear the receive vector
        // tmp->clear();
    }
    
    // Clear the SyncQueues used to send the data after all the sends have completed
    MPI_Waitall(sreq_count, sreqs, MPI_STATUSES_IGNORE);
    
    for (auto i = comm_send_map.begin() ; i != comm_send_map.end() ; ++i) {
        i->second.squeue->clear();
    }
    
    // Do an allreduce to see if there were any messages sent
    // boost::mpi::communicator world;
    // int input = msg_count;
    // int count;
    // all_reduce( world, &input, 1, &count, std::plus<int>() );
    // return count;
    int input = msg_count;

    int count;
    MPI_Allreduce( &input, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    msg_count = count;
#endif
}

void
RankSyncParallelSkip::deserializeMessage(comm_recv_pair* msg)
{
    char* buffer = msg->rbuf;
    SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(buffer);
    unsigned int size = hdr->buffer_size;
    
    auto deserialStart = SST::Core::Profile::now();
    boost::iostreams::basic_array_source<char> source(&buffer[sizeof(SyncQueue::Header)],size-sizeof(SyncQueue::Header));
    boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
    boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header | boost::archive::no_tracking );
    deserializeTime += SST::Core::Profile::getElapsed(deserialStart);
    
    // std::vector<Activity*> activities;
    ia >> msg->activity_vec;
}
    

template<class Archive>
void
RankSyncParallelSkip::serialize(Archive & ar, const unsigned int version)
{
    // printf("begin Sync::serialize\n");
    // ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SyncBase);
    // printf("  - Sync::period\n");
    // // ar & BOOST_SERIALIZATION_NVP(period);
    // printf("  - Sync::comm_map (%d)\n", (int) comm_map.size());
    // ar & BOOST_SERIALIZATION_NVP(comm_map);
    // printf("  - Sync::link_map (%d)\n", (int) link_map.size());
    // ar & BOOST_SERIALIZATION_NVP(link_map);
    // // don't serialize comm - let it be silently rebuilt at restart
    // printf("end Sync::serialize\n");
}


} // namespace SST


