// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/threadSyncSimpleSkip.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
//#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"

namespace SST {

SimTime_t ThreadSyncSimpleSkip::localMinimumNextActivityTime = 0;

/** Create a new ThreadSyncSimpleSkip object */
ThreadSyncSimpleSkip::ThreadSyncSimpleSkip(int num_threads, int thread, Simulation* sim) :
    NewThreadSync(),
    num_threads(num_threads),
    thread(thread),
    sim(sim),
    totalWaitTime(0.0)
{
    // TraceFunction trace(CALL_INFO_LONG);
    for ( int i = 0; i < num_threads; i++ ) {
        queues.push_back(new ThreadSyncQueue());
    }

    if ( sim->getRank().thread == 0 )
        barrier.resize(num_threads);

    if ( sim->getNumRanks().rank > 1 ) single_rank = false;
    else single_rank = true;

    max_period = sim->getInterThreadMinLatency();
    nextSyncTime = max_period;
}

ThreadSyncSimpleSkip::~ThreadSyncSimpleSkip()
{
    // TraceFunction trace(CALL_INFO_LONG);
    if ( totalWaitTime > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0, "ThreadSyncSimpleSkip total wait time: %lg seconds.\n", totalWaitTime);
    for ( int i = 0; i < num_threads; i++ ) {
        delete queues[i];
    }
    queues.clear();
}

// void
// ThreadSyncSimpleSkip::setMaxPeriod(TimeConverter* period)
// {
//     max_period = period;
//     SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
//     setPriority(THREADSYNCPRIORITY);
//     sim->insertActivity( next, this );
// }

void
ThreadSyncSimpleSkip::registerLink(LinkId_t link_id, Link* link)
{
    // TraceFunction trace(CALL_INFO_LONG);
    link_map[link_id] = link;
}

ActivityQueue*
ThreadSyncSimpleSkip::getQueueForThread(int tid)
{
    // TraceFunction trace(CALL_INFO_LONG);
    return queues[tid];
}

void
ThreadSyncSimpleSkip::before()
{
    // TraceFunction trace(CALL_INFO_LONG);

    // No need to barrier.  SyncManger already barriers before calling
    // this function

    // totalWaitTime += barrier.wait();

    // if ( disabled ) return;

    // Empty all the queues and send events on the links
    for ( int i = 0; i < queues.size(); i++ ) {
        ThreadSyncQueue* queue = queues[i];
        std::vector<Activity*>& vec = queue->getVector();
        for ( int j = 0; j < vec.size(); j++ ) {
            Event* ev = static_cast<Event*>(vec[j]);
            auto link = link_map.find(ev->getLinkId());
            if (link == link_map.end()) {
                printf("Link not found in map!\n");
                abort();
            } else {
                SimTime_t delay = ev->getDeliveryTime() - sim->getCurrentSimCycle();
                link->second->send(delay,ev);
            }
        }
        queue->clear();
    }

    // No need to barrier, SyncManger will barrier right after this
    // call

    // totalWaitTime += barrier.wait();

}

void
ThreadSyncSimpleSkip::after()
{
    // TraceFunction trace(CALL_INFO_LONG);

    // Use this nextSyncTime computation for no skip
    // nextSyncTime = sim->getCurrentSimCycle() + max_period;
    
    
    // No need to barrier.  SyncManger already barriers before calling
    // this function

    // totalWaitTime += barrier.wait();


    // Use this nextSyncTime computation for skipping

    // if ( thread == 0 ) localMinimumNextActivityTime = sim->getLocalMinimumNextActivityTime();
    // totalWaitTime += barrier.wait();
    // nextSyncTime = localMinimumNextActivityTime + max_period;
    nextSyncTime = sim->getLocalMinimumNextActivityTime() + max_period;
    totalWaitTime += barrier.wait();

}

void
ThreadSyncSimpleSkip::execute()
{
    // TraceFunction trace(CALL_INFO_LONG);

    totalWaitTime = barrier.wait();
    before();
    totalWaitTime = barrier.wait();
    after();
    

    // // TraceFunction trace(CALL_INFO_LONG);
    // totalWaitTime += barrier.wait();
    // // if ( disabled ) return;
    // // Empty all the queues and send events on the links
    // for ( int i = 0; i < queues.size(); i++ ) {
    //     ThreadSyncQueue* queue = queues[i];
    //     std::vector<Activity*>& vec = queue->getVector();
    //     for ( int j = 0; j < vec.size(); j++ ) {
    //         Event* ev = static_cast<Event*>(vec[j]);
    //         auto link = link_map.find(ev->getLinkId());
    //         if (link == link_map.end()) {
    //             printf("Link not found in map!\n");
    //             abort();
    //         } else {
    //             SimTime_t delay = ev->getDeliveryTime() - sim->getCurrentSimCycle();
    //             link->second->send(delay,ev);
    //         }
    //     }
    //     queue->clear();
    // }

    // // Use this nextSyncTime computation for no skip
    // // nextSyncTime = sim->getCurrentSimCycle() + max_period;


    // // Use this nextSyncTime computation for skipping
    // totalWaitTime += barrier.wait();

    // // if ( thread == 0 ) localMinimumNextActivityTime = sim->getLocalMinimumNextActivityTime();
    // // totalWaitTime += barrier.wait();
    // // nextSyncTime = localMinimumNextActivityTime + max_period;
    // nextSyncTime = sim->getLocalMinimumNextActivityTime() + max_period;
    
}

void
ThreadSyncSimpleSkip::processLinkInitData()
{
    // TraceFunction trace(CALL_INFO_LONG);
    // Need to walk through all the queues and send the data to the
    // correct links
    for ( int i = 0; i < num_threads; i++ ) {
        ThreadSyncQueue* queue = queues[i];
        std::vector<Activity*>& vec = queue->getVector();
        for ( int j = 0; j < vec.size(); j++ ) {
            Event* ev = static_cast<Event*>(vec[j]);
            auto link = link_map.find(ev->getLinkId());
            if (link == link_map.end()) {
                printf("Link not found in map!\n");
                abort();
            } else {
                // link->second->sendInitData_sync(ev);
                sendInitData_sync(link->second,ev);
            }
        }
        queue->clear();
    }
}

void
ThreadSyncSimpleSkip::finalizeLinkConfigurations() {
    // TraceFunction trace(CALL_INFO_LONG);
    for (auto i = link_map.begin() ; i != link_map.end() ; ++i) {
        // i->second->finalizeConfiguration();
        finalizeConfiguration(i->second);
    }
}

uint64_t
ThreadSyncSimpleSkip::getDataSize() const {
    // TraceFunction trace(CALL_INFO_LONG);
    size_t count = 0;
    return count;
}


// void
// ThreadSyncSimpleSkip::print(const std::string& header, Output &out) const
// {
//     out.output("%s ThreadSyncSimpleSkip with period %" PRIu64 " to be delivered at %" PRIu64
//                " with priority %d\n",
//                header.c_str(), max_period->getFactor(), getDeliveryTime(), getPriority());
// }


// bool ThreadSyncSimpleSkip::disabled = false;
Core::ThreadSafe::Barrier ThreadSyncSimpleSkip::barrier;

} // namespace SST
