// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/clock.h"

#include "sst/core/factory.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeConverter.h"

namespace SST {

Clock::HandlerBase::HandlerBase()
{
    handler_id = Factory::getFactory()->CreateClockHandlerId();
}

Clock::Clock(TimeConverter* period, int priority) : Action(), currentCycle(0), period(period), scheduled(false)
{
    // HandlerBase::handler_id =
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

bool
Clock::registerHandler(Clock::HandlerBase* handler)
{
    staticHandlerMap.push_back(handler);
    if ( !scheduled ) { schedule(); }
    return 0;
}

bool
Clock::unregisterHandler(Clock::HandlerBase* handler, bool& empty)
{

    StaticHandlerMap_t::iterator iter = staticHandlerMap.begin();

    for ( ; iter != staticHandlerMap.end(); iter++ ) {
        if ( *iter == handler ) {
            staticHandlerMap.erase(iter);
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

void
Clock::execute(void)
{
    Simulation_impl* sim = Simulation_impl::getSimulation();

    if ( staticHandlerMap.empty() ) {
        // std::cout << "Not rescheduling clock" << std::endl;
        scheduled = false;
        return;
    }

    // Derive the current cycle from the core time
    // currentCycle = period->convertFromCoreTime(sim->getCurrentSimCycle());
    currentCycle++;

    StaticHandlerMap_t::iterator sop_iter;
    for ( sop_iter = staticHandlerMap.begin(); sop_iter != staticHandlerMap.end(); ) {
        Clock::HandlerBase* handler = *sop_iter;

#ifdef SST_CLOCK_PROFILING
#ifdef SST_HIGH_RESOLUTION_CLOCK
        auto start = std::chrono::high_resolution_clock::now();
#else
        struct timeval clockStart, clockEnd, clockDiff;
        gettimeofday(&clockStart, NULL);
#endif
#endif

        if ( (*handler)(currentCycle) )
            sop_iter = staticHandlerMap.erase(sop_iter);
        else
            ++sop_iter;

#ifdef SST_CLOCK_PROFILING
#ifdef SST_HIGH_RESOLUTION_CLOCK
        auto finish = std::chrono::high_resolution_clock::now();
#else
        gettimeofday(&clockEnd, NULL);
        timersub(&clockEnd, &clockStart, &clockDiff);
#endif

        auto it = sim->clockHandlers.find(handler->GetId());

#ifdef SST_HIGH_RESOLUTION_CLOCK
        it->second += std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
#else
        it->second += clockDiff.tv_usec + clockDiff.tv_sec * 1e6;
#endif

        auto iter = sim->clockCounters.find(handler->GetId());
        if ( iter != sim->clockCounters.end() ) { iter->second++; }
#endif

        // (*handler)(currentCycle);
        // ++sop_iter;
    }

    next = sim->getCurrentSimCycle() + period->getFactor();
    sim->insertActivity(next, this);

    return;
}

void
Clock::schedule()
{
    Simulation_impl* sim = Simulation_impl::getSimulation();
    currentCycle         = sim->getCurrentSimCycle() / period->getFactor();
    SimTime_t next       = (currentCycle * period->getFactor()) + period->getFactor();

    // Check to see if we need to insert clock into queue at current
    // simtime.  This happens if the clock would have fired at this
    // tick and if the current priority is less than my priority.
    // However, if we are at time = 0, then we always go out to the
    // next cycle;
    if ( sim->getCurrentPriority() < getPriority() && sim->getCurrentSimCycle() != 0 ) {
        if ( sim->getCurrentSimCycle() % period->getFactor() == 0 ) { next = sim->getCurrentSimCycle(); }
    }

    // std::cout << "Scheduling clock " << period->getFactor() << " at cycle " << next << " current cycle is " <<
    // sim->getCurrentSimCycle() << std::endl;
    sim->insertActivity(next, this);
    scheduled = true;
}

void
Clock::print(const std::string& header, Output& out) const
{
    out.output(
        "%s Clock Activity with period %" PRIu64 " to be delivered at %" PRIu64
        " with priority %d, with %d items on clock list\n",
        header.c_str(), period->getFactor(), getDeliveryTime(), getPriority(), (int)staticHandlerMap.size());
}

} // namespace SST
