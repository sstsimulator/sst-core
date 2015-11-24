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
#include "sst/core/serialization.h"
#include "sst/core/rankSync.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"

#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>

#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif

namespace SST {

/**
   Class that implements the slave functionality for rank syncs.
 */
class RankSyncSlave : public Action {
public:
    RankSyncSlave(Core::ThreadSafe::Barrier&, TimeConverter* period);
    virtual ~RankSyncSlave();
    void execute();

private:
    Core::ThreadSafe::Barrier& barrier;
    TimeConverter* period;
    double totalWait;
};

RankSyncSlave::RankSyncSlave(Core::ThreadSafe::Barrier& barrier, TimeConverter* period) :
    Action(),
    barrier(barrier),
    period(period),
    totalWait(0.0)
{
    setPriority(SYNCPRIORITY);
}


RankSyncSlave::~RankSyncSlave()
{
    if ( totalWait > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0, "RankSyncSlave total Barrier wait time: %lg sec\n", totalWait);
}


void RankSyncSlave::execute() {
    totalWait += barrier.wait();
    totalWait += barrier.wait();
    SimTime_t next = Simulation::getSimulation()->getCurrentSimCycle() + period->getFactor();
    Simulation::getSimulation()->insertActivity( next, this );
}

/**
   Class that implements the master functionality for rank syncs.
 */
class RankSyncMaster : public Action {
public:
    RankSyncMaster(RankSync* sync, Core::ThreadSafe::Barrier&, TimeConverter* period);
    virtual ~RankSyncMaster();
    void execute();

private:
    RankSync* sync;
    Core::ThreadSafe::Barrier& barrier;
    TimeConverter* period;
    double totalWait;
};

RankSyncMaster::RankSyncMaster(RankSync* sync, Core::ThreadSafe::Barrier& barrier, TimeConverter* period) :
    Action(),
    sync(sync),
    barrier(barrier),
    period(period),
    totalWait(0.0)
{
    setPriority(SYNCPRIORITY);
}

RankSyncMaster::~RankSyncMaster()
{
    if ( totalWait > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0, "RankSyncSlave total Barrier wait time: %lg sec\n", totalWait);
}

void RankSyncMaster::execute() {
#if 0
    // First thing to do is fill up the work queue, then barrier to
    // make sure everything is sent before proceeding.
    for ( auto i = sync->comm_send_map.begin(); i != sync->comm_send_map.end(); ++i ) {
        serializeQ->try_insert(&(i->second));
    }
#endif
    totalWait += barrier.wait();
#if 0
    // Now, I just wait until I get serialized buffers back to send.
    // Can't just check for empty to see if I'm done.  Need to count
    // how many requests are returned and stop when they're all done.
    uint32_t count = comm_send_map.size();
    while ( count != 0 ) {
        
    }
#endif
    
    sync->execute();
    totalWait += barrier.wait();
    SimTime_t next = Simulation::getSimulation()->getCurrentSimCycle() + period->getFactor();
    Simulation::getSimulation()->insertActivity( next, this );
}


///// RankSync class /////

Action*
RankSync::getSlaveAction()
{
    if ( max_period == NULL ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO,1,"Call to getSlaveAction() before call to setMaxPeriod().  Exiting...\n");
    }
    return new RankSyncSlave(barrier,max_period);
}

Action*
RankSync::getMasterAction()
{
    if ( max_period == NULL ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO,1,"Call to getMasterAction() before call to setMaxPeriod().  Exiting...\n");
    }
    return new RankSyncMaster(this,barrier,max_period);
}

    
RankSync::RankSync(Core::ThreadSafe::Barrier& barrier) :
    SyncBase(),
    barrier(barrier),
    mpiWaitTime(0.0),
    deserializeTime(0.0)
{
}
    
RankSync::~RankSync()
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

    if ( mpiWaitTime > 0.0 || deserializeTime > 0.0 ) {
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0, "RankSync mpiWait: %lg sec  deserializeWait:  %lg sec\n", mpiWaitTime, deserializeTime);
    }
}
    
