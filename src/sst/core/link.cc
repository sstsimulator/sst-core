// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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
#include "sst/core/sync/syncManager.h"
#include "sst/core/sync/syncQueue.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/timeVortex.h"
#include "sst/core/uninitializedQueue.h"
#include "sst/core/unitAlgebra.h"

#include <utility>

namespace SST {

void
SST::Core::Serialization::serialize_impl<Link*>::operator()(Link*& s, SST::Core::Serialization::serializer& ser)
{
    // Need to treat Links and SelfLinks differently
    bool    self_link;
    // Type of link (Link is not polymorphic, so we can't use
    // dynamic_cast to see which type it is):
    // 0 - nullptr
    // 1 - Link
    // 2 - SelfLink
    // 3 - Sync Link Pair
    int16_t type;

    switch ( ser.mode() ) {
    case serializer::SIZER:
    case serializer::PACK:
        // If s is nullptr, just put in a 0
        if ( nullptr == s ) {
            type = 0;
            ser& type;
            return;
        }

        self_link = (s == s->pair_link);
        if ( self_link ) {
            type = 2;
            ser& type;
            // send_queue will be recreated after deserialization, no
            // need to serialize (polling links not supported)

            Event::HandlerBase* handler = reinterpret_cast<Event::HandlerBase*>(s->delivery_info);

            // Need to serialize both the uintptr_t (delivery_info)
            // and the pointer because we'll need the numerical value
            // of the pointer as a tag when restarting.
            ser & s->delivery_info;
            ser& handler;

            ser & s->defaultTimeBase;
            ser & s->latency;
            // s->pair_link not needed for SelfLinks
            // s->current_time is automatically set on construction so
            // no need to serialize
            ser & s->type;
            ser & s->mode;
            ser & s->tag;
            // Need to serialize anything in the AttachPoint. Not all
            // tool types will be serializable, so will need to check
            // by using dynamic_cast<serializable*>.  Just create a
            // new vector with only the serializable elements and
            // serialize that.  On restart, those will be the only
            // ones that get attached, unless there is another
            // specified on the command line.

            // Determine how many serializable tools there are
            Link::ToolList tools;
            if ( s->attached_tools ) {
                for ( auto x : *s->attached_tools ) {
                    if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) { tools.push_back(x); }
                }
            }
            size_t tool_count = tools.size();
            ser&   tool_count;
            if ( tool_count > 0 ) {
                // Serialize each tool, then call
                // serializeEventAttachPointKey() to serialize any
                // data associated with the key
                for ( auto x : tools ) {
                    SST::Core::Serialization::serializable* obj =
                        dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                    ser& obj;
                    x.first->serializeEventAttachPointKey(ser, x.second);
                }
            }
            return;
        }

        // Check to see if this is a SYNC link pair.  If so, we will
        // serialize all the info together so we can do the
        // registerLink() calls on deserialization.  The serialization
        // call will always be on the non-sync link of the pair.
        if ( s->pair_link->type == Link::SYNC ) {
            type = 3;
            ser& type;

            // MULTI-PARELLEL RESTART: When supporting different
            // restart parallelism, will also need to store the rank
            // of the links in order to have unique identifies for
            // each link.  For pair links on another rank, we will use
            // the delivery_info field as the pointer part of the tag
            // (this is the uintptr_t representation of the link
            // pointer on the remote rank).  This will also require
            // that the remote rank be stored somewhere in the link
            // object.  The most likely place is in the tag, since
            // this field is essentially unused when the link is
            // connected to a sync object (No ordering in the
            // SyncQueue and the real tag will be added on the remote
            // side).

            // No need to keep pointers as tags since we'll have all
            // the data in the serialization stream

            // Store info for the non-sync link
            ser & s->type;
            ser & s->mode;
            ser & s->tag;

            if ( s->type == Link::POLL ) {
                // If I'm a polling link, I need to serialize my
                // pair's send_queue.  For HANDLER and SYNC links, the
                // send_queue will be reinitialized after restart.
                PollingLinkQueue* queue = dynamic_cast<PollingLinkQueue*>(s->pair_link->send_queue);
                // PollingLinkQueues don't work with the serialization
                // functions.  Call things directly.
                // ser&              queue;
                queue->serialize_order(ser);
            }

            // Now serialize the handler
            Event::HandlerBase* handler = reinterpret_cast<Event::HandlerBase*>(s->pair_link->delivery_info);

            // Need to serialize both the uintptr_t
            // (delivery_info) and the pointer because we'll need
            // the numerical value of the pointer as a tag when
            // restarting.
            ser & s->pair_link->delivery_info;
            ser& handler;

            ser & s->defaultTimeBase;
            ser & s->latency;

            // Store data for sync link

            // We'll need the pointer of the synclink in order to
            // generate a globally unique name for
            // SyncManager::registerLink().
            uintptr_t ptr = reinterpret_cast<uintptr_t>(s->pair_link);
            ser&      ptr;

            ser & s->pair_link->type;
            ser & s->pair_link->mode;
            ser & s->pair_link->tag;

            // No need to store queue info, it will be reintialized by
            // the registerLink() call on restart

            // Just put in the delivery_info directly, this is the
            // pointer to the link on the other parition
            ser & s->delivery_info;

            // Need to store my rank info and the rank info for
            // the other partition.  This information will be used
            // to create a unique name for connecting things on
            // restart
            RankInfo ri = Simulation_impl::getSimulation()->getRank();
            ser&     ri;

            // Get the remote rank from my pairs send queue (which
            // is a sync queue)
            SyncQueue* q = dynamic_cast<SyncQueue*>(s->send_queue);
            ri           = q->getToRank();
            ser& ri;

            ser & s->pair_link->defaultTimeBase;
            ser & s->pair_link->latency;

            // Need to serialize anything in the AttachPoint. Not all
            // tool types will be serializable, so will need to check
            // by using dynamic_cast<serializable*>.  Just create a
            // new vector with only the serializable elements and
            // serialize that.  On restart, those will be the only
            // ones that get attached, unless there is another
            // specified on the command line.

            // // Determine how many serializable tools there are
            Link::ToolList tools;
            if ( s->attached_tools ) {
                for ( auto x : *s->attached_tools ) {
                    if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) { tools.push_back(x); }
                }
            }
            size_t tool_count = tools.size();
            ser&   tool_count;
            if ( tool_count > 0 ) {
                // Serialize each tool, then call
                // serializeEventAttachPointKey() to serialize any
                // data associated with the key
                for ( auto x : tools ) {
                    SST::Core::Serialization::serializable* obj =
                        dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                    ser& obj;
                    x.first->serializeEventAttachPointKey(ser, x.second);
                }
            }
        }
        else {
            // Regular link
            type = 1;
            ser& type;

            // Need to put a uintptr_t in for my pointer and my pair's
            // pointer.  This will be used to identify link
            // connections on restart

            uintptr_t ptr = reinterpret_cast<uintptr_t>(s);
            ser&      ptr;
            ptr = reinterpret_cast<uintptr_t>(s->pair_link);
            ser& ptr;

            // Store some of the data we'll need to make decisions
            // during unpacking
            ser & s->type;
            ser & s->mode;
            ser & s->tag;

            if ( s->type == Link::POLL ) {
                // If I'm a polling link, I need to serialize my
                // pair's send_queue.  For HANDLER and SYNC links, the
                // send_queue will be reinitialized after restart
                PollingLinkQueue* queue = dynamic_cast<PollingLinkQueue*>(s->pair_link->send_queue);
                // PollingLinkQueues don't work with the serialization
                // functions.  Call things directly.
                // ser&              queue;
                queue->serialize_order(ser);
            }

            // My handler is stored in pair_link->delivery_info

            // First serialize the pointer tag so we can fix
            // things up after restart

            // Now serialize the handler
            Event::HandlerBase* handler = reinterpret_cast<Event::HandlerBase*>(s->pair_link->delivery_info);

            // Need to serialize both the uintptr_t
            // (delivery_info) and the pointer because we'll need
            // the numerical value of the pointer as a tag when
            // restarting.
            ser & s->pair_link->delivery_info;
            ser& handler;

            ser & s->defaultTimeBase;
            ser & s->latency;

            // s->pair_link - tag stored above
            // s->current_time is automatically set on construction so
            // no need to serialize

            // Need to serialize anything in the AttachPoint. Not all
            // tool types will be serializable, so will need to check
            // by using dynamic_cast<serializable*>.  Just create a
            // new vector with only the serializable elements and
            // serialize that.  On restart, those will be the only
            // ones that get attached, unless there is another
            // specified on the command line.

            // Determine how many serializable tools there are
            Link::ToolList tools;
            if ( s->attached_tools ) {
                for ( auto x : *s->attached_tools ) {
                    if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) { tools.push_back(x); }
                }
            }
            size_t tool_count = tools.size();
            ser&   tool_count;
            if ( tool_count > 0 ) {
                // Serialize each tool, then call
                // serializeEventAttachPointKey() to serialize any
                // data associated with the key
                for ( auto x : tools ) {
                    SST::Core::Serialization::serializable* obj =
                        dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                    ser& obj;
                    x.first->serializeEventAttachPointKey(ser, x.second);
                }
            }
        }
        break;
    case serializer::UNPACK:
        ser& type;

        // If we put in a nullptr, return a nullptr
        if ( type == 0 ) {
            s = nullptr;
            return;
        }

        if ( type == 2 ) {
            // Self link
            s = new SelfLink();
            ser.report_new_pointer(reinterpret_cast<uintptr_t>(s));

            // send_queue will be recreated after deserialization, no
            // need to serialize (polling links not supported)

            uintptr_t delivery_info;
            ser&      delivery_info;

            Event::HandlerBase* handler;
            ser&                handler;
            s->delivery_info = reinterpret_cast<uintptr_t>(handler);

            Simulation_impl::getSimulation()->event_handler_restart_tracking[delivery_info] = s->delivery_info;

            ser & s->defaultTimeBase;
            ser & s->latency;
            // s->pair_link not needed for SelfLinks
            // s->current_time is automatically set on construction so
            // no need to serialize
            ser & s->type;
            ser & s->mode;
            ser & s->tag;
            // Profile tools not yet supported
            // ser & s->profile_tools;

            s->send_queue = Simulation_impl::getSimulation()->getTimeVortex();

            // Need to restore any Tool in the AttachPoint.
            size_t tool_count;
            ser&   tool_count;
            if ( tool_count > 0 ) {
                s->attached_tools = new Link::ToolList();
                for ( size_t i = 0; i < tool_count; ++i ) {
                    SST::Core::Serialization::serializable* tool;
                    uintptr_t                               key;
                    ser&                                    tool;
                    Link::AttachPoint*                      ap = dynamic_cast<Link::AttachPoint*>(tool);
                    ap->serializeEventAttachPointKey(ser, key);
                    s->attached_tools->emplace_back(ap, key);
                }
            }
            else {
                s->attached_tools = nullptr;
            }
        }
        else if ( type == 3 ) {
            // Sync link

            // Need to create both links in the pair
            s = new Link();
            ser.report_new_pointer(reinterpret_cast<uintptr_t>(s));

            Link* pair_link      = new Link();
            s->pair_link         = pair_link;
            pair_link->pair_link = s;

            // Get data for non-sync link
            ser & s->type;
            ser & s->mode;
            ser & s->tag;

            // Get the send_queue.  It goes in my pair_link
            if ( s->type == Link::POLL ) {
                // If I'm a polling link, need to deserialize my
                // pair's send_queue. For now, I will store it in my
                // own send_queue variable and swap once we have both
                // links.
                PollingLinkQueue* queue;
                // PollingLinkQueues don't work with the serialization
                // functions.  Call things directly.
                // ser&              queue;
                queue = new PollingLinkQueue();
                queue->serialize_order(ser);
                pair_link->send_queue = queue;
            }
            else {
                pair_link->send_queue = Simulation_impl::getSimulation()->getTimeVortex();
            }

            // Get delivery_info (handler). It goes in my pair link
            uintptr_t delivery_info;
            ser&      delivery_info;

            Event::HandlerBase* handler;
            ser&                handler;
            pair_link->delivery_info = reinterpret_cast<uintptr_t>(handler);

            Simulation_impl::getSimulation()->event_handler_restart_tracking[delivery_info] = pair_link->delivery_info;

            // Get defaultTimeBase and latency
            ser & s->defaultTimeBase;
            ser & s->latency;

            // Now get data from the sync side of the link
            uintptr_t my_tag;
            ser&      my_tag;

            ser & pair_link->type;
            ser & pair_link->mode;
            ser & pair_link->tag;

            // Get the delivery info for the sync.  This is the
            // pointer to the link on the remote partition
            ser& delivery_info;


            RankInfo local_rank;
            RankInfo remote_rank;
            ser&     local_rank;
            ser&     remote_rank;

            // Need to reregister with the SyncManager, but first need to create a unique name
            std::string    uname = s->createUniqueGlobalLinkName(local_rank, my_tag, remote_rank, delivery_info);
            ActivityQueue* sync_q =
                Simulation_impl::getSimulation()->syncManager->registerLink(remote_rank, local_rank, uname, pair_link);
            // SyncQueue goes in the non-sync link
            s->send_queue = sync_q;

            ser & pair_link->defaultTimeBase;
            ser & pair_link->latency;

            // Need to restore any Tool in the AttachPoint.
            size_t tool_count;
            ser&   tool_count;
            if ( tool_count > 0 ) {
                s->attached_tools = new Link::ToolList();
                for ( size_t i = 0; i < tool_count; ++i ) {
                    SST::Core::Serialization::serializable* tool;
                    uintptr_t                               key;
                    ser&                                    tool;
                    Link::AttachPoint*                      ap = dynamic_cast<Link::AttachPoint*>(tool);
                    ap->serializeEventAttachPointKey(ser, key);
                    s->attached_tools->emplace_back(ap, key);
                }
            }
            else {
                s->attached_tools = nullptr;
            }
        }
        else {
            // Regular link

            // Pull out the tags for the two links
            uintptr_t my_tag;
            uintptr_t pair_tag;

            ser& my_tag;
            ser& pair_tag;

            s = new Link();
            ser.report_new_pointer(reinterpret_cast<uintptr_t>(s));
            Link* pair_link = nullptr;

            // Need to check to see if my pair link has been unpacked
            auto& link_tracker = Simulation_impl::getSimulation()->link_restart_tracking;
            if ( link_tracker.count(pair_tag) ) {
                pair_link = link_tracker[pair_tag];
                link_tracker.erase(pair_tag);

                // Set pair link
                s->pair_link = pair_link;

                // Set my neighbor's pair link
                pair_link->pair_link = s;
            }
            else {
                link_tracker[my_tag] = s;
            }


            ser & s->type;
            ser & s->mode;
            ser & s->tag;

            if ( s->type == Link::POLL ) {
                // If I'm a polling link, need to deserialize my
                // pair's send_queue. For now, I will store it in my
                // own send_queue variable and swap once we have both
                // links.
                PollingLinkQueue* queue;
                // PollingLinkQueues don't work with the serialization
                // functions.  Call things directly.
                // ser&              queue;
                queue = new PollingLinkQueue();
                queue->serialize_order(ser);
                s->send_queue = queue;
            }
            else {
                s->send_queue = Simulation_impl::getSimulation()->getTimeVortex();
            }

            uintptr_t delivery_info;
            ser&      delivery_info;

            Event::HandlerBase* handler;
            ser&                handler;
            s->delivery_info = reinterpret_cast<uintptr_t>(handler);

            Simulation_impl::getSimulation()->event_handler_restart_tracking[delivery_info] = s->delivery_info;


            // If we have a pair link already, swap delivery_info and
            // send_queue
            if ( pair_link ) {
                uintptr_t temp              = s->delivery_info;
                s->delivery_info            = s->pair_link->delivery_info;
                s->pair_link->delivery_info = temp;

                // Swap the queues
                ActivityQueue* queue     = s->send_queue;
                s->send_queue            = s->pair_link->send_queue;
                s->pair_link->send_queue = queue;
            }

            ser & s->defaultTimeBase;
            ser & s->latency;

            // Need to restore any Tool in the AttachPoint.
            size_t tool_count;
            ser&   tool_count;
            if ( tool_count > 0 ) {
                s->attached_tools = new Link::ToolList();
                for ( size_t i = 0; i < tool_count; ++i ) {
                    SST::Core::Serialization::serializable* tool;
                    uintptr_t                               key;
                    ser&                                    tool;
                    Link::AttachPoint*                      ap = dynamic_cast<Link::AttachPoint*>(tool);
                    ap->serializeEventAttachPointKey(ser, key);
                    s->attached_tools->emplace_back(ap, key);
                }
            }
            else {
                s->attached_tools = nullptr;
            }
        }
        break;
    case serializer::MAP:
        // This version of the function is not called in mapping mode.
        break;
    }
}

