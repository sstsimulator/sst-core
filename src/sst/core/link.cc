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
#include <sst/core/link.h>

#include <utility>

#include <sst/core/event.h>
#include <sst/core/initQueue.h>
#include <sst/core/pollingLinkQueue.h>
#include <sst/core/simulation.h>
#include <sst/core/timeConverter.h>
#include <sst/core/timeLord.h>
#include <sst/core/timeVortex.h>
//#include <sst/core/syncQueue.h>
#include <sst/core/uninitializedQueue.h>
#include <sst/core/unitAlgebra.h>

namespace SST { 

// ActivityQueue* Link::uninitQueue = NULL;
ActivityQueue* Link::uninitQueue = new UninitializedQueue("ERROR: Trying to send or recv from link during initialization.  Send and Recv cannot be called before setup.");
ActivityQueue* Link::afterInitQueue = new UninitializedQueue("ERROR: Trying to call sendUntimedData/sendInitData or recvUntimedData/recvInitData during the run phase.");
ActivityQueue* Link::afterRunQueue = new UninitializedQueue("ERROR: Trying to call send or recv during complete phase.");
    
Link::Link(LinkId_t id) :
    rFunctor( NULL ),
    defaultTimeBase( NULL ),
    latency(1),
    type(HANDLER),
    id(id)
{
    recvQueue = uninitQueue;
    untimedQueue = NULL;
    configuredQueue = Simulation::getSimulation()->getTimeVortex();
}

Link::Link() :
    rFunctor( NULL ),
    defaultTimeBase( NULL ),
    latency(1),
    type(HANDLER),
    id(-1)
{
    recvQueue = uninitQueue;
    untimedQueue = NULL;
    configuredQueue = Simulation::getSimulation()->getTimeVortex();
}

Link::~Link() {
    if ( type == POLL && recvQueue != uninitQueue && recvQueue != afterInitQueue && recvQueue != afterRunQueue ) {
        delete recvQueue;
    }
    if ( rFunctor != NULL ) delete rFunctor;
}

void Link::finalizeConfiguration() {
    recvQueue = configuredQueue;
    configuredQueue = untimedQueue;
    if ( untimedQueue != NULL ) {
        if ( dynamic_cast<InitQueue*>(untimedQueue) != NULL) {
            delete untimedQueue;
            configuredQueue = NULL;
        }
    }
    untimedQueue = afterInitQueue;
}

void Link::prepareForComplete() {
    if ( type == POLL && recvQueue != uninitQueue && recvQueue != afterInitQueue ) {
        delete recvQueue;
    }
    recvQueue = afterRunQueue;
    untimedQueue = configuredQueue;
}

void Link::setPolling() {
    type = POLL;
    configuredQueue = new PollingLinkQueue();
}

    
void Link::setLatency(Cycle_t lat) {
    latency = lat;
}
    
void Link::addRecvLatency(int cycles, std::string timebase) {
    SimTime_t tb = Simulation::getSimulation()->getTimeLord()->getSimCycles(timebase,"addOutputLatency");
    pair_link->latency += (cycles * tb);
}
    
void Link::addRecvLatency(SimTime_t cycles, TimeConverter* timebase) {
    pair_link->latency += timebase->convertToCoreTime(cycles);
}
    
void Link::send( SimTime_t delay, TimeConverter* tc, Event* event ) {  
    if ( tc == NULL ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Cannot send an event on Link with NULL TimeConverter\n");
    }
    
    Cycle_t cycle = Simulation::getSimulation()->getCurrentSimCycle() +
        tc->convertToCoreTime(delay) + latency;
    
    if ( event == NULL ) {
        event = new NullEvent();
    }
    event->setDeliveryTime(cycle);
    event->setDeliveryLink(id,pair_link);

#if __SST_DEBUG_EVENT_TRACKING__
    event->addSendComponent(comp, ctype, port);
    event->addRecvComponent(pair_link->comp, pair_link->ctype, pair_link->port);
#endif

    // trace.getOutput().output(CALL_INFO, "%p\n",pair_link->recvQueue);
    pair_link->recvQueue->insert( event );
}
    

Event* Link::recv() 
{
    // Check to make sure this is a polling link
    if ( UNLIKELY( type != POLL ) ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Cannot call recv on a Link with an event handler installed (non-polling link.\n");
        
    }
    
    Event* event = NULL;
    Simulation *simulation = Simulation::getSimulation();

    if ( !recvQueue->empty() ) {
	Activity* activity = recvQueue->front();
	if ( activity->getDeliveryTime() <=  simulation->getCurrentSimCycle() ) {
	    event = static_cast<Event*>(activity);
	    recvQueue->pop();
	}
    }
    return event;
} 

void Link::sendUntimedData(Event* data)
{
    if ( pair_link->untimedQueue == NULL ) {
        pair_link->untimedQueue = new InitQueue();
    }
    Simulation::getSimulation()->untimed_msg_count++;
    data->setDeliveryTime(Simulation::getSimulation()->untimed_phase + 1);
    data->setDeliveryLink(id,pair_link);
    
    pair_link->untimedQueue->insert(data);
#if __SST_DEBUG_EVENT_TRACKING__
    data->addSendComponent(comp,ctype,port);
    data->addRecvComponent(pair_link->comp, pair_link->ctype, pair_link->port);
#endif
}

void Link::sendUntimedData_sync(Event* data)
{
    if ( pair_link->untimedQueue == NULL ) {
        pair_link->untimedQueue = new InitQueue();
    }
    // data->setDeliveryLink(id,pair_link);

    pair_link->untimedQueue->insert(data);
}

Event* Link::recvUntimedData()
{
    if ( untimedQueue == NULL ) return NULL;

    Event* event = NULL;
    if ( !untimedQueue->empty() ) {
	Activity* activity = untimedQueue->front();
	if ( activity->getDeliveryTime() <= Simulation::getSimulation()->untimed_phase ) {
	    event = static_cast<Event*>(activity);
	    untimedQueue->pop();
	}
    }
    return event;
}

void Link::setDefaultTimeBase(TimeConverter* tc) {
    defaultTimeBase = tc;
}

TimeConverter* Link::getDefaultTimeBase() {
    return defaultTimeBase;
}


} // namespace SST
