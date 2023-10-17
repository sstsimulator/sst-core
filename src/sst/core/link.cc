// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/link.h"

#include "sst/core/event.h"
#include "sst/core/factory.h"
#include "sst/core/initQueue.h"
#include "sst/core/pollingLinkQueue.h"
#include "sst/core/profile/eventHandlerProfileTool.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/ssthandler.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/timeVortex.h"
#include "sst/core/uninitializedQueue.h"
#include "sst/core/unitAlgebra.h"

#include <utility>

namespace SST {

/**
 * Null Event.  Used when nullptr is passed into any of the send
 * functions.  On delivery, it will delete itself and return nullptr.
 */

class NullEvent : public Event
{
public:
    NullEvent() : Event() {}
    ~NullEvent() {}

    void execute(void) override
    {
        (*reinterpret_cast<HandlerBase*>(delivery_info))(nullptr);
        delete this;
    }

private:
    ImplementSerializable(SST::NullEvent)
};


class LinkSendProfileToolList
{
public:
    LinkSendProfileToolList() {}

    inline void eventSent(Event* ev)
    {
        for ( auto& x : tools ) {
            x.first->eventSent(x.second, ev);
        }
    }

    void addProfileTool(SST::Profile::EventHandlerProfileTool* tool, const EventHandlerMetaData& mdata)
    {
        auto key = tool->registerHandler(mdata);
        tools.push_back(std::make_pair(tool, key));
    }

private:
    std::vector<std::pair<SST::Profile::EventHandlerProfileTool*, uintptr_t>> tools;
};

Link::Link(LinkId_t tag) :
    send_queue(nullptr),
    delivery_info(0),
    defaultTimeBase(0),
    latency(1),
    pair_link(nullptr),
    current_time(Simulation_impl::getSimulation()->currentSimCycle),
    type(UNINITIALIZED),
    mode(INIT),
    tag(tag),
    profile_tools(nullptr)
{}

Link::Link() :
    send_queue(nullptr),
    delivery_info(0),
    defaultTimeBase(0),
    latency(1),
    pair_link(nullptr),
    current_time(Simulation_impl::getSimulation()->currentSimCycle),
    type(UNINITIALIZED),
    mode(INIT),
    tag(-1),
    profile_tools(nullptr)
{}

Link::~Link()
{
    // Check to see if my pair_link is nullptr.  If not, let the other
    // link know I've been deleted
    if ( pair_link != nullptr && pair_link != this ) {
        pair_link->pair_link = nullptr;
        // If my pair link is a SYNC link,
        // also need to delete it because no one else has a pointer to.
        if ( SYNC == pair_link->type ) delete pair_link;
    }

    if ( profile_tools ) delete profile_tools;
}

void
Link::finalizeConfiguration()
{
    mode = RUN;
    if ( SYNC == type ) {
        // No configuration changes to be made
        return;
    }

    // If we have a queue, it means we ended up having init events
    // sent.  No need to keep the initQueue around
    if ( nullptr == pair_link->send_queue ) {
        delete pair_link->send_queue;
        pair_link->send_queue = nullptr;
    }

    if ( HANDLER == type ) { pair_link->send_queue = Simulation_impl::getSimulation()->getTimeVortex(); }
    else if ( POLL == type ) {
        pair_link->send_queue = new PollingLinkQueue();
    }

    // If my pair link is a SYNC link, also need to call
    // finalizeConfiguration() on it since no one else has a pointer
    // to it
    if ( SYNC == pair_link->type ) pair_link->finalizeConfiguration();
}

void
Link::prepareForComplete()
{
    mode = COMPLETE;

    if ( SYNC == type ) {
        // No configuration changes to be made
        return;
    }

    if ( POLL == type ) { delete pair_link->send_queue; }

    pair_link->send_queue = nullptr;

    // If my pair link is a SYNC link, also need to call
    // prepareForComplete() on it
    if ( SYNC == pair_link->type ) pair_link->prepareForComplete();
}

void
Link::setPolling()
{
    type = POLL;
}


void
Link::setLatency(Cycle_t lat)
{
    latency = lat;
}

void
Link::addSendLatency(int cycles, const std::string& timebase)
{
    SimTime_t tb = Simulation_impl::getSimulation()->getTimeLord()->getSimCycles(timebase, "addOutputLatency");
    latency += (cycles * tb);
}

void
Link::addSendLatency(SimTime_t cycles, TimeConverter* timebase)
{
    latency += timebase->convertToCoreTime(cycles);
}

void
Link::addRecvLatency(int cycles, const std::string& timebase)
{
    SimTime_t tb = Simulation_impl::getSimulation()->getTimeLord()->getSimCycles(timebase, "addOutputLatency");
    pair_link->latency += (cycles * tb);
}

void
Link::addRecvLatency(SimTime_t cycles, TimeConverter* timebase)
{
    pair_link->latency += timebase->convertToCoreTime(cycles);
}

void
Link::setFunctor(Event::HandlerBase* functor)
{
    if ( UNLIKELY(type == POLL) ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(
            CALL_INFO, 1, "Cannot call setFunctor on a Polling Link\n");
    }

    type                     = HANDLER;
    pair_link->delivery_info = reinterpret_cast<uintptr_t>(functor);
}

void
Link::replaceFunctor(Event::HandlerBase* functor)
{
    if ( UNLIKELY(type == POLL) ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(
            CALL_INFO, 1, "Cannot call replaceFunctor on a Polling Link\n");
    }

    type = HANDLER;
    if ( pair_link->delivery_info ) {
        auto* handler = reinterpret_cast<Event::HandlerBase*>(pair_link->delivery_info);
        functor->transferProfilingInfo(handler);
        delete handler;
    }
    pair_link->delivery_info = reinterpret_cast<uintptr_t>(functor);
}

void
Link::send_impl(SimTime_t delay, Event* event)
{
    if ( RUN != mode ) {
        if ( INIT == mode ) {
            Simulation_impl::getSimulation()->getSimulationOutput().fatal(
                CALL_INFO, 1,
                "ERROR: Trying to send or recv from link during initialization.  Send and Recv cannot be called before "
                "setup.\n");
        }
        else if ( COMPLETE == mode ) {
            Simulation_impl::getSimulation()->getSimulationOutput().fatal(
                CALL_INFO, 1, "ERROR: Trying to call send or recv during complete phase.");
        }
    }
    Cycle_t cycle = current_time + delay + latency;

    if ( event == nullptr ) { event = new NullEvent(); }
    event->setDeliveryTime(cycle);
    event->setDeliveryInfo(tag, delivery_info);

#if __SST_DEBUG_EVENT_TRACKING__
    event->addSendComponent(comp, ctype, port);
    event->addRecvComponent(pair_link->comp, pair_link->ctype, pair_link->port);
#endif

    if ( profile_tools ) profile_tools->eventSent(event);
    send_queue->insert(event);
}


Event*
Link::recv()
{
    // Check to make sure this is a polling link
    if ( UNLIKELY(type != POLL) ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(
            CALL_INFO, 1, "Cannot call recv on a Link with an event handler installed (non-polling link.\n");
    }

    Event*      event      = nullptr;
    Simulation* simulation = Simulation_impl::getSimulation();

    if ( !pair_link->send_queue->empty() ) {
        Activity* activity = pair_link->send_queue->front();
        if ( activity->getDeliveryTime() <= simulation->getCurrentSimCycle() ) {
            event = static_cast<Event*>(activity);
            pair_link->send_queue->pop();
        }
    }
    return event;
}

void
Link::sendUntimedData(Event* data)
{
    if ( RUN == mode ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(
            CALL_INFO, 1,
            "ERROR: Trying to call sendUntimedData/sendInitData or recvUntimedData/recvInitData during the run phase.");
    }

    if ( send_queue == nullptr ) { send_queue = new InitQueue(); }
    Simulation_impl::getSimulation()->untimed_msg_count++;
    data->setDeliveryTime(Simulation_impl::getSimulation()->untimed_phase + 1);
    data->setDeliveryInfo(tag, delivery_info);

    send_queue->insert(data);
#if __SST_DEBUG_EVENT_TRACKING__
    data->addSendComponent(comp, ctype, port);
    data->addRecvComponent(pair_link->comp, pair_link->ctype, pair_link->port);
#endif
}

// Called by SyncManager
void
Link::sendUntimedData_sync(Event* data)
{
    if ( send_queue == nullptr ) { send_queue = new InitQueue(); }

    send_queue->insert(data);
}

Event*
Link::recvUntimedData()
{
    if ( pair_link->send_queue == nullptr ) return nullptr;

    Event* event = nullptr;
    if ( !pair_link->send_queue->empty() ) {
        Activity* activity = pair_link->send_queue->front();
        if ( activity->getDeliveryTime() <= Simulation_impl::getSimulation()->untimed_phase ) {
            event = static_cast<Event*>(activity);
            pair_link->send_queue->pop();
        }
    }
    return event;
}

void
Link::setDefaultTimeBase(TimeConverter* tc)
{
    if ( tc == nullptr )
        defaultTimeBase = 0;
    else
        defaultTimeBase = tc->getFactor();
}

TimeConverter*
Link::getDefaultTimeBase()
{
    if ( defaultTimeBase == 0 ) return nullptr;
    return Simulation_impl::getSimulation()->getTimeLord()->getTimeConverter(defaultTimeBase);
}

const TimeConverter*
Link::getDefaultTimeBase() const
{
    if ( defaultTimeBase == 0 ) return nullptr;
    return Simulation_impl::getSimulation()->getTimeLord()->getTimeConverter(defaultTimeBase);
}

void
Link::addProfileTool(SST::Profile::EventHandlerProfileTool* tool, const EventHandlerMetaData& mdata)
{
    if ( !profile_tools ) profile_tools = new LinkSendProfileToolList();
    profile_tools->addProfileTool(tool, mdata);
}


} // namespace SST
