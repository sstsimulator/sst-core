// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/rankSyncSerialSkip.h"

#include "sst/core/serialization/serializer.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"
#include "sst/core/profile.h"

#include <sst/core/warnmacros.h>
#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#define UNUSED_WO_MPI(x) x
#else
#define UNUSED_WO_MPI(x) UNUSED(x)
#endif


namespace SST {

// Static Data Members
SimTime_t RankSyncSerialSkip::myNextSyncTime = 0;


RankSyncSerialSkip::RankSyncSerialSkip(TimeConverter* UNUSED(minPartTC)) :
    NewRankSync(),
    mpiWaitTime(0.0),
    deserializeTime(0.0)
{
    max_period = Simulation::getSimulation()->getMinPartTC();
    myNextSyncTime = max_period->getFactor();
}

RankSyncSerialSkip::~RankSyncSerialSkip()
{
    for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
        delete i->second.squeue;
    }
    comm_map.clear();
    
    for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
        delete i->second;
    }
    link_map.clear();

    if ( mpiWaitTime > 0.0 || deserializeTime > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0, "RankSyncSerialSkip mpiWait: %lg sec  deserializeWait:  %lg sec\n", mpiWaitTime, deserializeTime);
}
    
ActivityQueue* RankSyncSerialSkip::registerLink(const RankInfo& to_rank, const RankInfo& UNUSED(from_rank), LinkId_t link_id, Link* link)
{
    SyncQueue* queue;
    if ( comm_map.count(to_rank.rank) == 0 ) {
        queue = comm_map[to_rank.rank].squeue = new SyncQueue();
        comm_map[to_rank.rank].rbuf = new char[4096];
        comm_map[to_rank.rank].local_size = 4096;
        comm_map[to_rank.rank].remote_size = 4096;
    } else {
        queue = comm_map[to_rank.rank].squeue;
    }
	
    link_map[link_id] = link;
#ifdef __SST_DEBUG_EVENT_TRACKING__
    link->setSendingComponentInfo("SYNC", "SYNC", "");
#endif
    return queue;
}

void
RankSyncSerialSkip::finalizeLinkConfigurations() {
    for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
        finalizeConfiguration(i->second);
    }
}

void
RankSyncSerialSkip::prepareForComplete() {
    for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
        prepareForCompleteInt(i->second);
    }
}

