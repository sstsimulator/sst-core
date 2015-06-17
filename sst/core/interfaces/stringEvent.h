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

#ifndef INTERFACES_STRINGEVENT_H
#define INTERFACES_STRINGEVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <sst/core/event.h>

namespace SST {
namespace Interfaces {

/**
 * Simple event to pass strings between components
 */
class StringEvent : public SST::Event {
public:
	StringEvent() {} // For serialization only

    /** Create a new StringEvent
     * @param str - The String contents of this event
     */
	StringEvent(const std::string &str) :
		SST::Event(), str(str)
	{ }

    /** Copies an existing StringEvent */
	StringEvent(const StringEvent &me) : SST::Event()
	{
		str = me.str;
		setDeliveryLink(me.getLinkId(), NULL);
	}

    /** Copies an existing StringEvent */
	StringEvent(const StringEvent *me) : SST::Event()
	{
		str = me->str;
		setDeliveryLink(me->getLinkId(), NULL);
	}

    /** Returns the contents of this Event */
	const std::string& getString(void) { return str; }

private:
	std::string str;

	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
		ar & BOOST_SERIALIZATION_NVP(str);
	}
};

} //namespace Interfaces
} //namespace SST

#endif /* INTERFACES_STRINGEVENT_H */
