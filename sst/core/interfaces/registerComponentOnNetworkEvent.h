
#ifndef _REGISTER_COMPONENT_ON_NETWORK_EVENT_H
#define _REGISTER_COMPONENT_ON_NETWORK_EVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/debug.h>
#include <sst/core/activity.h>

#include <cstdint>

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

}
}

#endif /* _RegisterComponentOnNetworkEvent_H */
