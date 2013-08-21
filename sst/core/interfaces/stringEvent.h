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

#ifndef _STRINGEVENT_H
#define _STRINGEVENT_H
#include <sst/core/sst_types.h>

//#include <sst/core/activity.h>
#include <sst/core/debug.h>
#include <sst/core/event.h>
#include <sst/core/serialization.h>

namespace SST {
namespace Interfaces {

class StringEvent : public SST::Event {
public:
	StringEvent() {} // For serialization only

	StringEvent(const std::string &str) :
		SST::Event(), str(str)
	{ }

	StringEvent(const StringEvent &me) : SST::Event()
	{
		str = me.str;
		setDeliveryLink(me.getLinkId(), NULL);
	}
	StringEvent(const StringEvent *me) : SST::Event()
	{
		str = me->str;
		setDeliveryLink(me->getLinkId(), NULL);
	}

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

#endif /* _STRINGEVENT_H */
