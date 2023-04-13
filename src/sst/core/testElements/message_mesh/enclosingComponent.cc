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

#include "sst_config.h"

#include "sst/core/testElements/message_mesh/enclosingComponent.h"

#include "sst/core/testElements/message_mesh/messageEvent.h"

#include "sst/core/clock.h"
#include "sst/core/componentInfo.h"
#include "sst/core/output.h"
#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/rng.h"
#include "sst/core/timeConverter.h"

using namespace SST;
using namespace SST::CoreTest;
using namespace SST::CoreTest::MessageMesh;

EnclosingComponent::EnclosingComponent(ComponentId_t id, Params& params) : Component(id), message_count(0)
{

    my_id = params.find<int>("id", -1);
    if ( my_id == -1 ) {
        Output::getDefaultObject().fatal(CALL_INFO, -1, "Must specify param 'id' in EnclosingComponent\n");
    }

    // Need to check to see how many ports there are and create all the SubComponents
    SubComponentSlotInfo* info = getSubComponentSlotInfo("ports");
    if ( !info ) {
        Output::getDefaultObject().fatal(
            CALL_INFO, -1,
            "Must specify at least one PortInterface SubComponent for slot 'ports' in EnclosingComponent\n");
    }

    info->createAll<PortInterface>(ports, ComponentInfo::SHARE_NONE);

    // Need to get the "route" object
    route = loadUserSubComponent<RouteInterface>("route", ComponentInfo::SHARE_NONE, ports, my_id);
    if ( !info ) {
        Output::getDefaultObject().fatal(
            CALL_INFO, -1,
            "Must specify The RouteInterface SubComponent to use for slot 'route' in EnclosingComponent\n");
    }

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

void
EnclosingComponent::setup()
{
    // Send an event on each output port.  Also register the callback
    // function
    for ( size_t i = 0; i < ports.size(); ++i ) {
        ports[i]->send(new MessageEvent());
        ports[i]->setNotifyOnReceive(
            new PortInterface::Handler<EnclosingComponent, int>(this, &EnclosingComponent::handleEvent, i));
    }
}

void
EnclosingComponent::finish()
{
    Output::getDefaultObject().output("%d received %d messages\n", my_id, message_count);
}

void
EnclosingComponent::handleEvent(SST::Event* ev, int port)
{
    MessageEvent* mev = static_cast<MessageEvent*>(ev);

    // "Route" the message and send it on to next port
    message_count++;
    route->send(mev, port);
}

PortSlot::PortSlot(ComponentId_t id, Params& UNUSED(params)) : PortInterface(id)
{
    port = loadUserSubComponent<PortInterface>("port");
}

MessagePort::MessagePort(ComponentId_t id, Params& UNUSED(params)) : PortInterface(id)
{
    link = configureLink("port", new Event::Handler<MessagePort>(this, &MessagePort::handleEvent));
}

void
MessagePort::send(MessageEvent* ev)
{
    link->send(ev);
}

void
MessagePort::handleEvent(Event* ev)
{
    // Just pass this to the functor.  If no functor, just delete
    // event
    if ( rFunctor )
        (*rFunctor)(ev);
    else
        delete ev;
}


RouteMessage::RouteMessage(
    ComponentId_t id, Params& UNUSED(params), const std::vector<PortInterface*>& parent_ports, int nid) :
    RouteInterface(id),
    ports(parent_ports),
    my_id(nid)
{
    rng = new SST::RNG::MersenneRNG(my_id + 100);
}

void
RouteMessage::send(MessageEvent* ev, int UNUSED(incoming_port))
{
    // Route to random port
    int nextport = rng->generateNextUInt32() % ports.size();
    ports[nextport]->send(ev);
}
