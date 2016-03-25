// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_EVENT_H
#define SST_CORE_CORE_EVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <atomic>
#include <string>

#include <sst/core/activity.h>

namespace SST {

class Link;

/**
 * Base class for Events - Items sent across links to communicate between
 * components.
 */
class Event : public Activity {
public:

    /** Type definition of unique identifiers */
    typedef std::pair<uint64_t, int> id_type;
    /** Constatn, default value for id_types */
    static const id_type NO_ID;


    Event() : Activity() {
        setPriority(EVENTPRIORITY);
#if __SST_DEBUG_EVENT_TRACKING__
        first_comp = "";
        last_comp = "";
#endif
    }
    virtual ~Event() = 0;

    /** Cause this event to fire */
    void execute(void);

    /** Clones the event in for the case of a broadcast */
    virtual Event* clone();
    
    /** Sets the link id used for delivery.  For use by SST Core only */
    inline void setDeliveryLink(LinkId_t id, Link *link) {
        link_id = id;
        delivery_link = link;
    }

    /** Gets the link id used for delivery.  For use by SST Core only */
    inline Link* getDeliveryLink() {
        return delivery_link;
    }

    /** For use by SST Core only */
    inline void setRemoteEvent() {
        delivery_link = NULL;
    }

    /** Gets the link id associated with this event.  For use by SST Core only */
    inline LinkId_t getLinkId(void) const { return link_id; }


    /// Functor classes for Event handling
    class HandlerBase {
    public:
        /** Handler function */
        virtual void operator()(Event*) = 0;
        virtual ~HandlerBase() {}
    };


    /**
     * Event Handler class with user-data argument
     */
    template <typename classT, typename argT = void>
    class Handler : public HandlerBase {
    private:
        typedef void (classT::*PtrMember)(Event*, argT);
        classT* object;
        const PtrMember member;
        argT data;

    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         * @param data - Additional argument to pass to handler
         */
        Handler( classT* const object, PtrMember member, argT data ) :
            object(object),
            member(member),
            data(data)
        {}

        void operator()(Event* event) {
            (object->*member)(event,data);
        }
    };


    /**
     * Event Handler class with no user-data.
     */
    template <typename classT>
    class Handler<classT, void> : public HandlerBase {
        private:
            typedef void (classT::*PtrMember)(Event*);
            const PtrMember member;
            classT* object;

        public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         */
            Handler( classT* const object, PtrMember member ) :
                member(member),
                object(object)
            {}

            void operator()(Event* event) {
                (object->*member)(event);
            }
        };

    /** Virtual function to "pretty-print" this event.  Should be implemented by subclasses. */
    virtual void print(const std::string& header, Output &out) const {
        out.output("%s Generic Event to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
    }

#ifdef __SST_DEBUG_EVENT_TRACKING__

    virtual void printTrackingInfo(const std::string& header, Output &out) const {
        out.output("%s Event first sent from: %s:%s (type: %s) and last received by %s:%s (type: %s)\n", header.c_str(),
                   first_comp.c_str(),first_port.c_str(),first_type.c_str(),
                   last_comp.c_str(),last_port.c_str(),last_type.c_str());
    }

    const std::string& getFirstComponentName() { return first_comp; }
    const std::string& getFirstComponentType() { return first_type; }
    const std::string& getFirstPort() { return first_port; }
    const std::string& getLastComponentName() { return last_comp; }
    const std::string& getLastComponentType() { return last_type; }
    const std::string& getLastPort() { return last_port; }
    
    void addSendComponent(const std::string& comp, const std::string& type, const std::string& port) {
        if ( first_comp == "" ) { 
            first_comp = comp;
            first_type = type;
            first_port = port;
        }
    }
    void addRecvComponent(const std::string& comp, const std::string& type, const std::string& port) {
        last_comp = comp;
        last_type = type;
        last_port = port;
    }

#endif

    void serialize_order(SST::Core::Serialization::serializer &ser){
        Activity::serialize_order(ser);
        ser & link_id;
#ifdef __SST_DEBUG_EVENT_TRACKING__
        ser & first_comp;
        ser & first_type;
        ser & first_port;
        ser & last_comp;
        ser & last_type;
        ser & last_port;
#endif

    }    

    
protected:
    /** Link used for delivery */
    Link* delivery_link;

    /**
     * Generates an ID that is unique across ranks, components and events.
     */
    id_type generateUniqueId();

private:
    static std::atomic<uint64_t> id_counter;
    LinkId_t link_id;

#ifdef __SST_DEBUG_EVENT_TRACKING__
    std::string first_comp;
    std::string first_type;
    std::string first_port;
    std::string last_comp;
    std::string last_type;
    std::string last_port;
#endif
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version );
};


/**
 * Null Event.  Does nothing.
 */
class NullEvent : public Event, public SST::Core::Serialization::serializable_type<NullEvent> {
public:
    NullEvent() : Event() {}
    ~NullEvent() {}

    void execute(void);

    virtual void print(const std::string& header, Output &out) const {
        out.output("%s NullEvent to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
    }

    

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version );

    ImplementSerializable(SST::NullEvent)

};
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Event)
BOOST_CLASS_EXPORT_KEY(SST::NullEvent)

#endif // SST_CORE_EVENT_H
