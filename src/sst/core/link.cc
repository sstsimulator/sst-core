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
#include "sst/core/linkPair.h"
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
SST::Core::Serialization::serialize_impl<Link*>::serialize_events(
    serializer& ser, uintptr_t delivery_info, ActivityQueue* queue)
{
    if ( ser.mode() == serializer::SIZER || ser.mode() == serializer::PACK ) {
        // Look up all the events for the specified handler
        std::pair<::SST::pvt::TimeVortexSort::iterator, ::SST::pvt::TimeVortexSort::iterator> activites =
            Simulation_impl::getSimulation()->getEventsForHandler(delivery_info);

        size_t count = std::distance(activites.first, activites.second);
        SST_SER(count);
        for ( auto it = activites.first; it < activites.second; ++it ) {
            SST_SER(*it);
        }
    }
    else if ( ser.mode() == serializer::UNPACK ) {
        size_t count;
        SST_SER(count);

        for ( size_t i = 0; i < count; ++i ) {
            Event* ev;
            SST_SER(ev);
            // Insert into the specified ActivityQueue after updating
            // delvery_info
            Link::updateEventDeliveryInfo(ev, delivery_info);
            queue->insert(ev);
        }
    }
}

void
SST::Core::Serialization::serialize_impl<Link*>::operator()(Link*& s, serializer& ser, ser_opt_t UNUSED(options))
{
    // Type of link (Link is not polymorphic, so we can't use
    // dynamic_cast to see which type it is):
    // 0 - nullptr
    // 1 - Link
    // 2 - SelfLink
    // 3 - Sync Link Pair
    int16_t type;
    int16_t REG  = 1;
    int16_t SELF = 2;
    int16_t SYNC = 3;

    // Need a pointer to my simulation object
    Simulation_impl* sim = Simulation_impl::getSimulation();

    // In order to uniquely identify links on restart, we need to
    // track the rank of the link and its pair link.  For regular
    // links, they are the same, but for sync link pairs, the pair
    // link will be on a different rank.  For self links, this
    // information isn't needed
    RankInfo my_rank   = sim->getRank();
    RankInfo pair_rank = my_rank;

    switch ( ser.mode() ) {
    case serializer::SIZER:
    case serializer::PACK:
    {
        // If s is nullptr, just put in a 0
        if ( nullptr == s ) {
            type = 0;
            SST_SER(type);
            return;
        }

        // Figure out what type of link this is
        if ( s == s->pair_link )
            type = SELF;
        else if ( s->pair_link->type == Link::SYNC )
            type = SYNC;
        else
            type = REG;

        SST_SER(type);

        /*
          Unique Identifiers

          For non-selflinks, we need to be able to create a unique
          identifier so we can connect the pairs on restart.  The
          unique identifiers are created using the MPI rank and point
          of the link cast as a uintptr_t.

          For regular links, we only store the rank once since both
          links in the pair are on the same rank.

          For SYNC links, the local link only knows the remote link by
          it's pair link, so we will use that pointer for the unique
          ID.

          For self links, no rank info is stored since we don't need
          to create a unique ID
        */
        if ( type == SYNC || type == REG ) {
            SST_SER(my_rank);

            uintptr_t ptr;
            if ( type == SYNC )
                ptr = reinterpret_cast<uintptr_t>(s->pair_link);
            else
                ptr = reinterpret_cast<uintptr_t>(s);

            SST_SER(ptr);

            if ( type == SYNC ) {
                // The unique ID for the remote links is constructed from
                // the rank of the remote pair link and its pointer on
                // that rank.  The remote pointer is stored in
                // delivery_info and we can get the remote rank from the
                // sync queue.
                SyncQueue* q = dynamic_cast<SyncQueue*>(s->send_queue);
                pair_rank    = q->getToRank();
                SST_SER(pair_rank);
                SST_SER(s->delivery_info);
            }
            else {
                // Unique ID for my pair link is my rank and pair_link
                // pointer.  Rank is already stored, just store pair
                // pointer
                uintptr_t pair_ptr = reinterpret_cast<uintptr_t>(s->pair_link);
                SST_SER(pair_ptr);
            }
        } // if ( type == SYNC || type == REG )

        /*
          Store the metadata for this link
        */
        SST_SER(s->type);
        SST_SER(s->mode);
        SST_SER(s->tag);

        /*
          Store handler for handler links, or store the contents
          of the PollingLinkQueue for polling links
        */
        if ( s->type == Link::POLL ) {
            // If I'm a polling link, I need to serialize my pair's
            // send_queue (which is really my receive queue).  For
            // HANDLER and SYNC links, the send_queue will be
            // reinitialized after restart.
            PollingLinkQueue* queue = dynamic_cast<PollingLinkQueue*>(s->pair_link->send_queue);
            // PollingLinkQueues don't work with the serialization
            // functions.  Call things directly.
            queue->serialize_order(ser);
        }
        else {
            // Store the handler for this link.

            // Need to serialize both the uintptr_t stored in
            // pair_link->delivery_info and the pointer because we'll
            // need the numerical value of the pointer as a tag when
            // restarting.
            Event::HandlerBase* handler = reinterpret_cast<Event::HandlerBase*>(s->pair_link->delivery_info);

            // Tag for handler
            SST_SER(s->pair_link->delivery_info);
            // Actual handler
            SST_SER(handler);
        }

        /*
          Store timing info for link
        */
        SST_SER(s->defaultTimeBase);
        SST_SER(s->latency);
        // If part of a sync pair, need to save the pair_link's
        // latency in case Link::addRecvLatency() was called.
        // This will be added to the new pair_link on restart.
        if ( type == SYNC ) SST_SER(s->pair_link->latency);

        /*
          Serialize any serializable attached tools

          Need to serialize anything in the AttachPoint. Not all tool
          types will be serializable, so will need to check by using
          dynamic_cast<serializable*>.  Just create a new vector with
          only the serializable elements and serialize that.  On
          restart, those will be the only ones that get attached,
          unless there is another specified on the command line.
        */

        // Determine how many serializable tools there are
        Link::ToolList tools;
        if ( s->attached_tools ) {
            for ( auto x : *s->attached_tools ) {
                if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) {
                    tools.push_back(x);
                }
            }
        }
        size_t tool_count = tools.size();
        SST_SER(tool_count);
        if ( tool_count > 0 ) {
            // Serialize each tool, then call
            // serializeEventAttachPointKey() to serialize any data
            // associated with the key
            for ( auto x : tools ) {
                SST::Core::Serialization::serializable* obj =
                    dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                SST_SER(obj);
                x.first->serializeEventAttachPointKey(ser, x.second);
            }
        }

        /*
          Serialize all the events targeting this Link
        */
        serialize_events(ser, s->pair_link->delivery_info);
    } break;
    case serializer::UNPACK:
    {
        SST_SER(type);


        // If we put in a nullptr, return a nullptr
        if ( type == 0 ) {
            s = nullptr;
            return;
        }

        /*
          If this isn't a self link, then we need to determine if this
          is a sync link or a regular link with the potentially new
          repartioning on restart.

          We do this by querying the Simulation_impl object to see if
          the pair link is on the same partition or not.  If it is, it
          is a regular link, otherwise it is part of a sync link pair.
        */
        bool is_orig_sync = (type == 3);

        /*
          Unique identifiers

          Get the ranks and tags for this link and its pair link
        */
        RankInfo my_restart_rank   = sim->getRank();
        RankInfo pair_restart_rank = my_restart_rank;

        uintptr_t my_tag;
        uintptr_t pair_tag;

        if ( type == SYNC || type == REG ) {
            SST_SER(my_rank);
            SST_SER(my_tag);

            if ( type == SYNC )
                SST_SER(pair_rank);
            else
                pair_rank = my_rank;

            SST_SER(pair_tag);
        }


        /*
          Determine current sync state
        */
        if ( type != SELF ) {
            pair_restart_rank = sim->getRankForLinkOnRestart(pair_rank.rank, pair_tag);

            // If pair_restart_rank.rank == UNASSIGNED, then we have
            // the same paritioning as the checkpoint and the ranks
            // for both links are the same
            if ( pair_restart_rank.rank == RankInfo::UNASSIGNED ) pair_restart_rank = pair_rank;
        }

        bool is_restart_sync = (my_restart_rank != pair_restart_rank);

        /*
          Create or get link from tracker

          See if the link has already been created by its pair. If
          not, create a LinkPair.  This link will be the left link
          of the pair.
        */
        if ( type == SELF ) {
            s = new SelfLink();
            ser.unpacker().report_new_pointer(reinterpret_cast<uintptr_t>(s));
        }
        else {
            auto&                     link_tracker   = sim->link_restart_tracking;
            std::pair<int, uintptr_t> my_unique_id   = std::make_pair(my_rank.rank, my_tag);
            std::pair<int, uintptr_t> pair_unique_id = std::make_pair(pair_rank.rank, pair_tag);

            if ( !is_restart_sync && link_tracker.count(my_unique_id) ) {
                // Get my link and erase it from the map
                s = link_tracker[my_unique_id];
                link_tracker.erase(my_unique_id);
            }
            else {
                // Create a link pair and set s to the left link
                LinkPair links;
                s = links.getLeft();

                // Set latency to zero (it defaults to 1) so that the
                // addition below works properly
                s->setLatency(0);
                s->pair_link->setLatency(0);

                // Put my pair link in the tracking map
                link_tracker[pair_unique_id] = s->pair_link;
            }
        }

        /*
          Get the metadata for the link
        */
        SST_SER(s->type);
        SST_SER(s->mode);
        SST_SER(s->tag);

        /*
          Get handler for handler links, or restore the contents
          of the PollingLinkQueue for polling links
        */
        if ( s->type == Link::POLL ) {
            // If I'm a polling link, I need to serialize my
            // pair's send_queue (which is really my receive
            // queue).  For HANDLER and SYNC links, the send_queue
            // will be reinitialized after restart.
            PollingLinkQueue* queue;
            // PollingLinkQueues don't work with the serialization
            // functions.  Call things directly.
            queue = new PollingLinkQueue();
            queue->serialize_order(ser);
            s->pair_link->send_queue = queue;
        }
        else {
            // Set the send_queue to the TimeVortex
            s->pair_link->send_queue = sim->getTimeVortex();

            // Restore the handler for this link.
            uintptr_t delivery_info;
            SST_SER(delivery_info);

            Event::HandlerBase* handler;
            SST_SER(handler);
            s->pair_link->delivery_info = reinterpret_cast<uintptr_t>(handler);

            // Report the handler for tracking
            sim->event_handler_restart_tracking[delivery_info] = s->pair_link->delivery_info;
        }

        /*
          Get the timing info
        */
        SST_SER(s->defaultTimeBase);

        // Get the latency. We need to add it to what's already
        // there, because our pair_link deserialization may have
        // added latency in the case of a sync link where
        // Link::addRecvLatency() was called.
        SimTime_t latency;
        SST_SER(latency);
        s->latency += latency;

        // If originally part of a sync pair, need to restore the
        // pair_link's latency in case Link::addRecvLatency() was
        // called. We will add this latency to the pair_link
        // latency because this link pair may no longer be a sync
        // pair and may already have non-zero latency.
        if ( is_orig_sync ) {
            SST_SER(latency);
            s->pair_link->latency += latency;
        }

        /*
          Restore attached tools
        */
        size_t tool_count;
        SST_SER(tool_count);
        if ( tool_count > 0 ) {
            s->attached_tools = new Link::ToolList();
            for ( size_t i = 0; i < tool_count; ++i ) {
                SST::Core::Serialization::serializable* tool;
                uintptr_t                               key;
                SST_SER(tool);
                Link::AttachPoint* ap = dynamic_cast<Link::AttachPoint*>(tool);
                ap->serializeEventAttachPointKey(ser, key);
                s->attached_tools->emplace_back(ap, key);
            }
        }
        else {
            s->attached_tools = nullptr;
        }

        /*
          Deserialize the events targetting this link
        */

        // Need to send the events on the pair's send_queue, with
        // the delivery_info stored there. If this happens to be a
        // PollingLinkQueue, then nothing will actually get sent
        // since no events would have been serialized at this
        // point in Link serialization.
        serialize_events(ser, s->pair_link->delivery_info, s->pair_link->send_queue);

        // Need to finish initializing the links if this is now a
        // sync link.
        if ( is_restart_sync ) {
            s->pair_link->type = Link::SYNC;
            s->pair_link->mode = s->mode;
            s->pair_link->tag  = s->tag;

            s->pair_link->defaultTimeBase = 1;

            // Need to register with the SyncManager, but first
            // need to create a unique name
            std::string    uname = s->createUniqueGlobalLinkName(my_rank, my_tag, pair_rank, pair_tag);
            ActivityQueue* sync_q =
                sim->syncManager->registerLink(pair_restart_rank, my_restart_rank, uname, s->pair_link);
            s->send_queue = sync_q;
        }
    } break;
    case serializer::MAP:
        // TODO: Implement Link mapping mode
        break;
    }
}

