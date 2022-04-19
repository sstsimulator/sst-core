// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/event.h"

#include "sst/core/link.h"
#include "sst/core/simulation_impl.h"

#include <sys/time.h>

namespace SST {

std::atomic<uint64_t>     SST::Event::id_counter(0);
const SST::Event::id_type SST::Event::NO_ID = std::make_pair(0, -1);

#if SST_HIGH_RESOLUTION_CLOCK
#define SST_EVENT_PROFILE_START auto event_profile_eventStart = std::chrono::high_resolution_clock::now();
#define SST_EVENT_PROFILE_STOP                                                                                         \
    auto event_profile_eventFinish  = std::chrono::high_resolution_clock::now();                                       \
    auto event_profile_eventHandler = sim->eventHandlers.find(getLastComponentName());                                 \
    if ( event_profile_eventHandler != sim->eventHandlers.end() ) {                                                    \
        event_profile_eventHandler->second +=                                                                          \
            std::chrono::duration_cast<std::chrono::nanoseconds>(event_profile_eventFinish - event_profile_eventStart) \
                .count();                                                                                              \
    }
#else
#define SST_EVENT_PROFILE_START                     \
    struct timeval eventStart, eventEnd, eventDiff; \
    gettimeofday(&eventStart, NULL);
#define SST_EVENT_PROFILE_STOP                                                            \
    gettimeofday(&eventEnd, NULL);                                                        \
    timersub(&eventEnd, &eventStart, &eventDiff);                                         \
    auto event_profile_eventHandler = sim->eventHandlers.find(getLastComponentName());    \
    if ( event_profile_eventHandler != sim->eventHandlers.end() ) {                       \
        event_profile_eventHandler->second += eventDiff.tv_usec + eventDiff.tv_sec * 1e6; \
    }
#endif

Event::~Event() {}

void
Event::execute(void)
{

#if SST_EVENT_PROFILING
    SST_EVENT_PROFILE_START
#endif

        (*reinterpret_cast<HandlerBase*>(delivery_info))
    (this);

#if SST_EVENT_PROFILING
    Simulation_impl* sim = Simulation_impl::getSimulation();

    SST_EVENT_PROFILE_STOP

    // Track sending and receiving counters
    auto eventCount = sim->eventRecvCounters.find(getLastComponentName());
    if ( eventCount != sim->eventRecvCounters.end() ) { eventCount->second++; }
    else {
        if ( getLastComponentName() != "" ) {
            sim->eventRecvCounters.insert(std::pair<std::string, uint64_t>(getLastComponentName(), 1));
            sim->eventHandlers.insert(std::pair<std::string, uint64_t>(getLastComponentName(), 0));
        }
    }
    auto eventSend = sim->eventSendCounters.find(getFirstComponentName());
    if ( eventSend != sim->eventSendCounters.end() ) { eventSend->second++; }
    else {
        // Insert handler and counter for the subcomponent so that all link traffic is monitored
        if ( getFirstComponentName() != "" ) {
            sim->eventSendCounters.insert(std::pair<std::string, uint64_t>(getFirstComponentName(), 1));
            sim->eventHandlers.insert(std::pair<std::string, uint64_t>(getFirstComponentName(), 0));
        }
    }
#endif
}

Event*
Event::clone()
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(
        CALL_INFO, 1,
        "Called clone() on an Event that doesn't"
        " implement it.");
    return nullptr; // Never reached, but gets rid of compiler warning
}

Event::id_type
Event::generateUniqueId()
{
    return std::make_pair(id_counter++, Simulation_impl::getSimulation()->getRank().rank);
}

#ifdef USE_MEMPOOL
std::mutex                        Activity::poolMutex;
std::vector<Activity::PoolInfo_t> Activity::memPools;
#endif

} // namespace SST