ActivityQueue* RankSync::registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link)
{
    SyncQueue* queue;
    if ( comm_send_map.count(to_rank) == 0 ) {
        queue = comm_send_map[to_rank].squeue = new SyncQueue();
        comm_send_map[to_rank].remote_size = 4096;
        comm_send_map[to_rank].dest = to_rank;
    } else {
        queue = comm_send_map[to_rank].squeue;
    }

    // This is a bit of an abuse of a RankInfo, but the rank will be
    // the remote rank and the thread will be the local thread.
    RankInfo ri = RankInfo(to_rank.rank,from_rank.thread);
    if ( comm_recv_map.count(ri) == 0 ) {
        comm_recv_map[ri].rbuf = new char[4096];
        comm_recv_map[ri].local_size = 4096;
        comm_recv_map[ri].remote_rank = ri.rank;
        comm_recv_map[ri].local_thread = ri.thread;
    }
    
    link_map[link_id] = link;
#ifdef __SST_DEBUG_EVENT_TRACKING__
    link->setSendingComponentInfo("SYNC", "SYNC", "");
#endif
    return queue;
}

void
RankSync::finalizeLinkConfigurations() {
    for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
        // i->second->finalizeConfiguration();
        finalizeConfiguration(i->second);
    }

    // Tells us we're done with registering links.  Can now size the
    // work queues
    serializeQ = new Core::ThreadSafe::BoundedQueue<comm_pair_send*>(comm_send_map.size());
    sendQ = new Core::ThreadSafe::BoundedQueue<comm_pair_send*>(comm_send_map.size());

}

uint64_t
RankSync::getDataSize() const {
    size_t count = 0;
    for ( auto it = comm_send_map.begin(); it != comm_send_map.end(); ++it ) {
        count += it->second.squeue->getDataSize();
    }
    for ( auto it = comm_recv_map.begin(); it != comm_recv_map.end(); ++it ) {
        count += it->second.local_size;
    }
    return count;
}

inline int make_tag( int thread, int tag ) {
    int thread_mask = 0x3ff;  // Limited to 1024 threads for now
    int tag_shift = 10;

    return ( tag << tag_shift ) | ( thread & thread_mask );
}


#ifdef SST_CONFIG_HAVE_MPI
int
RankSync::sendQueuedEvents(comm_pair_send& send_info, MPI_Request* request)
{
    int sent = 0;
    char* send_buffer = send_info.squeue->getData();
    // Cast to Header so we can get/fill in data
    SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
    // Simulation::getSimulation()->getSimulationOutput().output("Data size = %d\n", hdr->buffer_size);
    int tag = make_tag(send_info.dest.thread, 1);
    // Check to see if remote queue is big enough for data
    if ( send_info.remote_size < hdr->buffer_size ) {
        // not big enough, send message that will tell remote side to get larger buffer
        hdr->mode = 1;
        MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE,
                  send_info.dest.rank/*dest*/, tag, MPI_COMM_WORLD, request);
        sent++;
        request++;
        send_info.remote_size = hdr->buffer_size;
        tag = make_tag(send_info.dest.thread, 2);
    }
    else {
        hdr->mode = 0;
    }
    MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE,
              send_info.dest.rank/*dest*/, tag, MPI_COMM_WORLD, request);

    return sent;
    
}
#endif

