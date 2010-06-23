// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_EVENT_H
#define SST_EVENT_H

#include <sst/core/eventFunctor.h>
#include <sst/core/activity.h>
#include <sst/core/link.h>


namespace SST {

class Link;
    
// #include <sst/core/sst.h>


class Event : public Activity {
public:

    Event() : Activity() {
	setPriority(50);
    }
    virtual ~Event() = 0;

    inline void execute(void) {
 	delivery_link->deliverEvent(this);
    }

    void setDeliveryLink(LinkId_t id, Link * link) {
	link_id = id;
	delivery_link = link;
    }

    void setRemoteEvent() {
	delivery_link = NULL;
    }

    LinkId_t getLinkId(void) const { return link_id; }

protected:
    Link* delivery_link;
    
private:

    LinkId_t link_id;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Activity);
        ar & BOOST_SERIALIZATION_NVP(delivery_link);
        ar & BOOST_SERIALIZATION_NVP(link_id);
    }
};

class NullEvent : public Event {

public:
    NullEvent() : Event() {}
    ~NullEvent() {}

    inline void execute(void) {
	delivery_link->deliverEvent(NULL);
	delete this;
    }
};
}

#endif // SST_EVENT_H
