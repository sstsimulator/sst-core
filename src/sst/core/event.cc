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
#include "sst/core/event.h"
#include "sst/core/simulation.h"

#include "sst/core/link.h"

namespace SST {


std::atomic<uint64_t> SST::Event::id_counter(0);
const SST::Event::id_type SST::Event::NO_ID = std::make_pair(0, -1);

Event::~Event() {}

extern std::map<std::string, uint64_t> eventHandlers;
extern std::map<std::string, uint64_t> eventSendCounters;
extern std::map<std::string, uint64_t> eventRecvCounters;

void Event::execute(void)
{

    #ifdef EVENT_PROFILING
    #ifdef HIGH_RESOLUTION_CLOCK
    auto eventStart = std::chrono::high_resolution_clock::now();
    #else
    struct timeval eventStart, eventEnd, eventDiff;
    gettimeofday(&eventStart, NULL);
    #endif
    #endif

    delivery_link->deliverEvent(this);

    #ifdef EVENT_PROFILING
    #ifdef HIGH_RESOLUTION_CLOCK
    auto eventFinish = std::chrono::high_resolution_clock::now();
    #else
    gettimeofday(&eventEnd, NULL);
    timersub(&eventEnd, &eventStart, &eventDiff);
    #endif

    // Track receiver processing time
    auto eventHandler = eventHandlers.find(getLastComponentName());
    if(eventHandler != eventHandlers.end())
    {
        #ifdef HIGH_RESOLUTION_CLOCK
        eventHandler->second += std::chrono::duration_cast<std::chrono::nanoseconds>(eventFinish - eventStart).count();
        #else
        eventHandler->second += eventDiff.tv_usec + eventDiff.tv_sec * 1e6;
        #endif
    }

    // Track sending and receiving counters
    auto eventCount = eventRecvCounters.find(getLastComponentName());
    if(eventCount != eventRecvCounters.end())
    {
        eventCount->second ++;
    }else{
        if(getLastComponentName() != "")
        {
            eventRecvCounters.insert(std::pair<std::string, uint64_t>(getLastComponentName().c_str(), 1));
            eventHandlers.insert(std::pair<std::string, uint64_t>(getLastComponentName().c_str(), 0));
        }
    }
    auto eventSend = eventSendCounters.find(getFirstComponentName());
    if(eventSend != eventSendCounters.end())
    {
       eventSend->second++;
    }else{
        // Insert handler and counter for the subcomponent so that all link traffic is monitored
        if(getFirstComponentName() != "")
        {
            eventSendCounters.insert(std::pair<std::string, uint64_t>(getFirstComponentName().c_str(),1));
            eventHandlers.insert(std::pair<std::string, uint64_t>(getFirstComponentName().c_str(), 0));
       }
    }
    #endif
}

Event* Event::clone() {
    Simulation::getSimulation()->getSimulationOutput().
        fatal(CALL_INFO,1,"Called clone() on an Event that doesn't"
              " implement it.");
    return nullptr;  // Never reached, but gets rid of compiler warning
}

Event::id_type Event::generateUniqueId()
{
    return std::make_pair(id_counter++, Simulation::getSimulation()->getRank().rank);
}


void NullEvent::execute(void)
{
    delivery_link->deliverEvent(nullptr);
    delete this;
}



#ifdef USE_MEMPOOL
std::mutex Activity::poolMutex;
std::vector<Activity::PoolInfo_t> Activity::memPools;
#endif


} // namespace SST