#ifdef SST_CONFIG_HAVE_MPI
std::vector<Activity*>*
RankSync::recvEvents(comm_pair_recv& recv_info)
{
    // Get the buffer and deserialize all the events
    char* buffer = recv_info.rbuf;
    
    SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(buffer);

    unsigned int size = hdr->buffer_size;
    int mode = hdr->mode;
    
    if ( mode == 1 ) {
        // May need to resize the buffer
        if ( size > recv_info.local_size ) {
            delete[] recv_info.rbuf;
            recv_info.rbuf = new char[size];
            recv_info.local_size = size;
        }
        MPI_Recv(recv_info.rbuf, recv_info.local_size, MPI_BYTE,
                 recv_info.remote_rank, make_tag(recv_info.local_thread, 2),
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        buffer = recv_info.rbuf;
    }
    
    auto deserialStart = SST::Core::Profile::now();
    boost::iostreams::basic_array_source<char> source(&buffer[sizeof(SyncQueue::Header)],size-sizeof(SyncQueue::Header));
    boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
    boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header | boost::archive::no_tracking );
    
    std::vector<Activity*>* activities = new std::vector<Activity*>();
    ia >> *activities;
    deserializeTime += SST::Core::Profile::getElapsed(deserialStart);
    return activities;
    
}
#endif

void
RankSync::execute(void)
{
    // static Output tmp_debug("@r: @t:  ",5,-1,Output::FILE);
#ifdef SST_CONFIG_HAVE_MPI
    // std::vector<boost::mpi::request> pending_requests;
    
    //Maximum number of outstanding requests is 3 times the number
    // of ranks I communicate with (1 recv, 2 sends per rank)
    MPI_Request sreqs[2 * comm_send_map.size()];
    MPI_Request rreqs[comm_recv_map.size()];
    int sreq_count = 0;
    int rreq_count = 0;
    
    for (auto i = comm_send_map.begin() ; i != comm_send_map.end() ; ++i) {
        
        // Do all the sends
        int sent = sendQueuedEvents(i->second, &sreqs[sreq_count]);
        sreq_count += sent;
#if 0
        // Get the buffer from the syncQueue
        char* send_buffer = i->second.squeue->getData();
        // Cast to Header so we can get/fill in data
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
        // Simulation::getSimulation()->getSimulationOutput().output("Data size = %d\n", hdr->buffer_size);
        int tag = make_tag(i->first.thread, 1);
        // Check to see if remote queue is big enough for data
        if ( i->second.remote_size < hdr->buffer_size ) {
            // not big enough, send message that will tell remote side to get larger buffer
            hdr->mode = 1;
            MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE,
                      i->first.rank/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
            i->second.remote_size = hdr->buffer_size;
            tag = make_tag(i->first.thread, 2);
        }
        else {
            hdr->mode = 0;
        }
        MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE,
                  i->first.rank/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
#endif   
    }

    
    for (auto i = comm_recv_map.begin() ; i != comm_recv_map.end() ; ++i) {
        // Post all the receives
        MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE,
                  i->first.rank, make_tag(i->first.thread, 1),
                  MPI_COMM_WORLD, &rreqs[rreq_count++]);
    }
    
    // Wait for all sends and recvs to complete
    Simulation* sim = Simulation::getSimulation();
    SimTime_t current_cycle = sim->getCurrentSimCycle();
    
    auto mpiWaitStart = SST::Core::Profile::now();
    MPI_Waitall(rreq_count, rreqs, MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(mpiWaitStart);
    
    for (auto i = comm_recv_map.begin() ; i != comm_recv_map.end() ; ++i) {
#if 0
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
                     i->first.rank, make_tag(i->first.thread, 2),
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buffer = i->second.rbuf;
        }
        
        boost::iostreams::basic_array_source<char> source(&buffer[sizeof(SyncQueue::Header)],size-sizeof(SyncQueue::Header));
        boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
        boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header | boost::archive::no_tracking );

        std::vector<Activity*> activities;
        ia >> activities;
