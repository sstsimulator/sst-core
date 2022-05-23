// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORETEST_MESSAGEMESH_ENCLOSINGCOMPONENT_H
#define SST_CORE_CORETEST_MESSAGEMESH_ENCLOSINGCOMPONENT_H

#include "sst/core/testElements/message_mesh/messageEvent.h"

#include "sst/core/component.h"
#include "sst/core/event.h"
#include "sst/core/link.h"
#include "sst/core/rng/rng.h"
#include "sst/core/ssthandler.h"
#include "sst/core/subcomponent.h"

namespace SST {
namespace CoreTest {
namespace MessageMesh {

class PortInterface : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CoreTest::MessageMesh::PortInterface)

    PortInterface(ComponentId_t id) : SubComponent(id) {}
    virtual ~PortInterface() {}

    /**
       Base handler for event delivery.
     */
    using HandlerBase = SSTHandlerBase<void, Event*>;

    /**
       Used to create handlers to notify the component when a message
       has arrived. endpoint when the The callback function is
       expected to be in the form of:

         void func(Event* ev)

       In which case, the class is created with:

         new PortInterface::Handler<classname>(this, &classname::function_name)

       Or, to add static data, the callback function is:

         void func(Event* ev, dataT data)

       and the class is created with:

         new PortInterface::Handler<classname, dataT>(this, &classname::function_name, data)
    */
    template <typename classT, typename dataT = void>
    using Handler = SSTHandler<void, Event*, classT, dataT>;

    virtual void setNotifyOnReceive(HandlerBase* functor) { rFunctor = functor; }

    virtual void send(MessageEvent* ev) = 0;

protected:
    HandlerBase* rFunctor;
};

class RouteInterface : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CoreTest::MessageMesh::RouteInterface, const std::vector<PortInterface*>&, int)

    RouteInterface(ComponentId_t id) : SubComponent(id) {}
    virtual ~RouteInterface() {}

    virtual void send(MessageEvent* ev, int incoming_port) = 0;
};

class EnclosingComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        EnclosingComponent,
        "coreTestElement",
        "message_mesh.enclosing_component",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Base element that encloses the SubComponents that actually provide the functionality",
        COMPONENT_CATEGORY_NETWORK
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"id", "Id for this componentd", ""},
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"ports", "Slot that the ports objects go in", "SST::CoreTest::MessageMesh::PortInterface" },
        {"route", "Slot that the ports objects go in", "SST::CoreTest::MessageMesh::RouteInterface" }
    )

    EnclosingComponent(ComponentId_t id, Params& params);

    void setup();
    void finish();

private:
    void handleEvent(SST::Event* ev, int port);

    std::vector<PortInterface*> ports;
    RouteInterface*             route;

    int my_id;
    int message_count;
};

// SubComponents

class PortSlot : public PortInterface
{
public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        PortSlot,
        "coreTestElement",
        "message_mesh.port_slot",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SubComponent implementing PortInterface that simply defers to another loaded PortInterface",
        SST::CoreTest::MessageMesh::PortInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"port", "Slot to load the real PortInterface object", "SST::CoreTest::MessageMesh::PortInterface" }
    )

    PortSlot(ComponentId_t id, Params& params);
    ~PortSlot() {}

    void send(MessageEvent* ev) override { port->send(ev); }
    void setNotifyOnReceive(HandlerBase* functor) override { port->setNotifyOnReceive(functor); }

private:
    PortInterface* port;
};


class MessagePort : public PortInterface
{
public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        MessagePort,
        "coreTestElement",
        "message_mesh.message_port",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SubComponent implementing PortInterface for sending and receiving messages",
        SST::CoreTest::MessageMesh::PortInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
        {"port", "Port to send or receive on", { "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    MessagePort(ComponentId_t id, Params& params);
    ~MessagePort() {}

    void send(MessageEvent* ev);
    void handleEvent(Event* ev);

private:
    Link* link;
};

class RouteMessage : public RouteInterface
{
public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        RouteMessage,
        "coreTestElement",
        "message_mesh.route_message",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SubComponent implementing message routing",
        SST::CoreTest::MessageMesh::RouteInterface
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    RouteMessage(ComponentId_t id, Params& params, const std::vector<PortInterface*>& ports, int my_id);
    ~RouteMessage() {}

    void send(MessageEvent* ev, int incoming_port) override;

private:
    const std::vector<PortInterface*> ports;
    int                               my_id;
    SST::RNG::Random*                 rng;
};

} // namespace MessageMesh
} // namespace CoreTest
} // namespace SST

#endif // SST_CORE_CORETEST_MESSAGEMESH_ENCLOSINGCOMPONENT_H
