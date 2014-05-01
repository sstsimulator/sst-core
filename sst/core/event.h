// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_EVENT_H
#define SST_CORE_CORE_EVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

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
        setPriority(50);
    }
    virtual ~Event() = 0;

    /** Cause this event to fire */
    void execute(void);

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

protected:
    /** Link used for delivery */
    Link* delivery_link;

    id_type generateUniqueId();

private:
    static uint64_t id_counter;
    LinkId_t link_id;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version );
};


/**
 * Null Event.  Does nothing.
 */
class NullEvent : public Event {
public:
    NullEvent() : Event() {}
    ~NullEvent() {}

    void execute(void);

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version );
};
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Event)
BOOST_CLASS_EXPORT_KEY(SST::NullEvent)

#endif // SST_CORE_EVENT_H
