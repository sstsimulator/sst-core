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
#include "sst/core/threadSync.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
//#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"

namespace SST {


/** Create a new ThreadSync object */
ThreadSync::ThreadSync(int num_threads, Simulation* sim) :
    Action(),
    num_threads(num_threads),
    sim(sim),
    totalWaitTime(0.0)
{
    for ( int i = 0; i < num_threads; i++ ) {
        queues.push_back(new ThreadSyncQueue());
    }

    if ( sim->getRank().thread == 0 )
        barrier.resize(num_threads);

    if ( sim->getNumRanks().rank > 1 ) single_rank = false;
    else single_rank = true;
}

ThreadSync::~ThreadSync()
{
    if ( totalWaitTime > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0, "ThreadSync total wait time: %lg seconds.\n", totalWaitTime);
    for ( int i = 0; i < num_threads; i++ ) {
        delete queues[i];
    }
    queues.clear();
}

void
ThreadSync::setMaxPeriod(TimeConverter* period)
{
    max_period = period;
    SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
    setPriority(THREADSYNCPRIORITY);
    sim->insertActivity( next, this );
}

void
ThreadSync::registerLink(LinkId_t link_id, Link* link)
{
    link_map[link_id] = link;
}

ActivityQueue*
ThreadSync::getQueueForThread(int tid)
{
    return queues[tid];
}

void
ThreadSync::execute()
{
    TraceFunction trace(CALL_INFO_LONG);
    totalWaitTime += barrier.wait();
    if ( disabled ) return;
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

    Exit* exit = sim->getExit();
    if ( single_rank && exit->getRefCount() == 0 ) {
        endSimulation(exit->getEndTime());
    }

    totalWaitTime += barrier.wait();

    SimTime_t next = sim->getCurrentSimCycle() + max_period->getFactor();
    sim->insertActivity( next, this );    
}

void
ThreadSync::processLinkUntimedData()
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
                link->second->sendUntimedData_sync(ev);
            }
        }
        queue->clear();
    }
}

void
ThreadSync::finalizeLinkConfigurations() {
    for (auto i = link_map.begin() ; i != link_map.end() ; ++i) {
        i->second->finalizeConfiguration();
    }
}

uint64_t
ThreadSync::getDataSize() const {
    size_t count = 0;
    return count;
}


void
ThreadSync::print(const std::string& header, Output &out) const
{
    out.output("%s ThreadSync with period %" PRIu64 " to be delivered at %" PRIu64
               " with priority %d\n",
               header.c_str(), max_period->getFactor(), getDeliveryTime(), getPriority());
}


bool ThreadSync::disabled = false;
Core::ThreadSafe::Barrier ThreadSync::barrier;

} // namespace SST