void
SST::Core::Serialization::serialize_impl<Link*>::operator()(
    Link*& UNUSED(s), SST::Core::Serialization::serializer& UNUSED(ser), const char* UNUSED(name))
{
    // TODO: Implement Link mapping mode
}

/**
 * Null Event.  Used when nullptr is passed into any of the send
 * functions.  On delivery, it will delete itself and return nullptr.
 */

class NullEvent : public Event
{
public:
    NullEvent() : Event() {}
    ~NullEvent() {}

    void execute() override
    {
        (*reinterpret_cast<HandlerBase*>(delivery_info))(nullptr);
        delete this;
    }

private:
    ImplementSerializable(SST::NullEvent)
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
    attached_tools(nullptr)
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
    attached_tools(nullptr)
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

    if ( attached_tools ) delete attached_tools;
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
    if ( nullptr != pair_link->send_queue ) {
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
        functor->transferAttachedToolInfo(handler);
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

    if ( attached_tools ) {
        for ( auto& x : *attached_tools ) {
            x.first->eventSent(x.second, event);
            // Check to see if the event was deleted.  If so, return.
            if ( nullptr == event ) return;
        }
    }
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

    Event*           event      = nullptr;
    Simulation_impl* simulation = Simulation_impl::getSimulation();

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

std::string
Link::createUniqueGlobalLinkName(RankInfo local_rank, uintptr_t local_ptr, RankInfo remote_rank, uintptr_t remote_ptr)
{
    std::stringstream ss;

    uint32_t  high_rank;
    uint32_t  low_rank;
    uintptr_t high_ptr;
    uintptr_t low_ptr;
    if ( local_rank.rank > remote_rank.rank ) {
        high_rank = local_rank.rank;
        high_ptr  = local_ptr;
        low_rank  = remote_rank.rank;
        low_ptr   = remote_ptr;
    }
    else if ( remote_rank.rank > local_rank.rank ) {
        high_rank = remote_rank.rank;
        high_ptr  = remote_ptr;
        low_rank  = local_rank.rank;
        low_ptr   = local_ptr;
    }
    else { // Ranks are the same
        high_rank = remote_rank.rank;
        low_rank  = high_rank;
        if ( local_ptr > remote_ptr ) {
            high_ptr = local_ptr;
            low_ptr  = remote_ptr;
        }
        else {
            high_ptr = remote_ptr;
            low_ptr  = local_ptr;
        }
    }

    // Convert each parameter to hexadecimal and concatenate
    ss << std::hex << std::setw(8) << std::setfill('0') << low_rank << "-" << std::hex
       << std::setw(sizeof(uintptr_t) * 2) << std::setfill('0') << low_ptr << "-" << std::hex << std::setw(8)
       << std::setfill('0') << high_rank << "-" << std::hex << std::setw(sizeof(uintptr_t) * 2) << std::setfill('0')
       << high_ptr;

    return ss.str();
}

void
Link::attachTool(AttachPoint* tool, const AttachPointMetaData& mdata)
{
    if ( !attached_tools ) attached_tools = new ToolList();
    auto key = tool->registerLinkAttachTool(mdata);
    attached_tools->push_back(std::make_pair(tool, key));
}

void
Link::AttachPoint::serializeEventAttachPointKey(
    SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& UNUSED(key))
{}

} // namespace SST
