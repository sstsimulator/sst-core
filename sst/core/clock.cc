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

#include <boost/foreach.hpp>

#include "sst/core/clock.h"
#include "sst/core/timeConverter.h"
#include "sst/core/event.h"
#include "sst/core/simulation.h"
    
namespace SST {

Clock::Clock( TimeConverter* period ) :
    Action(),
    currentCycle(0),
    period( period )
{
    setPriority(40);
} 


bool Clock::HandlerRegister( Clock::HandlerBase* handler )
{
    _CLE_DBG("handler %p\n",handler);
    handlerMap.push_back( handler );


//     std::pair<bool,Clock::HandlerBase*> tmp;
//     tmp.first = true;
//     tmp.second = handler;
//     handlerMap.push_back( tmp );

    return 0;
}

    bool Clock::HandlerUnregister( Clock::HandlerBase* handler, bool& empty )
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

// bool Clock::handler( Event* event ) {
void Clock::execute( void ) {
    Simulation *sim = Simulation::getSimulation();
    
    _CLE_DBG("time=FIXME cycle=%lu epoch=FIXME\n", (unsigned long) currentCycle );

    if ( handlerMap.empty() ) 
    {
        return;
    } 

    // Derive the current cycle from the core time
    currentCycle = period->convertFromCoreTime(sim->getCurrentSimCycle());
    
//     BOOST_FOREACH( Clock::HandlerBase* c, handlerMap ) {
//         ( *c )( currentCycle );
//     }

    HandlerMap_t::iterator op_iter, curr;
    for ( op_iter = handlerMap.begin(); op_iter != handlerMap.end();  ) {
	curr = op_iter++;
	Clock::HandlerBase* handler = *curr;
	if ( (*handler)(currentCycle) ) handlerMap.erase(curr);
    }

//     for ( int i = 0; i < handlerMap.size(); i++ ) {
// 	if (handlerMap[i].first) {
// 	    handlerMap[i].first = !( (*handlerMap[i].second)(currentCycle) );
// 	}
//     }

    SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
    _CLE_DBG( "all called next %lu\n", (unsigned long) next );
    sim->insertActivity( next, this );
    
    return;
}

} // namespace SST

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Clock);

