// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORETEST_SUBCOMPONENT_H
#define SST_CORE_CORETEST_SUBCOMPONENT_H

#include "sst/core/component.h"
#include "sst/core/link.h"
#include "sst/core/subcomponent.h"

#include <vector>

namespace SST {
namespace CoreTestSubComponent {

/*

  CoreTestSubComponent will test the various ways to use SubComponents.
  The SubComponents will be loadable as both named and unnamed
  SubComponets.  When an unname SubComponent is used, it inherits the
  port interface from the BaseComponent that created it.  A named
  SubComponent owns it's own ports and will mask the ports from any
  BaseComponent higher in the call tree.

  Each BaseComponent will have a port(s) that may or may not be used
  depending on the configuration.
 */

class SubCompInterface : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CoreTestSubComponent::SubCompInterface)

    SubCompInterface(ComponentId_t id) : SubComponent(id) {}
    SubCompInterface(ComponentId_t id, Params& UNUSED(params)) : SubComponent(id) {}
    SubCompInterface() : SubComponent() {}
    virtual ~SubCompInterface() {}
    virtual void clock(SST::Cycle_t) {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::SubComponent::serialize_order(ser);
    }
    ImplementSerializable(SST::CoreTestSubComponent::SubCompInterface)
};

class SubCompSlotInterface : public SubCompInterface
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::CoreTestSubComponent::SubCompSlotInterface,
                                              SST::CoreTestSubComponent::SubCompInterface)

    SST_ELI_DOCUMENT_PARAMS(
        {"num_subcomps","Number of anonymous SubComponents to load.  Ignored if using name SubComponents.","1"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"test", "Just a test port", { "coreTestMessageGeneratorComponent.coreTestMessage", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    SubCompSlotInterface(ComponentId_t id) : SubCompInterface(id) {}
    SubCompSlotInterface(ComponentId_t id, Params& UNUSED(params)) : SubCompInterface(id) {}
    virtual ~SubCompSlotInterface() {}

    SubCompSlotInterface() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override { SubCompInterface::serialize_order(ser); }
    ImplementSerializable(SST::CoreTestSubComponent::SubCompSlotInterface)
};

/* Our trivial component */
class SubComponentLoader : public Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        SubComponentLoader,
        "coreTestElement",
        "SubComponentLoader",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Demonstrates subcomponents",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"clock", "Clock Rate", "1GHz"},
        {"unnamed_subcomponent", "Unnamed SubComponent to load.  If empty, then a named subcomponent is loaded", ""},
        {"num_subcomps","Number of anonymous SubComponents to load.  Ignored if using name SubComponents.","1"},
        {"verbose", "Verbosity level", "0"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"totalSent", "# of total messages sent", "", 1},
    )

    // This ports will be used only by unnamed SubComponents
    SST_ELI_DOCUMENT_PORTS(
        {"port%d", "Sending or Receiving Port(s)", { "coreTestMessageGeneratorComponent.coreTestMessage", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"mySubComp", "Test slot", "SST::CoreTestSubComponent::SubCompInterface" }
    )

    SubComponentLoader(ComponentId_t id, SST::Params& params);

    SubComponentLoader() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::Component::serialize_order(ser);
        SST_SER(subComps)
    }
    ImplementSerializable(SST::CoreTestSubComponent::SubComponentLoader)

private:
    bool                           tick(SST::Cycle_t);
    std::vector<SubCompInterface*> subComps;
};

/* Our example subcomponents */
class SubCompSlot : public SubCompSlotInterface
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        SubCompSlot,
        "coreTestElement",
        "SubCompSlot",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Subcomponent which is just a wrapper for the actual SubComponent to be used",
        SST::CoreTestSubComponent::SubCompSlotInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"unnamed_subcomponent", "Unnamed SubComponent to load.  If empty, then a named subcomponent is loaded", ""},
        {"verbose", "Verbosity level", "0"},
    )

    // Only used when loading unnamed SubComponents
    SST_ELI_DOCUMENT_PORTS(
        {"slot_port%d", "Port(s) to send or receive on", { "coreTestMessageGeneratorComponent.coreTestMessage", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"mySubCompSlot", "Test slot", "SST::CoreTestSubComponent::SubCompInterface" }
    )

    SubCompSlot() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SubCompSlotInterface::serialize_order(ser);
        SST_SER(subComps)
    }
    ImplementSerializable(SST::CoreTestSubComponent::SubCompSlot)

