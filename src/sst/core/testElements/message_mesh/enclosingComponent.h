// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
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

#include <cstddef>
#include <cstdint>
#include <vector>

namespace SST::CoreTest::MessageMesh {

class PortInterface : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CoreTest::MessageMesh::PortInterface)

    explicit PortInterface(ComponentId_t id) :
        SubComponent(id)
    {}
    PortInterface() {}
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

    /**
       Used to create checkpointable handlers to notify the component
       when a message has arrived. endpoint when the The callback
       function is expected to be in the form of:

         void func(Event* ev)

       In which case, the class is created with:

         new PortInterface::Handler2<classname, &classname::function_name>(this)

       Or, to add static data, the callback function is:

         void func(Event* ev, dataT data)

       and the class is created with:

         new PortInterface::Handler2<classname, &classname::function_name, dataT>(this, data)
    */
    template <typename classT, auto funcT, typename dataT = void>
    using Handler2 = SSTHandler2<void, Event*, classT, dataT, funcT>;

    virtual void setNotifyOnReceive(HandlerBase* functor) { functor_ = functor; }

    virtual void send(MessageEvent* ev, size_t index) = 0;

    virtual size_t getPortCount() = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SubComponent::serialize_order(ser);
        SST_SER(functor_);
    }
    ImplementVirtualSerializable(SST::CoreTest::MessageMesh::PortInterface);

protected:
    HandlerBase* functor_ = nullptr;
};

class RouteInterface : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CoreTest::MessageMesh::RouteInterface, std::vector<PortInterface*>&, int)

    explicit RouteInterface(ComponentId_t id) :
        SubComponent(id)
    {}
    RouteInterface() {}
    virtual ~RouteInterface() {}

    virtual void send(MessageEvent* ev, int incoming_port) = 0;

    virtual void sendInitialEvents(int mod) = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override { SubComponent::serialize_order(ser); }
    ImplementVirtualSerializable(SST::CoreTest::MessageMesh::RouteInterface);
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
        {"id", "Id for this component", ""},
        {"mod", "Port modulus to restrict number of initial events", "1"},
        {"verbose", "Print message count at end of simulation", "True"},
        {"stats", "Statistics per component", "0"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"stat", "Test statistic", "count", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"ports", "Slot that the ports objects go in", "SST::CoreTest::MessageMesh::PortInterface" },
        {"route", "Slot that the route object goes in", "SST::CoreTest::MessageMesh::RouteInterface" }
    )

    SST_ELI_IS_CHECKPOINTABLE()

    EnclosingComponent(ComponentId_t id, Params& params);
    EnclosingComponent() {}
    ~EnclosingComponent() {}

    void setup() override;
    void finish() override;

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTest::MessageMesh::EnclosingComponent);

private:
    void handleEvent(SST::Event* ev, int port);

    std::vector<PortInterface*> ports_;
    RouteInterface*             route_;

    // Measuring statistics per component
    std::vector<Statistic<uint64_t>*> stats_;

    int  my_id_;
    int  message_count_ = 0;
    int  mod_           = 1;
    bool verbose_       = true;
};

// SubComponents

class PortSlot : public PortInterface
{
public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
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

    SST_ELI_IS_CHECKPOINTABLE()

    PortSlot(ComponentId_t id, Params& params);
    PortSlot() {}
    ~PortSlot() {}

    void   send(MessageEvent* ev, size_t index) override { port_->send(ev, index); }
    void   setNotifyOnReceive(HandlerBase* functor) override { port_->setNotifyOnReceive(functor); }
    size_t getPortCount() override { return port_->getPortCount(); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTest::MessageMesh::PortSlot);


private:
    PortInterface* port_;
};


class MessagePort : public PortInterface
{
public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
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
        {"port%d", "Port to send or receive on", { "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    MessagePort(ComponentId_t id, Params& params);
    MessagePort() {}
    ~MessagePort() {}

    void   send(MessageEvent* ev, size_t index) override;
    void   handleEvent(Event* ev);
    size_t getPortCount() override { return links_.size(); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTest::MessageMesh::MessagePort);

private:
    std::vector<Link*> links_;
};

class RouteMessage : public RouteInterface
{
public:
    // REGISTER THIS SUB-COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_SUBCOMPONENT(
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
      {"msg_count", "Message counter", "count", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    SST_ELI_IS_CHECKPOINTABLE()

    RouteMessage(ComponentId_t id, Params& params, std::vector<PortInterface*>& ports, int my_id);
    RouteMessage() {}
    ~RouteMessage() {}

    void send(MessageEvent* ev, int incoming_port) override;

    void sendInitialEvents(int mod) override;

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTest::MessageMesh::RouteMessage);

private:
    std::vector<PortInterface*> ports_;
    std::vector<size_t>         counts_;
    int                         my_id_;
    SST::RNG::Random*           rng_;

    Statistic<uint64_t>* mcnt_;
};

} // namespace SST::CoreTest::MessageMesh

#endif // SST_CORE_CORETEST_MESSAGEMESH_ENCLOSINGCOMPONENT_H
