// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
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
    period( period )
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
    staticHandlerMap.push_back( handler );	
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

Cycle_t
Clock::getNextCycle()
{
    return currentCycle + 1;
    // return period->convertFromCoreTime(next);
}
    
void Clock::execute( void ) {
    Simulation *sim = Simulation::getSimulation();
    
    if ( handlerMap.empty() && staticHandlerMap.empty() ) 
    {
        // return;
    } 

    // Derive the current cycle from the core time
    // currentCycle = period->convertFromCoreTime(sim->getCurrentSimCycle());
    currentCycle++;
    
    StaticHandlerMap_t::iterator sop_iter,start_iter,stop_iter;
    //bool group = false;	//Scoggin(Jan23,2014) fix unused varialble warning in build
    for ( sop_iter = staticHandlerMap.begin(); sop_iter != staticHandlerMap.end();  ) {
    	Clock::HandlerBase* handler = *sop_iter;
    	if ( (*handler)(currentCycle) ) sop_iter = staticHandlerMap.erase(sop_iter);
    	else ++sop_iter;
    	// (*handler)(currentCycle);
    	// ++sop_iter;
    }

    next = sim->getCurrentSimCycle() + period->getFactor();
    _CLE_DBG( "all called next %lu\n", (unsigned long) next );
    sim->insertActivity( next, this );
    
    return;
}

} // namespace SST

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Clock);

