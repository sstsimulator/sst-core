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


#include <sst_config.h>

#include <utility>

#include <sst/core/link.h>
#include <sst/core/simulation.h>

namespace SST { 

Link::Link( Event::Handler_t* functor ) :
    sendQueue( NULL),
    rFunctor( functor ),
    sFunctor( NULL ),
    m_syncQueue( NULL ),
    defaultTimeBase( NULL ),
    latency(1)
{
    if ( functor ) {
	recvQueue = Simulation::getSimulation()->getEventQueue();
	type = HANDLER;
    } else {
	recvQueue = new EventQueue_t;
	type = DIRECT;
    }

    _LINK_DBG("this=%p functor=%p recvQueue=%p type=%d\n", 
                        this, functor, recvQueue, type );
} 

Link::~Link() {
    if ( type == DIRECT ) {
	delete recvQueue;
    }
}
    
void Link::setLatency(Cycle_t lat) {
    latency = lat;
}
    
void Link::Connect( Link *link, Cycle_t lat ) {
    if ( ! link ) {
        _abort(Link,"NULL link\n");
    }
    sendQueue = const_cast<EventQueue_t*>(link->recvQueue);
    sFunctor = const_cast<Event::Handler_t*>(link->rFunctor);
    latency = lat;
    _LINK_DBG("this=%p sendQueue=%p\n", this, sendQueue);
}

void Link::Connect( CompEventQueue_t* queue, Cycle_t lat ) {
    _LINK_DBG("this=%p sendQueue=%p \n", this, queue );
    m_syncQueue = queue;
    latency = lat;
}
    
void Link::Send( SimTime_t delay, TimeConverter* tc, CompEvent* event ) {
    _LINK_DBG("delay=%lu sendQueue=%p event=%p sFunctor=%p\n",
                    delay,sendQueue,event,sFunctor);
    
    if ( tc == NULL ) {
	_abort(Link,"Cannot send an event on Link with NULL TimeConverter\n");
    }

    Cycle_t cycle = Simulation::getSimulation()->getCurrentSimCycle() +
                    tc->convertToCoreTime(delay) + latency;

    _LINK_DBG( "cycle=%lu\n", cycle );

    if ( m_syncQueue ) {
        _LINK_DBG("Sync %p\n", m_syncLink);
        event->SetLinkPtr( m_syncLink );
        event->SetCycle( cycle );
        m_syncQueue->push_back( event ); 
    } else {
        std::pair<EventHandlerBase<bool,Event*>*,Event*> envelope;
        envelope.first = sFunctor;
        envelope.second = event;
        sendQueue->insert( cycle, envelope );
    }
}
    
void Link::SyncInsert( Cycle_t cycle, CompEvent* event )
{ 
    _LINK_DBG("%p cycle=%lu\n",this,cycle);
    _LINK_DBG("sFunctor=%p\n",sFunctor);
    _LINK_DBG("recvQ=%p\n",recvQueue);
    
    std::pair<EventHandlerBase<bool,Event*>*,Event*> envelope;
    envelope.first = rFunctor;
    envelope.second = event;
    recvQueue->insert( cycle, envelope );
}
    

CompEvent* Link::Recv()
{
    Event* event = NULL;
    Simulation *simulation = Simulation::getSimulation();

    if ( !recvQueue->empty() ) {
	_LINK_DBG("key=%lu current=%lu\n",recvQueue->key(),
		  simulation->getCurrentSimCycle());
	if ( recvQueue->key() <=  simulation->getCurrentSimCycle() ) {
	    event = recvQueue->top().second;
	    recvQueue->pop();
	}
    }
    return static_cast<CompEvent*>(event);
} 
    
void Link::setDefaultTimeBase(TimeConverter* tc) {
  defaultTimeBase = tc;
}
    
} // namespace SST