#endif

        std::vector<Activity*>* activities_ptr = recvEvents(i->second);
        std::vector<Activity*>& activities = *activities_ptr;
        
        for ( unsigned int j = 0; j < activities.size(); j++ ) {
            
            Event* ev = static_cast<Event*>(activities[j]);
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
        
        // for ( int j = 0; j < count; j++ ) {
        //     Event* ev;
        //     ia >> ev;
        
        //     link_map_t::iterator link = link_map.find(ev->getLinkId());
        //     if (link == link_map.end()) {
        //         printf("Link not found in map!\n");
        //         abort();
        //     } else {
        //         // Need to figure out what the "delay" is for this event.
        //         SimTime_t delay = ev->getDeliveryTime() - current_cycle;
        //         link->second->send(delay,ev);
        //     }
        // }
    }

    // Clear the SyncQueues used to send the data after all the sends have completed
    mpiWaitStart = SST::Core::Profile::now();
    MPI_Waitall(sreq_count, sreqs, MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(mpiWaitStart);
    
    for (auto i = comm_send_map.begin() ; i != comm_send_map.end() ; ++i) {
        i->second.squeue->clear();
    }
    
    // If we have an Exit object, fire it to see if we need end simulation
    if ( exit != NULL ) exit->check();
    
    // Check to see when the next event is scheduled, then do an
    // all_reduce with min operator and set next sync time to be
    // min + max_period.
    // boost::mpi::communicator world;
    // SimTime_t input = Simulation::getSimulation()->getNextActivityTime();;
    // SimTime_t min_time;
    // all_reduce( world, &input, 1, &min_time, boost::mpi::minimum<SimTime_t>() );
    
    SimTime_t input = Simulation::getSimulation()->getNextActivityTime();;
    SimTime_t min_time;
    mpiWaitStart = SST::Core::Profile::now();
    MPI_Allreduce( &input, &min_time, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD );
    mpiWaitTime += SST::Core::Profile::getElapsed(mpiWaitStart);
    
    // tmp_debug.output(CALL_INFO,"  my_time: %" PRIu64 ", min_time: %" PRIu64 "\n",input, min_time);
    
    // SimTime_t next = min_time + max_period->getFactor();
    // sim->insertActivity( next, this );
#endif
}

int
RankSync::exchangeLinkInitData(int msg_count)
{
#ifdef SST_CONFIG_HAVE_MPI
    
    //Maximum number of outstanding requests is 3 times the number
    // of ranks I communicate with (1 recv, 2 sends per rank)
    MPI_Request sreqs[2 * comm_send_map.size()];
    MPI_Request rreqs[comm_recv_map.size()];
    int rreq_count = 0;
    int sreq_count = 0;
    
    for (auto i = comm_send_map.begin() ; i != comm_send_map.end() ; ++i) {
        
        // Do all the sends
        // Get the buffer from the syncQueue
        char* send_buffer = i->second.squeue->getData();
        // Cast to Header so we can get/fill in data
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
        int tag = make_tag(i->first.thread, 1);
        // Check to see if remote queue is big enough for data
        if ( i->second.remote_size < hdr->buffer_size ) {
            // std::cout << i->second.remote_size << ", " << hdr->buffer_size << std::endl;
            // not big enough, send message that will tell remote side to get larger buffer
            hdr->mode = 1;
            // std::cout << "MPI_Isend of header only to " << i->first << " with tag " << tag << std::endl;
            MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE, i->first.rank/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
            i->second.remote_size = hdr->buffer_size;
            tag = make_tag(i->first.thread, 2);
        }
        else {
            hdr->mode = 0;
        }
        // std::cout << "MPI_Isend of whole buffer to " << i->first << " with tag " << tag << std::endl;
        MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE, i->first.rank/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
        
        
    }

    for (auto i = comm_recv_map.begin() ; i != comm_recv_map.end() ; ++i) {
        // Post all the receives
        MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first.rank,
                  make_tag(i->first.thread, 1), MPI_COMM_WORLD, &rreqs[rreq_count++]);
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
            MPI_Recv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first.rank,
                     make_tag(i->first.thread, 2), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
    return count;
#else
    return 0;
#endif
}

template<class Archive>
void
RankSync::serialize(Archive & ar, const unsigned int version)
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


