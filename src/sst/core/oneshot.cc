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

#include "sst/core/oneshot.h"

#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"


namespace SST {

OneShot::OneShot(TimeConverter* timeDelay, int priority) :
    Action(),
    m_timeDelay(timeDelay),
    m_scheduled(false)
{
    setPriority(priority);
}

OneShot::~OneShot()
{
    HandlerList_t*        ptrHandlerList;
    OneShot::HandlerBase* handler;

//    std::cout << "OneShot (destructor) #" << m_timeDelay->getFactor() << " Destructor;" << std::endl;

    // For all the entries in the map, find each vector set
    for (HandlerVectorMap_t::iterator m_it = m_HandlerVectorMap.begin(); m_it != m_HandlerVectorMap.end(); ++m_it ) {
        ptrHandlerList = m_it->second;

        // Delete all handlers in the list
        for (HandlerList_t::iterator v_it = ptrHandlerList->begin(); v_it != ptrHandlerList->end(); ++v_it ) {
            handler = *v_it;
            delete handler;
        }
        ptrHandlerList->clear();
        delete ptrHandlerList;
    }
    m_HandlerVectorMap.clear();
}

void OneShot::registerHandler(OneShot::HandlerBase* handler)
{
    SimTime_t      nextEventTime;
    HandlerList_t* ptrHandlerList;

    // Since we have a new handler, schedule the OneShot to callback in the future
    nextEventTime = computeDeliveryTime();

    // Check to see if the nextEventTime is already in our map.  Only
    // need to check the first entry.  Anything else would have for
    // sure been put in at an earlier time, and therefore could not
    // have the same delivery time.
    if ( m_HandlerVectorMap.empty() ||  m_HandlerVectorMap.front().first != nextEventTime ) {
        // HandlerList with the specific nextEventTime not found,
        // create a new one and add it to the map of Vectors
        ptrHandlerList = new HandlerList_t();
        m_HandlerVectorMap.emplace_front(nextEventTime,ptrHandlerList);
    }
    else {
        ptrHandlerList = m_HandlerVectorMap.front().second;
    }

    // Add the handler to the list of handlers
    ptrHandlerList->push_back(handler);

    // Call scheduleOneShot() to make sure we're scheduled
    scheduleOneShot();
}

SimTime_t OneShot::computeDeliveryTime()
{
    // Get current simulation time
    Simulation* sim = Simulation::getSimulation();

    // Figure out what the next time should be for when the OneShot should fire
    SimTime_t nextEventTime = sim->getCurrentSimCycle() + m_timeDelay->getFactor();
    return nextEventTime;
}

void OneShot::scheduleOneShot()
{
    // If we aren't scheduled, put ourself in the event queue.  We
    // schedule based on the oldest entry in queue.
    if ( !m_scheduled && !m_HandlerVectorMap.empty() ) {
        // Add this one shot to the Activity queue, and mark this OneShot at scheduled
        Simulation::getSimulation()->insertActivity(m_HandlerVectorMap.back().first, this);
        m_scheduled = true;
    }
}

void OneShot::execute(void)
{
    // Execute the OneShot when the TimeVortex tells us to go.
    // This will call all registered callbacks.
    OneShot::HandlerBase* handler;
    HandlerList_t*        ptrHandlerList;

    // Figure out the current sim time
    SimTime_t currentEventTime = Simulation::getSimulation()->getCurrentSimCycle();

    if (m_HandlerVectorMap.back().first != currentEventTime ) {
        // This shouldn't happen, but if we're not at the right time,
        // then simply reschedule.
        scheduleOneShot();
        return;
    }

    // Get the List of Handlers for this time
    ptrHandlerList = m_HandlerVectorMap.back().second;

    // Walk the list of all handlers, and call them.
    for (HandlerList_t::iterator it = ptrHandlerList->begin(); it != ptrHandlerList->end(); ++it ) {
        handler = *it;

        // Call the registered Callback handlers
        ((*handler)());
    }

    // Delete the Handler list and remove it from the map
    delete ptrHandlerList;
    m_HandlerVectorMap.pop_back();
    m_scheduled = false;

    // Reschedule if there are any events left
    scheduleOneShot();
}

void OneShot::print(const std::string& header, Output &out) const
{
    out.output("%s OneShot Activity with time delay of %" PRIu64 " to be delivered at %" PRIu64
               " with priority %d\n",
               header.c_str(), m_timeDelay->getFactor(), getDeliveryTime(), getPriority());
}

} // namespace SST
