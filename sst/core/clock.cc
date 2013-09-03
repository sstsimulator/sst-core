// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/clock.h"

#include <boost/foreach.hpp>

//#include "sst/core/event.h"
#include "sst/core/debug.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"
    
namespace SST {

Clock::Clock( TimeConverter* period ) :
    Action(),
    currentCycle( 0 ),
    period( period ),
    started(false)
{
    setPriority(40);
} 


Clock::~Clock()
{
    // Delete all the handlers
    for ( HandlerMap_t::iterator it = handlerMap.begin(); it != handlerMap.end(); ++it ) {
	delete *it;
    }
    handlerMap.clear();
}


bool Clock::registerHandler( Clock::HandlerBase* handler )
{
    _CLE_DBG("handler %p\n",handler);
//     if ( started )
//     	handlerMap.push_back( handler );
//     else
    	staticHandlerMap.push_back( handler );

    // // if ( !started )
    // staticHandlerMap.push_back( handler );
    // // handlerMap.push_back( handler );

    return 0;
}


bool Clock::unregisterHandler( Clock::HandlerBase* handler, bool& empty )
{
    _CLE_DBG("handler %p\n",handler);

    HandlerMap_t::iterator iter = handlerMap.begin();

    for ( ; iter != handlerMap.end(); iter++ ) {
        if ( *iter == handler ) {
            _CLE_DBG("erase handler %p\n",handler);
            handlerMap.erase( iter );
            break;
        }
    }
  
    empty = handlerMap.empty();  
    
    return 0;
}

void Clock::execute( void ) {
    Simulation *sim = Simulation::getSimulation();
    
    if ( !started ) started = true;
    if ( handlerMap.empty() && staticHandlerMap.empty() ) 
    {
        return;
    } 

    // Derive the current cycle from the core time
    currentCycle = period->convertFromCoreTime(sim->getCurrentSimCycle());
    
//     HandlerMap_t::iterator op_iter, curr;
//     for ( op_iter = handlerMap.begin(); op_iter != handlerMap.end();  ) {
//     	curr = op_iter++;
//     	Clock::HandlerBase* handler = *curr;
//     	if ( (*handler)(currentCycle) ) handlerMap.erase(curr);
//     }

    StaticHandlerMap_t::iterator sop_iter,start_iter,stop_iter;
    //bool group = false;	//Scoggin(Jan23,2013) fix unused varialble warning in build
    for ( sop_iter = staticHandlerMap.begin(); sop_iter != staticHandlerMap.end();  ) {
    	Clock::HandlerBase* handler = *sop_iter;
    	if ( (*handler)(currentCycle) ) sop_iter = staticHandlerMap.erase(sop_iter);
    	else ++sop_iter;
    	// (*handler)(currentCycle);
    	// ++sop_iter;
    }

    // for ( sop_iter = staticHandlerMap.begin(); sop_iter != staticHandlerMap.end();  ) {
    // 	Clock::HandlerBase* handler = *sop_iter;
    // 	if ( (*handler)(currentCycle) ) {
    // 	    if (!group) {
    // 		group = true;
    // 		start_iter = sop_iter;
    // 	    }
    // 	}
    // 	else {
    // 	    if ( group ) {
    // 		sop_iter = staticHandlerMap.erase(start_iter,sop_iter);
    // 		group = false;
    // 	    }
    // 	}
    // 	++sop_iter;
    // }
    // if ( group ) {
    // 	staticHandlerMap.erase(start_iter,staticHandlerMap.end());
    // 	group = false;
    // }

    SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
    _CLE_DBG( "all called next %lu\n", (unsigned long) next );
    sim->insertActivity( next, this );
    
    return;
}

} // namespace SST

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Clock);

