// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
    virtual ~SubCompInterface() {}
    virtual void clock(SST::Cycle_t) {}
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

private:
    bool                           tick(SST::Cycle_t);
    std::vector<SubCompInterface*> subComps;
};

/* Our example subcomponents */
class SubCompSlot : public SubCompSlotInterface
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SubCompSlot,
        "coreTestElement",
        "SubCompSlot",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Subcomponent which is just a wrapper for the actual SubComponent to be used",
        SST::CoreTestSubComponent::SubCompSlotInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"unnamed_subcomponent", "Unnamed SubComponent to load.  If empty, then a named subcomponent is loaded", ""}
    )

    // Only used when loading unnamed SubComponents
    SST_ELI_DOCUMENT_PORTS(
        {"slot_port%d", "Port(s) to send or receive on", { "coreTestMessageGeneratorComponent.coreTestMessage", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"mySubCompSlot", "Test slot", "SST::CoreTestSubComponent::SubCompInterface" }
    )

private:
    std::vector<SubCompInterface*> subComps;

public:
    SubCompSlot(ComponentId_t id, Params& params);
    // Direct load
    SubCompSlot(ComponentId_t id, std::string unnamed_sub);

    ~SubCompSlot() {}
    void clock(Cycle_t);
};

// Add in some extra levels of ELI hierarchy for testing
class SubCompSendRecvInterface : public SubCompInterface
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::CoreTestSubComponent::SubCompSendRecvInterface,
                                              SST::CoreTestSubComponent::SubCompInterface)

    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SubCompSendRecvInterface,
        "coreTestElement",
        "SubCompSendRecv",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Default Subcomponent for ELI testing only",
        SST::CoreTestSubComponent::SubCompSendRecvInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"port_name", "Name of port to connect to", ""},
        {"sendCount", "Number of Messages to Send", "10"}
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
};

class SubCompSender : public SubCompSendRecvInterface
{
public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
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

private:
    Statistic<uint32_t>* nMsgSent;
    Statistic<uint32_t>* totalMsgSent;
    uint32_t             nToSend;
    SST::Link*           link;

public:
    SubCompSender(ComponentId_t id, Params& params);
    // Direct API
    SubCompSender(ComponentId_t id, uint32_t nToSend, const std::string& port_name);
    ~SubCompSender() {}
    void clock(Cycle_t);
};

class SubCompReceiver : public SubCompSendRecvInterface
{

public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
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

private:
    Statistic<uint32_t>* nMsgReceived;
    SST::Link*           link;

    void handleEvent(SST::Event* ev);

public:
    SubCompReceiver(ComponentId_t id, Params& params);
    SubCompReceiver(ComponentId_t id, std::string port);
    ~SubCompReceiver() {}
    void clock(Cycle_t);
};

} // namespace CoreTestSubComponent
} // namespace SST

#endif // SST_CORE_CORETEST_SUBCOMPONENT_H
