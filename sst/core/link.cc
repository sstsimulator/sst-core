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
#include <sst/core/link.h>

#include <utility>

#include <sst/core/event.h>
#include <sst/core/debug.h>
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
    recvQueue = configuredQueue;
    configuredQueue = NULL;
    if ( initQueue != NULL ) {
	if ( dynamic_cast<InitQueue*>(initQueue) != NULL) {
	    delete initQueue;
	}
    }
    initQueue = afterInitQueue;
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
        _abort(Link,"Cannot send an event on Link with NULL TimeConverter\n");
    }
    
    Cycle_t cycle = Simulation::getSimulation()->getCurrentSimCycle() +
        tc->convertToCoreTime(delay) + latency;
    
    _LINK_DBG( "cycle=%lu\n", (unsigned long)cycle );
    
    if ( event == NULL ) {
        event = new NullEvent();
    }
    event->setDeliveryTime(cycle);
    event->setDeliveryLink(id,pair_link);

#if __SST_DEBUG_EVENT_TRACKING__
    event->addSendComponent(comp, ctype, port);
    event->addRecvComponent(pair_link->comp, pair_link->ctype, pair_link->port);
#endif
        
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


template<class Archive>
void
Link::serialize(Archive & ar, const unsigned int version)
{
    std::string type;

    printf("begin Link::serialize\n");
    printf("  - Link::recvQueue\n");
    ar & BOOST_SERIALIZATION_NVP( recvQueue );
    printf("  - Link::initQueue\n");
    ar & BOOST_SERIALIZATION_NVP( initQueue );
    printf("  - Link::configuredQueue\n");
    ar & BOOST_SERIALIZATION_NVP( configuredQueue );
    // don't serialize rFunctor
    printf("  - Link::defaultTimeBase\n");
    ar & BOOST_SERIALIZATION_NVP( defaultTimeBase );
    printf("  - Link::latency\n");
    ar & BOOST_SERIALIZATION_NVP( latency );
    printf("  - Link::pair_link\n");
    ar & BOOST_SERIALIZATION_NVP(pair_link);
    printf("  - Link::type\n");
    ar & BOOST_SERIALIZATION_NVP( type );
    printf("  - Link::id\n");
    ar & BOOST_SERIALIZATION_NVP(id);
    printf("end Link::serialize\n");
}


template<class Archive>
void
SelfLink::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Link);
}
    
} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Link::serialize)
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::SelfLink::serialize)

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Link)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::SelfLink)
