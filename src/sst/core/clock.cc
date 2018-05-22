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
#include "sst/core/clock.h"

//#include "sst/core/event.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"
    
namespace SST {

Clock::Clock( TimeConverter* period, int priority ) :
    Action(),
    currentCycle( 0 ),
    period( period ),
    scheduled( false )
{
    setPriority(priority);
} 


Clock::~Clock()
{
    // Delete all the handlers
    for ( StaticHandlerMap_t::iterator it = staticHandlerMap.begin(); it != staticHandlerMap.end(); ++it ) {
        delete *it;
    }
    staticHandlerMap.clear();
}


bool Clock::registerHandler( Clock::HandlerBase* handler )
{
    staticHandlerMap.push_back( handler );	
    if ( !scheduled ) {
        schedule();
    }
    return 0;
}


bool Clock::unregisterHandler( Clock::HandlerBase* handler, bool& empty )
{

    StaticHandlerMap_t::iterator iter = staticHandlerMap.begin();

    for ( ; iter != staticHandlerMap.end(); iter++ ) {
        if ( *iter == handler ) {
            staticHandlerMap.erase( iter );
            break;
        }
    }
  
    empty = staticHandlerMap.empty();
    
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
    
    if ( staticHandlerMap.empty() ) {
        // std::cout << "Not rescheduling clock" << std::endl;
        scheduled = false;
        return;
    } 
    
    // Derive the current cycle from the core time
    // currentCycle = period->convertFromCoreTime(sim->getCurrentSimCycle());
    currentCycle++;
    
    StaticHandlerMap_t::iterator sop_iter,start_iter,stop_iter;
    //bool group = false;	//Scoggin(Jan23,2015) fix unused variable warning in build
    for ( sop_iter = staticHandlerMap.begin(); sop_iter != staticHandlerMap.end();  ) {
    	Clock::HandlerBase* handler = *sop_iter;
    	if ( (*handler)(currentCycle) ) sop_iter = staticHandlerMap.erase(sop_iter);
    	else ++sop_iter;
    	// (*handler)(currentCycle);
    	// ++sop_iter;
    }
    
    next = sim->getCurrentSimCycle() + period->getFactor();
    sim->insertActivity( next, this );

    return;
}

void
Clock::schedule()
{
    Simulation* sim = Simulation::getSimulation();
    currentCycle = sim->getCurrentSimCycle() / period->getFactor();
    SimTime_t next = (currentCycle * period->getFactor()) + period->getFactor();

    // Check to see if we need to insert clock into queue at current
    // simtime.  This happens if the clock would have fired at this
    // tick and if the current priority is less than my priority.
    // However, if we are at time = 0, then we always go out to the
    // next cycle;
    if ( sim->getCurrentPriority() < getPriority() && sim->getCurrentSimCycle() != 0 ) {
        if ( sim->getCurrentSimCycle() % period->getFactor() == 0 ) {
            next = sim->getCurrentSimCycle();
        }
    }

    // std::cout << "Scheduling clock " << period->getFactor() << " at cycle " << next << " current cycle is " << sim->getCurrentSimCycle() << std::endl;
    sim->insertActivity(next, this);
    scheduled = true;
}

void
Clock::print(const std::string& header, Output &out) const
{
    out.output("%s Clock Activity with period %" PRIu64 " to be delivered at %" PRIu64
               " with priority %d, with %d items on clock list\n",
               header.c_str(), period->getFactor(), getDeliveryTime(), getPriority(),
               (int)staticHandlerMap.size());
}

} // namespace SST