uint64_t
RankSyncSerialSkip::getDataSize() const {
    size_t count = 0;
    for ( comm_map_t::const_iterator it = comm_map.begin();
          it != comm_map.end(); ++it ) {
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
RankSyncSerialSkip::exchange(void)
{
#ifdef SST_CONFIG_HAVE_MPI
    
    //Maximum number of outstanding requests is 3 times the number
    // of ranks I communicate with (1 recv, 2 sends per rank)
    MPI_Request sreqs[2 * comm_map.size()];
    MPI_Request rreqs[comm_map.size()];
    int sreq_count = 0;
    int rreq_count = 0;
    
    for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
        
        // Do all the sends
        // Get the buffer from the syncQueue
        char* send_buffer = i->second.squeue->getData();
        // Cast to Header so we can get/fill in data
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
        // Simulation::getSimulation()->getSimulationOutput().output("Data size = %d\n", hdr->buffer_size);
        int tag = 1;
        // Check to see if remote queue is big enough for data
        if ( i->second.remote_size < hdr->buffer_size ) {
            // not big enough, send message that will tell remote side to get larger buffer
            hdr->mode = 1;
            MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE,
                      i->first/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
            i->second.remote_size = hdr->buffer_size;
            tag = 2;
        }
        else {
            hdr->mode = 0;
        }
        MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE,
                  i->first/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
        
        // Post all the receives
        MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE,
                  i->first, 1, MPI_COMM_WORLD, &rreqs[rreq_count++]);
    }
    
    // Wait for all sends and recvs to complete
    Simulation* sim = Simulation::getSimulation();
    SimTime_t current_cycle = sim->getCurrentSimCycle();
    
    auto waitStart = SST::Core::Profile::now();
    MPI_Waitall(rreq_count, rreqs, MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);
    
    for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
        // Get the buffer and deserialize all the events
        char* buffer = i->second.rbuf;
        
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(buffer);
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
                     i->first, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buffer = i->second.rbuf;
        }
        
        auto deserialStart = SST::Core::Profile::now();

        SST::Core::Serialization::serializer ser;
        ser.start_unpacking(&buffer[sizeof(SyncQueue::Header)],size-sizeof(SyncQueue::Header));

        std::vector<Activity*> activities;
        activities.clear();
        ser & activities;
        
        deserializeTime += SST::Core::Profile::getElapsed(deserialStart);

        for ( unsigned int j = 0; j < activities.size(); j++ ) {
            
            Event* ev = static_cast<Event*>(activities[j]);
            link_map_t::iterator link = link_map.find(ev->getLinkId());
            if (link == link_map.end()) {
                Simulation::getSimulationOutput().fatal(CALL_INFO,1,"Link not found in map!\n");
            } else {
                // Need to figure out what the "delay" is for this event.
                SimTime_t delay = ev->getDeliveryTime() - current_cycle;
                link->second->send(delay,ev);
            }
        }

        activities.clear();

    }

    // Clear the SyncQueues used to send the data after all the sends have completed
    waitStart = SST::Core::Profile::now();
    MPI_Waitall(sreq_count, sreqs, MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);
    
    for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
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
    MPI_Request sreqs[2 * comm_map.size()];
    MPI_Request rreqs[comm_map.size()];
    int rreq_count = 0;
    int sreq_count = 0;
    
    for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
        
        // Do all the sends
        // Get the buffer from the syncQueue
        char* send_buffer = i->second.squeue->getData();
        // Cast to Header so we can get/fill in data
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
        int tag = 1;
        // Check to see if remote queue is big enough for data
        if ( i->second.remote_size < hdr->buffer_size ) {
            // not big enough, send message that will tell remote side to get larger buffer
            hdr->mode = 1;
            MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE, i->first/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
            i->second.remote_size = hdr->buffer_size;
            tag = 2;
        }
        else {
            hdr->mode = 0;
        }
        MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE, i->first/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
        
        // Post all the receives
        MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first, 1, MPI_COMM_WORLD, &rreqs[rreq_count++]);
        
    }
    
    // Wait for all recvs to complete
    MPI_Waitall(rreq_count, rreqs, MPI_STATUSES_IGNORE);
    
    
    for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
        
        // Get the buffer and deserialize all the events
        char* buffer = i->second.rbuf;
        
        SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(buffer);
        unsigned int size = hdr->buffer_size;
        int mode = hdr->mode;
        
        if ( mode == 1 ) {
            // May need to resize the buffer
            if ( size > i->second.local_size ) {
                delete[] i->second.rbuf;
                i->second.rbuf = new char[size];
                i->second.local_size = size;
            }
            MPI_Recv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buffer = i->second.rbuf;
        }
        
        SST::Core::Serialization::serializer ser;
        ser.start_unpacking(&buffer[sizeof(SyncQueue::Header)],size-sizeof(SyncQueue::Header));
        
        std::vector<Activity*> activities;
        ser & activities;
        for ( unsigned int j = 0; j < activities.size(); j++ ) {
            
            Event* ev = static_cast<Event*>(activities[j]);
            link_map_t::iterator link = link_map.find(ev->getLinkId());
            if (link == link_map.end()) {
                Simulation::getSimulationOutput().fatal(CALL_INFO,1,"Link not found in map!\n");
            } else {
                sendUntimedData_sync(link->second,ev);
            }
        }
        
        
    }
    
    // Clear the SyncQueues used to send the data after all the sends have completed
    MPI_Waitall(sreq_count, sreqs, MPI_STATUSES_IGNORE);
    
    for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
        i->second.squeue->clear();
    }
    
    // Do an allreduce to see if there were any messages sent
    int input = msg_count;

    int count;
    MPI_Allreduce( &input, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    msg_count = count;
#endif
}

} // namespace SST


