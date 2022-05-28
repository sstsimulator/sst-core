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

#if SST_EVENT_PROFILING

#define SST_EVENT_PROFILE_REG                                                     \
    auto sim = Simulation_impl::getSimulation();                                  \
    sim->incrementEventCounters(getFirstComponentName(), getLastComponentName()); \
    std::string event_profile_handler { getLastComponentName().c_str() }

#if SST_HIGH_RESOLUTION_CLOCK
#define SST_EVENT_PROFILE_START \
    SST_EVENT_PROFILE_REG;      \
    auto event_profile_start = std::chrono::high_resolution_clock::now()

#define SST_EVENT_PROFILE_STOP                                                                                  \
    auto event_profile_stop = std::chrono::high_resolution_clock::now();                                        \
    auto event_profile_count =                                                                                  \
        std::chrono::duration_cast<std::chrono::nanoseconds>(event_profile_stop - event_profile_start).count(); \
    sim->incrementEventHandlerTime(event_profile_handler, event_profile_count)

#else
#define SST_EVENT_PROFILE_START                                                 \
    SST_EVENT_PROFILE_REG;                                                      \
    struct timeval event_profile_start, event_profile_stop, event_profile_diff; \
    gettimeofday(&event_profile_start, NULL)


#define SST_EVENT_PROFILE_STOP                                                               \
    gettimeofday(&event_profile_stop, NULL);                                                 \
    timersub(&event_profile_stop, &event_profile_start, &event_profile_diff);                \
    auto event_profile_count = event_profile_diff.tv_usec + event_profile_diff.tv_sec * 1e6; \
    sim->incrementEventHandlerTime(event_profile_handler, event_profile_count)

#endif

#else
#define SST_EVENT_PROFILE_START
#define SST_EVENT_PROFILE_STOP
#endif


Event::~Event() {}

void
Event::execute(void)
{
    SST_EVENT_PROFILE_START;

    (*reinterpret_cast<HandlerBase*>(delivery_info))(this);

    SST_EVENT_PROFILE_STOP;
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
