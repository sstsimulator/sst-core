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

#include <sst/clockEvent.h>
#include <sst/timeConverter.h>
#include <boost/foreach.hpp>
#include <sst/compEvent.h>
#include <sst/simulation.h>

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                             SST::ClockEvent, bool, SST::Event* )
#endif
    
namespace SST {

ClockEvent::ClockEvent( TimeConverter* period ) :
    Event(),
//     functor( new EventHandler< ClockEvent, bool, Time_t, Event* >
//                                         ( this, &ClockEvent::handler ) ),
    currentCycle(0),
    period( period )
{
  functor = new EventHandler< ClockEvent, bool, Event* >( this, &ClockEvent::handler );
} 


bool ClockEvent::HandlerRegister( Which_t which, ClockHandler_t* handler )
{
    _CLE_DBG("handler %p\n",handler);
    handlerMap[which].push_back( handler );
    return 0;
}

bool ClockEvent::HandlerUnregister( Which_t which, ClockHandler_t* handler, 
                                                            bool& empty )
{
    _CLE_DBG("handler %p\n",handler);

    HandlerMap_t::iterator iter = handlerMap[which].begin();

    for ( ; iter < handlerMap[which].end(); iter++ ) {
        if ( *iter == handler ) {
            _CLE_DBG("erase handler %p\n",handler);
            handlerMap[which].erase( iter );
            break;
        }
    }
  
    empty = handlerMap[which].empty();  
    
    return 0;
}

bool ClockEvent::handler( Event* event ) {
    Simulation *sim = Simulation::getSimulation();
    
    _CLE_DBG("time=FIXME cycle=%lu epoch=FIXME\n", currentCycle );

    if ( handlerMap[PRE].empty() &&
            handlerMap[DEFAULT].empty() &&
            handlerMap[POST].empty() ) 
    {
        _CLE_DBG( "empty\n" );
        delete event;
        return false;
    } 

    // Derive the current cycle from the core time
    currentCycle = period->convertFromCoreTime(sim->getCurrentSimCycle());
    
    BOOST_FOREACH( ClockHandler_t* c, handlerMap[PRE] ) {
        ( *c )( currentCycle );
    }
    BOOST_FOREACH( ClockHandler_t* c, handlerMap[DEFAULT] ) {
        ( *c )( currentCycle );
    }
    BOOST_FOREACH( ClockHandler_t* c, handlerMap[POST] ) {
        ( *c )( currentCycle );
    }


    SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
    _CLE_DBG( "all called next %lu\n", next );
    sim->insertEvent( next, event, functor );
    
    return false;
}

} // namespace SST
