
#ifndef _REGISTER_COMPONENT_ON_NETWORK_EVENT_H
#define _REGISTER_COMPONENT_ON_NETWORK_EVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/debug.h>
#include <sst/core/activity.h>

namespace SST {
namespace Interfaces {

class RegisterComponentOnNetwork : public SST::Event {
public:
	RegisterComponentOnNetworkEvent() {} // For serialization only

	RegisterComponentOnNetworkEvent(const std::string &componentNetworkName) :
		SST::Event(), componentNetworkName
	{ }

	RegisterComponentOnNetworkEvent(const RegisterComponentOnNetworkEvent &me) : SST::Event()
	{
		componentNetworkName = me.componentNetworkName;
		setDeliveryLink(me.getLinkId(), NULL);
	}
	RegisterComponentOnNetworkEvent(const RegisterComponentOnNetworkEvent *me) : SST::Event()
	{
		componentNetworkName = me->componentNetworkName;
		setDeliveryLink(me->getLinkId(), NULL);
	}

	const std::string& getComponentNetworkName(void) { return componentNetworkName; }

private:
	std::string componentNetworkName;

	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
		ar & BOOST_SERIALIZATION_NVP(componentNetworkName);
	}
};

}
}

#endif /* _RegisterComponentOnNetworkEvent_H */