/**
 * Null Event.  Used when nullptr is passed into any of the send
 * functions.  On delivery, it will delete itself and return nullptr.
 */

class NullEvent : public Event
{
public:
    NullEvent() :
        Event()
    {}
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

    if ( HANDLER == type ) {
        pair_link->send_queue = Simulation_impl::getSimulation()->getTimeVortex();
    }
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

    if ( POLL == type ) {
        delete pair_link->send_queue;
    }

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
Link::addSendLatency(SimTime_t cycles, TimeConverter timebase)
{
    latency += timebase.convertToCoreTime(cycles);
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
Link::addRecvLatency(SimTime_t cycles, TimeConverter timebase)
{
    pair_link->latency += timebase.convertToCoreTime(cycles);
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

Event::HandlerBase*
Link::getFunctor()
{
    if ( UNLIKELY(type == POLL) ) {
        return nullptr;
    }
    return reinterpret_cast<Event::HandlerBase*>(pair_link->delivery_info);
}

void
Link::send_impl(SimTime_t delay, Event* event)
{
    if ( RUN != mode ) {
        if ( INIT == mode ) {
            Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
                "ERROR: Trying to send or recv from link during initialization.  Send and Recv cannot be called before "
                "setup.\n");
        }
        else if ( COMPLETE == mode ) {
            Simulation_impl::getSimulation()->getSimulationOutput().fatal(
                CALL_INFO, 1, "ERROR: Trying to call send or recv during complete phase.");
        }
    }
    Cycle_t cycle = current_time + delay + latency;

    if ( event == nullptr ) {
        event = new NullEvent();
    }
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
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
            "ERROR: Trying to call sendUntimedData/sendInitData or recvUntimedData/recvInitData during the run phase.");
    }

    if ( send_queue == nullptr ) {
        send_queue = new InitQueue();
    }
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
    if ( send_queue == nullptr ) {
        send_queue = new InitQueue();
    }

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

void
Link::setDefaultTimeBase(TimeConverter tc)
{
    defaultTimeBase = tc.getFactor();
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
Link::detachTool(AttachPoint* tool)
{
    if ( !attached_tools ) return;

    for ( auto x = attached_tools->begin(); x != attached_tools->end(); ++x ) {
        if ( x->first == tool ) {
            attached_tools->erase(x);
            break;
        }
    }
}

void
Link::AttachPoint::serializeEventAttachPointKey(
    SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& UNUSED(key))
{}

} // namespace SST
