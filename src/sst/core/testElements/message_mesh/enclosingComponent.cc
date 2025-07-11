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

EnclosingComponent::EnclosingComponent(ComponentId_t id, Params& params) :
    Component(id),
    message_count_(0)
{
    my_id_ = params.find<int>("id", -1);
    if ( my_id_ == -1 ) {
        Output::getDefaultObject().fatal(CALL_INFO, -1, "Must specify param 'id' in EnclosingComponent\n");
    }

    verbose_ = params.find<bool>("verbose", true);

    mod_ = params.find<int>("mod", 1);
    if ( mod_ < 1 ) {
        Output::getDefaultObject().fatal(CALL_INFO, EXIT_FAILURE, "Modulus must be at least 1\n");
    }

    // Need to check to see how many ports there are and create all the SubComponents
    SubComponentSlotInfo* info = getSubComponentSlotInfo("ports");
    if ( !info ) {
        Output::getDefaultObject().fatal(CALL_INFO, EXIT_FAILURE,
            "Must specify at least one PortInterface SubComponent for slot 'ports' in EnclosingComponent\n");
    }

    info->createAll<PortInterface>(ports_, ComponentInfo::SHARE_NONE);

    // Need to get the "route" object
    route_ = loadUserSubComponent<RouteInterface>("route", ComponentInfo::SHARE_NONE, ports_, my_id_);
    if ( !info ) {
        Output::getDefaultObject().fatal(CALL_INFO, -1,
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
    for ( size_t i = 0; i < ports_.size(); ++i ) {
        ports_[i]->setNotifyOnReceive(
            new PortInterface::Handler2<EnclosingComponent, &EnclosingComponent::handleEvent, int>(this, i));
    }
    route_->sendInitialEvents(mod_);
}

void
EnclosingComponent::finish()
{
    if ( verbose_ ) {
        Output::getDefaultObject().output("%d received %d messages\n", my_id_, message_count_);
    }
}

void
EnclosingComponent::handleEvent(SST::Event* ev, int port)
{
    MessageEvent* mev = static_cast<MessageEvent*>(ev);

    // "Route" the message and send it on to next port
    message_count_++;
    route_->send(mev, port);
}

void
EnclosingComponent::serialize_order(SST::Core::Serialization::serializer& ser)
{
    Component::serialize_order(ser);
    SST_SER(my_id_);
    SST_SER(verbose_);
    SST_SER(message_count_);
    SST_SER(ports_);
    SST_SER(route_);
    // Don't need mod_ since it is only used during init
}


PortSlot::PortSlot(ComponentId_t id, Params& UNUSED(params)) :
    PortInterface(id)
{
    port_ = loadUserSubComponent<PortInterface>("port");
}

void
PortSlot::serialize_order(SST::Core::Serialization::serializer& ser)
{
    PortInterface::serialize_order(ser);
    SST_SER(port_);
}


MessagePort::MessagePort(ComponentId_t id, Params& UNUSED(params)) :
    PortInterface(id)
{
    std::string name = "port0";
    while ( isPortConnected(name) ) {
        links_.push_back(configureLink(name, new Event::Handler2<MessagePort, &MessagePort::handleEvent>(this)));
        name = "port" + std::to_string(links_.size());
    }
}

void
MessagePort::send(MessageEvent* ev, size_t index)
{
    links_[index]->send(ev);
}

void
MessagePort::handleEvent(Event* ev)
{
    // Just pass this to the functor.  If no functor, just delete
    // event
    if ( functor_ )
        (*functor_)(ev);
    else
        delete ev;
}

void
MessagePort::serialize_order(SST::Core::Serialization::serializer& ser)
{
    PortInterface::serialize_order(ser);
    SST_SER(links_);
}


RouteMessage::RouteMessage(
    ComponentId_t id, Params& UNUSED(params), std::vector<PortInterface*>& parent_ports, int nid) :
    RouteInterface(id),
    ports_(parent_ports),
    my_id_(nid)
{
    rng_  = new SST::RNG::MersenneRNG(my_id_ + 100);
    mcnt_ = registerStatistic<uint64_t>("msg_count");

    for ( auto port : ports_ ) {
        counts_.push_back(port->getPortCount());
    }
}

void
RouteMessage::send(MessageEvent* ev, int UNUSED(incoming_port))
{
    // Route to random port
    int    nextport = rng_->generateNextUInt32() % ports_.size();
    size_t portnum  = rng_->generateNextUInt32() % counts_[nextport];
    ports_[nextport]->send(ev, portnum);
    mcnt_->addData(1);
}

void
RouteMessage::sendInitialEvents(int mod)
{
    for ( size_t i = 0; i < ports_.size(); i++ ) {
        for ( size_t j = 0; j < counts_[i]; j++ ) {
            if ( (rng_->generateNextUInt32() % mod) == 0 ) {
                MessageEvent* ev = new MessageEvent();
                ports_[i]->send(ev, j);
            }
        }
    }
}

void
RouteMessage::serialize_order(SST::Core::Serialization::serializer& ser)
{
    RouteInterface::serialize_order(ser);
    SST_SER(ports_);
    SST_SER(my_id_);
    SST_SER(rng_);
    SST_SER(mcnt_);
    SST_SER(counts_);
}
