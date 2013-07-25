// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_EVENT_H
#define SST_CORE_EVENT_H

//#include <sst/core/eventFunctor.h>
#include <sst/core/sst_types.h>
#include <sst/core/debug.h>
#include <sst/core/activity.h>

namespace SST {

class Link;

class Event : public Activity {
public:
    Event() : Activity() {
	setPriority(50);
    }
    virtual ~Event() = 0;

    void execute(void);

    inline void setDeliveryLink(LinkId_t id, Link *link) {
	link_id = id;
	delivery_link = link;
    }

    inline void setRemoteEvent() {
	delivery_link = NULL;
    }

    inline LinkId_t getLinkId(void) const { return link_id; }


    // Functor classes for Event handling
    class HandlerBase {
    public:
	virtual void operator()(Event*) = 0;
	virtual ~HandlerBase() {}
    };
    

    template <typename classT, typename argT = void>
    class Handler : public HandlerBase {
    private:
	typedef void (classT::*PtrMember)(Event*, argT);
	classT* object;
	const PtrMember member;
	argT data;
	
    public:
	Handler( classT* const object, PtrMember member, argT data ) :
	    object(object),
	    member(member),
	    data(data)
	{}

	    void operator()(Event* event) {
		(object->*member)(event,data);
	    }
    };
    
    template <typename classT>
    class Handler<classT, void> : public HandlerBase {
    private:
	typedef void (classT::*PtrMember)(Event*);
	const PtrMember member;
	classT* object;
	
    public:
	Handler( classT* const object, PtrMember member ) :
  	  member(member),
	  object(object)
	{}

	    void operator()(Event* event) {
		(object->*member)(event);
	    }
    };

    virtual void print(const std::string& header, Output &out) const {
        out.output("%s Generic Event to be delivered at %"PRIu64" with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
    }

protected:
    Link* delivery_link;
    
private:
    LinkId_t link_id;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version );
};


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
}

BOOST_CLASS_EXPORT_KEY(SST::Event)
BOOST_CLASS_EXPORT_KEY(SST::NullEvent)

#endif // SST_EVENT_H
