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

ActivityQueue* Link::uninitQueue = NULL;
ActivityQueue* Link::afterInitQueue = NULL;
    
Link::Link(LinkId_t id) :
    rFunctor( NULL ),
    defaultTimeBase( NULL ),
    latency(1),
    type(HANDLER),
    id(id)
{
    if ( uninitQueue == NULL )
	uninitQueue = new UninitializedQueue("ERROR: Trying to send or recv from link during initialization.  Send and Recv cannot be called before setup.");
    if ( afterInitQueue == NULL )
	afterInitQueue = new UninitializedQueue("ERROR: Trying to call sendInitData or recvInitData after initialziation phase.");
    
    recvQueue = uninitQueue;
    initQueue = NULL;
    configuredQueue = Simulation::getSimulation()->getTimeVortex();
}

Link::Link() :
    rFunctor( NULL ),
    defaultTimeBase( NULL ),
    latency(1),
    type(HANDLER),
    id(-1)
{
    if ( uninitQueue == NULL )
	uninitQueue = new UninitializedQueue("ERROR: Trying to send or recv from link during initialization.  Send and Recv cannot be called before setup.");
    if ( afterInitQueue == NULL )
	afterInitQueue = new UninitializedQueue("ERROR: Trying to call sendInitData or recvInitData after initialziation phase.");
    
    recvQueue = uninitQueue;
    initQueue = NULL;
    configuredQueue = Simulation::getSimulation()->getTimeVortex();
}

Link::~Link() {
    if ( type == POLL && recvQueue != uninitQueue && recvQueue != afterInitQueue ) {
        delete recvQueue;
    }
    if ( rFunctor != NULL ) delete rFunctor;
}

void Link::finalizeConfiguration() {
    // TraceFunction trace (CALL_INFO_LONG);
    recvQueue = configuredQueue;
    configuredQueue = NULL;
    if ( initQueue != NULL ) {
	if ( dynamic_cast<InitQueue*>(initQueue) != NULL) {
	    delete initQueue;
	}
    }
    initQueue = afterInitQueue;
    // trace.getOutput().output(CALL_INFO,"id: %ld: recvQueue = %p, initQueue = %p\n",id,recvQueue,initQueue);
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
    // TraceFunction trace(CALL_INFO_LONG);
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

void Link::sendInitData(Event* init_data)
{
    if ( pair_link->initQueue == NULL ) {
        pair_link->initQueue = new InitQueue();
    }
    Simulation::getSimulation()->init_msg_count++;
    init_data->setDeliveryTime(Simulation::getSimulation()->init_phase + 1);
    init_data->setDeliveryLink(id,pair_link);
    
    pair_link->initQueue->insert(init_data);
#if __SST_DEBUG_EVENT_TRACKING__
    init_data->addSendComponent(comp,ctype,port);
    init_data->addRecvComponent(pair_link->comp, pair_link->ctype, pair_link->port);
#endif
}

void Link::sendInitData_sync(Event* init_data)
{
    if ( pair_link->initQueue == NULL ) {
        pair_link->initQueue = new InitQueue();
    }
    // init_data->setDeliveryLink(id,pair_link);

    pair_link->initQueue->insert(init_data);
}

Event* Link::recvInitData()
{
    if ( initQueue == NULL ) return NULL;

    Event* event = NULL;
    if ( !initQueue->empty() ) {
	Activity* activity = initQueue->front();
	if ( activity->getDeliveryTime() <= Simulation::getSimulation()->init_phase ) {
	    event = static_cast<Event*>(activity);
	    initQueue->pop();
	}
    }
    return event;
}

// UnitAlgebra
// Link::getTotalInputLatency()
// {
    
// }
    
// UnitAlgebra
// Link::getTotalOutputLatency()
// {
    
// }
    
void Link::setDefaultTimeBase(TimeConverter* tc) {
    defaultTimeBase = tc;
}

TimeConverter* Link::getDefaultTimeBase() {
    return defaultTimeBase;
}


} // namespace SST
