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

    if ( sim->getRank().thread == 0 ) {
        barrier[0].resize(num_threads);
        barrier[1].resize(num_threads);
        barrier[2].resize(num_threads);
    }

    if ( sim->getNumRanks().rank > 1 ) single_rank = false;
    else single_rank = true;

    my_max_period = sim->getInterThreadMinLatency();
    nextSyncTime = my_max_period;
}

ThreadSyncSimpleSkip::~ThreadSyncSimpleSkip()
{
    if ( totalWaitTime > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0, "ThreadSyncSimpleSkip total wait time: %lg seconds.\n", totalWaitTime);
    for ( int i = 0; i < num_threads; i++ ) {
        delete queues[i];
    }
    queues.clear();
}

void
ThreadSyncSimpleSkip::registerLink(LinkId_t link_id, Link* link)
{
    link_map[link_id] = link;
}

ActivityQueue*
ThreadSyncSimpleSkip::getQueueForThread(int tid)
{
    return queues[tid];
}

void
ThreadSyncSimpleSkip::before()
{
    // Empty all the queues and send events on the links
    for ( size_t i = 0; i < queues.size(); i++ ) {
        ThreadSyncQueue* queue = queues[i];
        std::vector<Activity*>& vec = queue->getVector();
        for ( size_t j = 0; j < vec.size(); j++ ) {
            Event* ev = static_cast<Event*>(vec[j]);
            auto link = link_map.find(ev->getLinkId());
            if (link == link_map.end()) {
                Simulation::getSimulationOutput().fatal(CALL_INFO,1,"Link not found in map!\n");
            } else {
                SimTime_t delay = ev->getDeliveryTime() - sim->getCurrentSimCycle();
                link->second->send(delay,ev);
            }
        }
        queue->clear();
    }
}

void
ThreadSyncSimpleSkip::after()
{
    // Use this nextSyncTime computation for no skip
    // nextSyncTime = sim->getCurrentSimCycle() + max_period;

    // Use this nextSyncTime computation for skipping

    auto nextmin = sim->getLocalMinimumNextActivityTime();
    auto nextminPlus = nextmin + my_max_period;
    nextSyncTime = nextmin > nextminPlus ? nextmin : nextminPlus;
}

void
ThreadSyncSimpleSkip::execute()
{
    totalWaitTime = barrier[0].wait();
    before();
    totalWaitTime = barrier[1].wait();
    after();
    totalWaitTime += barrier[2].wait();
}

void
ThreadSyncSimpleSkip::processLinkUntimedData()
{
    // Need to walk through all the queues and send the data to the
    // correct links
    for ( int i = 0; i < num_threads; i++ ) {
        ThreadSyncQueue* queue = queues[i];
        std::vector<Activity*>& vec = queue->getVector();
        for ( size_t j = 0; j < vec.size(); j++ ) {
            Event* ev = static_cast<Event*>(vec[j]);
            auto link = link_map.find(ev->getLinkId());
            if (link == link_map.end()) {
                Simulation::getSimulationOutput().fatal(CALL_INFO,1,"Link not found in map!\n");
            } else {
                sendUntimedData_sync(link->second,ev);
            }
        }
        queue->clear();
    }
}

void
ThreadSyncSimpleSkip::finalizeLinkConfigurations() {
    for (auto i = link_map.begin() ; i != link_map.end() ; ++i) {
        finalizeConfiguration(i->second);
    }
}

void
ThreadSyncSimpleSkip::prepareForComplete() {
    for (auto i = link_map.begin() ; i != link_map.end() ; ++i) {
        prepareForCompleteInt(i->second);
    }
}

uint64_t
ThreadSyncSimpleSkip::getDataSize() const {
    size_t count = 0;
    return count;
}


Core::ThreadSafe::Barrier ThreadSyncSimpleSkip::barrier[3];

} // namespace SST
