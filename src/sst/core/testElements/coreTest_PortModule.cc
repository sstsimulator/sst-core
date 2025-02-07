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

#include "sst_config.h"

#include "sst/core/testElements/coreTest_PortModule.h"

#include "sst/core/event.h"

using namespace SST;
using namespace SST::CoreTestPortModule;

/********* TestPortModule **********/

TestPortModule::TestPortModule(Params& params) : PortModule()
{
    modify_          = params.find<bool>("modify", "false");
    drop_            = params.find<bool>("drop", "false");
    replace_         = params.find<bool>("replace", "false");
    install_on_send_ = params.find<bool>("install_on_send", "false");

    int count = 0;
    if ( modify_ ) count++;
    if ( drop_ ) count++;
    if ( replace_ ) count++;

    if ( count > 1 ) {
        getSimulationOutput().fatal(
            CALL_INFO_LONG, 1, "ERROR: Only one of the parameters modify, drop and replace can be set to true\n");
    }
}

void
TestPortModule::eventSent(uintptr_t UNUSED(key), Event*& ev)
{
    // This only gets PortModuleEvents
    PortModuleEvent* event = static_cast<PortModuleEvent*>(ev);

    // The last event is for control only and the PortModule will
    // ignore it
    if ( event->last ) return;

    if ( replace_ ) {
        // Create ack event to deliver instead
        Event* new_ev = new PortModuleAckEvent();
        new_ev->copyAllDeliveryInfo(ev);
        delete ev;
        ev = new_ev;
        return;
    }

    if ( modify_ ) {
        event->modified = true;
        return;
    }

    if ( drop_ ) {
        delete ev;
        ev = nullptr;
        return;
    }
}


void
TestPortModule::interceptHandler(uintptr_t UNUSED(key), Event*& data, bool& cancel)
{
    // This only gets PortModuleEvents
    PortModuleEvent* event = static_cast<PortModuleEvent*>(data);

    // Default is to not cancel delivery
    cancel = false;

    // The last event is for control only and the PortModule will
    // ignore it
    if ( event->last ) return;

    if ( replace_ ) {
        Event* new_ev = new PortModuleAckEvent();
        new_ev->copyAllDeliveryInfo(data);
        delete data;
        data = new_ev;
        return;
    }

    if ( modify_ ) {
        event->modified = true;
        return;
    }

    if ( drop_ ) {
        delete data;
        data   = nullptr;
        cancel = true;
    }
}


/********* CoreTestPortModuleComponent **********/

coreTestPortModuleComponent::coreTestPortModuleComponent(ComponentId_t id, Params& params) : Component(id)
{
    // Get the parameters
    sendcount_             = params.find<int>("sendcount", "20");
    bool use_subcomponent_ = params.find<bool>("use_subcomponent", "false");

    // First need to determine if this is the first or last component.
    // The component is first if there is nothing connected to the
    // left port and is last if nothing is connected to the right
    // port.

    // We can check the ports at the component level even though they
    // may actually be connected in a subcompoment becuase they will
    // be connected via shared ports.
    bool first = !isPortConnected("left");
    bool last  = !isPortConnected("right");

    // Set up ports
    Event::HandlerBase* handler;
    if ( last ) {
        handler = new Event::Handler2<coreTestPortModuleComponent, &coreTestPortModuleComponent::handleEventLast>(this);
    }
    else {
        handler = new Event::Handler2<coreTestPortModuleComponent, &coreTestPortModuleComponent::handleEvent>(this);
    }

    if ( use_subcomponent_ ) {
        sub_ = loadAnonymousSubComponent<PortSubComponent>(
            "coreTestElement.PortSubComponent", "port_slot", 0, ComponentInfo::SHARE_PORTS, params);

        left_ = sub_->getLeft();
        // Replace the function. This will test the transfering of
        // AttachPoints on the handler when a PortModule is installed
        // on the receive handler
        if ( left_ )
            left_->replaceFunctor(handler); // Link on left port
        else
            delete handler; // No link on left port
        right_ = sub_->getRight();
    }
    else {
        left_ = configureLink("left", handler);
        if ( nullptr == left_ ) delete handler; // No link on left port
        right_ = configureLink("right", "1ns");
    }

    // If we are first, need to add a clock
    if ( first ) {
        registerClock(
            "10MHz", new Clock::Handler2<coreTestPortModuleComponent, &coreTestPortModuleComponent::tick>(this));
    }

    if ( first || last ) {
        registerAsPrimaryComponent();
        primaryComponentDoNotEndSim();
    }

    repeat_last_ = params.find<bool>("repeat_last", "false");
}

bool
coreTestPortModuleComponent::tick(Cycle_t UNUSED(cycle))
{
    if ( sendcount_ > 0 ) { right_->send(new PortModuleEvent()); }
    else {
        PortModuleEvent* ev = new PortModuleEvent();
        ev->last            = true;
        right_->send(ev);
        if ( sendcount_ == 0 ) primaryComponentOKToEndSim();
        // If we are repeating the last, then we won't end, otherwise,
        // cancel clock handler by returning true.
        return !repeat_last_;
    }

    sendcount_--;
    return false;
}

void
coreTestPortModuleComponent::handleEvent(Event* ev)
{
    // Just end this on to the right
    right_->send(ev);
}

void
coreTestPortModuleComponent::handleEventLast(Event* ev)
{
    // See what type of event this is
    PortModuleEvent* event = dynamic_cast<PortModuleEvent*>(ev);
    if ( event ) {
        if ( event->last ) {
            // Report end of simulation
            primaryComponentOKToEndSim();
        }
        else if ( event->modified ) {
            getSimulationOutput().output("(%" PRI_SIMTIME ") Got a modified event\n", getCurrentSimCycle());
        }
        else {
            getSimulationOutput().output("(%" PRI_SIMTIME ") Got an unmodified event\n", getCurrentSimCycle());
        }
        delete ev;
        return;
    }

    // Not a regular event, see if it is an ack
    PortModuleAckEvent* ack = dynamic_cast<PortModuleAckEvent*>(ev);
    if ( ack ) {
        getSimulationOutput().output("(%" PRI_SIMTIME ") Got an ack event\n", getCurrentSimCycle());
        delete ev;
        return;
    }

    delete ev;
    getSimulationOutput().output("ERROR: Got an event of unknown type\n");
}


/********* PortSubComponent **********/

PortSubComponent::PortSubComponent(ComponentId_t id, Params& UNUSED(params)) : SubComponent(id)
{
    // Need to connect to the right and left ports
    left_  = configureLink("left", new Event::Handler2<PortSubComponent, &PortSubComponent::dummy_handler>(this));
    right_ = configureLink("right", "1ns");
}
