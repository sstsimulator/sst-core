// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization/core.h"

#include <utility>

#include <sst/core/link.h>
#include <sst/core/simulation.h>
#include <sst/core/event.h>
#include <sst/core/pollingLinkQueue.h>
#include <sst/core/timeVortex.h>
#include <sst/core/timeLord.h>
#include <sst/core/syncQueue.h>

namespace SST { 

Link::Link(LinkId_t id) :
    rFunctor( NULL ),
    defaultTimeBase( NULL ),
    latency(1),
    type(HANDLER),
    id(id)
{
    recvQueue = Simulation::getSimulation()->getTimeVortex();
}

Link::Link() :
    rFunctor( NULL ),
    defaultTimeBase( NULL ),
    latency(1),
    type(HANDLER),
    id(-1)
{
    recvQueue = Simulation::getSimulation()->getTimeVortex();
}

Link::~Link() {
    if ( type == POLL ) {
	delete recvQueue;
    }
    if ( rFunctor != NULL ) delete rFunctor;
}

void Link::setPolling() {
    type = POLL;
    recvQueue = new PollingLinkQueue();
}

    
void Link::setLatency(Cycle_t lat) {
    latency = lat;
}
    
void Link::addOutputLatency(int cycles, std::string timebase) {
    SimTime_t tb = Simulation::getSimulation()->getTimeLord()->getSimCycles(timebase,"addOutputLatency");
    latency += (cycles * tb);
}
    
void Link::addOutputLatency(SimTime_t cycles, TimeConverter* timebase) {
    latency += timebase->convertToCoreTime(cycles);
}
    
void Link::Send( SimTime_t delay, TimeConverter* tc, Event* event ) {
//     _LINK_DBG("delay=%lu sendQueue=%p event=%p sFunctor=%p\n",
//               (unsigned long) delay,sendQueue,event,sFunctor);
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
    
    pair_link->recvQueue->insert( event );
}
    

Event* Link::Recv()
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