private:
    std::vector<SubCompInterface*> subComps;

public:
    SubCompSlot(ComponentId_t id, Params& params);
    // Direct load
    SubCompSlot(ComponentId_t id, std::string unnamed_sub);

    ~SubCompSlot() {}
    void clock(Cycle_t) override;
};

// Add in some extra levels of ELI hierarchy for testing
class SubCompSendRecvInterface : public SubCompInterface
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::CoreTestSubComponent::SubCompSendRecvInterface,
                                              SST::CoreTestSubComponent::SubCompInterface)

    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
        SubCompSendRecvInterface,
        "coreTestElement",
        "SubCompSendRecv",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Default Subcomponent for ELI testing only",
        SST::CoreTestSubComponent::SubCompSendRecvInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"port_name", "Name of port to connect to", ""},
        {"sendCount", "Number of Messages to Send", "10"},
        {"verbose",   "Verbosity level", "0"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"sendPort", "Sending Port", { "coreTestMessageGeneratorComponent.coreTestMessage", "" } },
        // The following port is a test to make sure that when loaded
        // anonymously, a port that's named the same as one of its
        // parent's ports doesn't conflict.
        {"slot_port%d", "This is just a test port that duplicates a port from the SubComponent that will instance it", { "", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"numRecv", "# of msgs recv", "", 1},
    )


    SubCompSendRecvInterface(ComponentId_t id) : SubCompInterface(id) {}
    SubCompSendRecvInterface(ComponentId_t id, Params& UNUSED(params)) : SubCompInterface(id) {}
    virtual ~SubCompSendRecvInterface() {}

    SubCompSendRecvInterface() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override { SubCompInterface::serialize_order(ser); }
    ImplementSerializable(SST::CoreTestSubComponent::SubCompSendRecvInterface)
};

class SubCompSender : public SubCompSendRecvInterface
{
public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
        SubCompSender,
        "coreTestElement",
        "SubCompSender",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Sending Subcomponent",
        SST::CoreTestSubComponent::SubCompSendRecvInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        SST_ELI_DELETE_STAT("numRecv"),
        {"numSent", "# of msgs sent", "", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"sendPort", "Sending Port", { "coreTestMessageGeneratorComponent.coreTestMessage", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"test_slot", "Test slot", "" }
    )

    SubCompSender() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SubCompSendRecvInterface::serialize_order(ser);
        SST_SER(link)
        SST_SER(nToSend)
        SST_SER(nMsgSent)
        SST_SER(totalMsgSent)
        SST_SER(out)
    }
    ImplementSerializable(SST::CoreTestSubComponent::SubCompSender)

private:
    Statistic<uint32_t>* nMsgSent;
    Statistic<uint32_t>* totalMsgSent;
    uint32_t             nToSend;
    SST::Link*           link;
    SST::Output*         out;

public:
    SubCompSender(ComponentId_t id, Params& params);
    // Direct API
    SubCompSender(ComponentId_t id, uint32_t nToSend, const std::string& port_name);
    ~SubCompSender() {}
    void clock(Cycle_t) override;
};

class SubCompReceiver : public SubCompSendRecvInterface
{

public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
        SubCompReceiver,
        "coreTestElement",
        "SubCompReceiver",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Receiving Subcomponent",
        SST::CoreTestSubComponent::SubCompSendRecvInterface
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PARAMS(
        SST_ELI_DELETE_PARAM("sendCount")
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
        SST_ELI_DELETE_PORT("sendPort"),
        {"recvPort", "Receiving Port", { "coreTestMessageGeneratorComponent.coreTestMessage", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        SST_ELI_DELETE_SUBCOMPONENT_SLOT("test_slot")
    )

    SubCompReceiver() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SubCompSendRecvInterface::serialize_order(ser);
        SST_SER(link)
        SST_SER(nMsgReceived)
        SST_SER(out);
    }
    ImplementSerializable(SST::CoreTestSubComponent::SubCompReceiver)

private:
    Statistic<uint32_t>* nMsgReceived;
    SST::Link*           link;
    SST::Output*         out;

    void handleEvent(SST::Event* ev);

public:
    SubCompReceiver(ComponentId_t id, Params& params);
    SubCompReceiver(ComponentId_t id, std::string port);
    ~SubCompReceiver() {}
    void clock(Cycle_t) override;
};

} // namespace CoreTestSubComponent
} // namespace SST

#endif // SST_CORE_CORETEST_SUBCOMPONENT_H
