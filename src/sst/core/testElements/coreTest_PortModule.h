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

#ifndef SST_CORE_CORETEST_PORTMODULE_H
#define SST_CORE_CORETEST_PORTMODULE_H

#include "sst/core/component.h"
#include "sst/core/event.h"
#include "sst/core/subcomponent.h"

namespace SST {
namespace CoreTestPortModule {

class PortSubComponent;

// Event passed between test components.  The variable modified can
// be set to true as a test of modifying events in PortModules.
class PortModuleEvent : public Event
{
public:
    bool modified = false;
    bool last     = false;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Event::serialize_order(ser);
        ser& modified;
        ser& last;
    }

    ImplementSerializable(SST::CoreTestPortModule::PortModuleEvent);
};

// Event that is created by a PortModule to notify the receiving
// component that the event was dropped
class PortModuleAckEvent : public Event
{
public:
    void serialize_order(SST::Core::Serialization::serializer& ser) override { Event::serialize_order(ser); }

    ImplementSerializable(SST::CoreTestPortModule::PortModuleAckEvent);
};


class TestPortModule : public SST::PortModule
{
public:
    SST_ELI_REGISTER_PORTMODULE(
        TestPortModule,
        "coreTestElement",
        "portmodules.test",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "PortModule used for testing port module functionality")

    // TODO Need stats

    SST_ELI_DOCUMENT_PARAMS(
        { "modify", "Set to true to have PortModule mark event as modfied. NOTE: only 1 of modify, drop or replace can be set to true.", "false" },
        { "drop", "Set to true to have PortModule drop events. NOTE: only 1 of modify, drop, or replace can be set to true.", "false" },
        { "replace", "Set to true to have PortModule drop events and deliver an Ack event instead. NOTE: only 1 of modify, drop or replace can be set to true.", "false" },
        { "install_on_send",  "Controls whether the PortModule is installed on the send or receive side.  Set to true to register on send and false to register on recieve.", "false" },
    )

    TestPortModule(Params& params);

    // For serialization only
    TestPortModule() = default;
    ~TestPortModule() {}

    void eventSent(uintptr_t key, Event*& ev) override;
    void interceptHandler(uintptr_t key, Event*& data, bool& cancel) override;

    bool installOnReceive() override { return !install_on_send_; }
    bool installOnSend() override { return install_on_send_; }

private:
    bool install_on_send_ = false;
    bool modify_          = false;
    bool drop_            = false;
    bool replace_         = false;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        ser& install_on_send_;
        ser& modify_;
        ser& drop_;
        ser& replace_;
    }
    ImplementSerializable(SST::CoreTestPortModule::TestPortModule)
};

class coreTestPortModuleComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestPortModuleComponent,
        "coreTestElement",
        "coreTestPortModuleComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Component to test PortModule functionality",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PORTS(
        {"left", "Link to the left. Will only receive on left port.  If nothing is attached to the left port, the component will send sendcount events.", { "" } },
        {"right", "Link to the right. Will only send on right port.  If nothing is connect to the right port, the component will check the types of the events recieved.", { "" } }  
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "sendcount", "Events to send if send is set to true", "20"},
        { "use_subcomponent", "Set to true to use a subcomponent to hook up the ports", "false"},
        { "repeat_last",  "When set to true, will keep sending \"last\" events until the simulation terminates.  This is to support test of the RandomDropPortModule which doesn't check event types or values so will not automatically pass through the event marked last.", "false" },
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"port_slot", "SLot for loading subcomponent to test shared ports", "" }
    )

    coreTestPortModuleComponent(SST::ComponentId_t id, SST::Params& params);

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::Component::serialize_order(ser);
        ser& sendcount_;
        ser& sub_;
        ser& repeat_last_;
        ser& left_;
        ser& right_;
    }
    ImplementSerializable(SST::CoreTestPortModule::coreTestPortModuleComponent)

private:
    int  sendcount_   = 20;
    bool repeat_last_ = false;

    Link* left_;
    Link* right_;

    PortSubComponent* sub_ = nullptr;

    coreTestPortModuleComponent() {}                                 // for serialization only
    coreTestPortModuleComponent(const coreTestPortModuleComponent&); // do not implement
    void operator=(const coreTestPortModuleComponent&);              // do not implement

    bool tick(SST::Cycle_t);
    void handleEvent(SST::Event* ev);
    void handleEventLast(SST::Event* ev);
};

class PortSubComponent : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CoreTestPortModule::PortSubComponent)

    SST_ELI_REGISTER_SUBCOMPONENT(
        PortSubComponent,
        "coreTestElement",
        "PortSubComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Subcomponent used to test putting PortModules on shared ports",
        SST::CoreTestPortModule::PortSubComponent
    )

    PortSubComponent(ComponentId_t id, Params& params);
    PortSubComponent() = default; // For serialization
    ~PortSubComponent() {}

    Link* getLeft() { return left_; }
    Link* getRight() { return right_; }

private:
    Link* left_;
    Link* right_;

    void dummy_handler(SST::Event* UNUSED(ev)) {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::SubComponent::serialize_order(ser);
        ser& left_;
        ser& right_;
    }
    ImplementSerializable(SST::CoreTestPortModule::PortSubComponent)
};

} // namespace CoreTestPortModule
} // namespace SST

#endif // SST_CORE_CORETEST_PORTMODULE_H
