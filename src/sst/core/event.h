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

#ifndef SST_CORE_EVENT_H
#define SST_CORE_EVENT_H

#include "sst/core/activity.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"

#include <atomic>
#include <cinttypes>
#include <string>

namespace SST {

class Link;
class NullEvent;
class RankSync;
class ThreadSync;

/**
 * Base class for Events - Items sent across links to communicate between
 * components.
 */
class Event : public Activity
{
public:
    /**
       Base handler for event delivery.
     */
    using HandlerBase = SSTHandlerBase<void, Event*, false>;

    /**
       Used to create handlers for event delivery.  The callback
       function is expected to be in the form of:

         void func(Event* event)

       In which case, the class is created with:

         new Event::Handler<classname>(this, &classname::function_name)

       Or, to add static data, the callback function is:

         void func(Event* event, dataT data)

       and the class is created with:

         new Event::Handler<classname, dataT>(this, &classname::function_name, data)
     */
    template <typename classT, typename dataT = void>
    using Handler = SSTHandler<void, Event*, false, classT, dataT>;

    /** Type definition of unique identifiers */
    typedef std::pair<uint64_t, int> id_type;
    /** Constant, default value for id_types */
    static const id_type             NO_ID;

    Event() : Activity()
    {
        setPriority(EVENTPRIORITY);
#if __SST_DEBUG_EVENT_TRACKING__
        first_comp = "";
        last_comp  = "";
#endif
    }
    virtual ~Event();

    /** Clones the event in for the case of a broadcast */
    virtual Event* clone();


#ifdef __SST_DEBUG_EVENT_TRACKING__

    virtual void printTrackingInfo(const std::string& header, Output& out) const override
    {
        out.output(
            "%s Event first sent from: %s:%s (type: %s) and last received by %s:%s (type: %s)\n", header.c_str(),
            first_comp.c_str(), first_port.c_str(), first_type.c_str(), last_comp.c_str(), last_port.c_str(),
            last_type.c_str());
    }

    const std::string& getFirstComponentName() { return first_comp; }
    const std::string& getFirstComponentType() { return first_type; }
    const std::string& getFirstPort() { return first_port; }
    const std::string& getLastComponentName() { return last_comp; }
    const std::string& getLastComponentType() { return last_type; }
    const std::string& getLastPort() { return last_port; }

    void addSendComponent(const std::string& comp, const std::string& type, const std::string& port)
    {
        if ( first_comp == "" ) {
            first_comp = comp;
            first_type = type;
            first_port = port;
        }
    }
    void addRecvComponent(const std::string& comp, const std::string& type, const std::string& port)
    {
        last_comp = comp;
        last_type = type;
        last_port = port;
    }

#endif

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Activity::serialize_order(ser);
#ifndef SST_ENFORCE_EVENT_ORDERING
        ser& tag;
#endif
        ser& delivery_info;
#ifdef __SST_DEBUG_EVENT_TRACKING__
        ser& first_comp;
        ser& first_type;
        ser& first_port;
        ser& last_comp;
        ser& last_type;
        ser& last_port;
#endif
    }

protected:
    /**
     * Generates an ID that is unique across ranks, components and events.
     */
    id_type generateUniqueId();


private:
    friend class Link;
    friend class NullEvent;
    friend class RankSync;
    friend class ThreadSync;


    /** Cause this event to fire */
    void execute(void) override;

    /**
       This sets the information needed to get the event properly
       delivered for the next step of transfer.

       For links that don't go to a sync object, the tag is
       used as the order_tag field when that support is
       enabled.  If enforce_event_ordering is off, then the field does
       nothing.

       For links that are going to a sync, the tag is the
       remote_tag that will be used to look up the corresponding link
       on the other rank.
     */
    inline void setDeliveryInfo(LinkId_t tag, uintptr_t delivery_info)
    {
#ifdef SST_ENFORCE_EVENT_ORDERING
        order_tag = tag;
#else
        this->tag = tag;
#endif
        this->delivery_info = delivery_info;
    }

    /** Gets the link id used for delivery.  For use by SST Core only */
    inline Link* getDeliveryLink() { return reinterpret_cast<Link*>(delivery_info); }

    /** Gets the link id associated with this event.  For use by SST Core only */
    inline LinkId_t getTag(void) const
    {
#ifdef SST_ENFORCE_EVENT_ORDERING
        return order_tag;
#else
        return tag;
#endif
    }


    /** Holds the delivery information.  This is stored as a
      uintptr_t, but is actually a pointer converted using
      reinterpret_cast.  For events send on links connected to a
      Component/SubComponent, this holds a pointer to the delivery
      functor.  For events sent on links connected to a Sync object,
      this holds a pointer to the remote link to send the event on
      after synchronization.
    */
    uintptr_t delivery_info;

private:
    static std::atomic<uint64_t> id_counter;
    // If SST_ENFORCE_EVENT_ORDERING is turned on, then we'll use
    // Activity::order_tag as the tag.

    // The tag is used for two things:
    //
    // As the remote_tag for events that cross partitions.  This is
    // used to look up the corect link in the other rank (MPI/thread)
    //
    // As the data used for enforcing link ordering for all other
    // events (when that feature is on).
    //
    // For events that cross a rank boundary, the first link that
    // sends it to the RankSync, will put in the remote_tag, and the
    // link on the other rank that sends it to the TimeVortex will put
    // in the enforce_event_ordering data.
#ifndef SST_ENFORCE_EVENT_ORDERING
    LinkId_t tag;
#endif

#ifdef __SST_DEBUG_EVENT_TRACKING__
    std::string first_comp;
    std::string first_type;
    std::string first_port;
    std::string last_comp;
    std::string last_type;
    std::string last_port;
#endif

    ImplementVirtualSerializable(SST::Action)
};

/**
 * Null Event.  Does nothing.
 */
class NullEvent : public Event, public SST::Core::Serialization::serializable_type<NullEvent>
{
public:
    NullEvent() : Event() {}
    ~NullEvent() {}

    void execute(void) override;

private:
    ImplementSerializable(SST::NullEvent)
};
} // namespace SST

#endif // SST_CORE_EVENT_H
