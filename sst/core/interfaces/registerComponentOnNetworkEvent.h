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

#ifndef INTERFACES_REGISTER_COMPONENT_ON_NETWORK_EVENT_H
#define INTERFACES_REGISTER_COMPONENT_ON_NETWORK_EVENT_H
#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

//#include <sst/core/activity.h>
#include <sst/core/debug.h>
#include <sst/core/event.h>

#include <sst/sst_stdint.h>

namespace SST {
namespace Interfaces {

class RegisterComponentOnNetworkEvent : public SST::Event {
public:
	RegisterComponentOnNetworkEvent() {} // For serialization only

	RegisterComponentOnNetworkEvent(const std::string &componentNetName, uint64_t netID) :
		SST::Event(), componentNetworkName(componentNetName), networkID(netID)
	{ }

	RegisterComponentOnNetworkEvent(const RegisterComponentOnNetworkEvent &me) : SST::Event()
	{
		componentNetworkName = me.componentNetworkName;
		networkID = me.networkID;

		setDeliveryLink(me.getLinkId(), NULL);
	}
	RegisterComponentOnNetworkEvent(const RegisterComponentOnNetworkEvent *me) : SST::Event()
	{
		componentNetworkName = me->componentNetworkName;
		networkID = me->networkID;

		setDeliveryLink(me->getLinkId(), NULL);
	}

	const std::string& getComponentNetworkName(void) { return componentNetworkName; }
	uint64_t getComponentNetworkID(void) { return networkID; }

private:
	std::string componentNetworkName;
	uint64_t networkID;

	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
		ar & BOOST_SERIALIZATION_NVP(componentNetworkName);
		ar & BOOST_SERIALIZATION_NVP(networkID);
	}
};

} //namespace Interfaces
} //namespace SST

#endif /* INTERFACES_REGISTER_COMPONENT_ON_NETWORK_EVENT_H */
