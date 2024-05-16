// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/testElements/coreTest_MessageGeneratorComponent.h"

#include "sst/core/testElements/coreTest_Message.h"

using namespace SST;
using namespace SST::CoreTestMessageGeneratorComponent;

coreTestMessageGeneratorComponent::coreTestMessageGeneratorComponent(ComponentId_t id, Params& params) : Component(id)
{
    clock_frequency_str = params.find<std::string>("clock", "1GHz");
    std::cout << "Clock is configured for: " << clock_frequency_str << std::endl;

    total_message_send_count = params.find<int64_t>("sendcount", 1000);
    output_message_info      = params.find<int64_t>("outputinfo", 1);

    message_counter_recv = 0;
    message_counter_sent = 0;

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    remote_component = configureLink(
        "remoteComponent",
        new Event::Handler<coreTestMessageGeneratorComponent>(this, &coreTestMessageGeneratorComponent::handleEvent));

    assert(remote_component);

    // set our clock
    registerClock(
        clock_frequency_str,
        new Clock::Handler<coreTestMessageGeneratorComponent>(this, &coreTestMessageGeneratorComponent::tick));
}

coreTestMessageGeneratorComponent::coreTestMessageGeneratorComponent() : Component(-1)
{
    // for serialization only
}

void
coreTestMessageGeneratorComponent::handleEvent(Event* event)
{
    message_counter_recv++;

    if ( output_message_info ) {
        std::cout << "Received message: " << message_counter_recv << " (time=" << getCurrentSimTimeMicro() << "us)"
                  << std::endl;
    }

    delete event;

    if ( message_counter_recv == total_message_send_count ) { primaryComponentOKToEndSim(); }
}

// each clock tick we do 'workPerCycle' iterations of a coreTest loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool coreTestMessageGeneratorComponent::tick(Cycle_t)
{
    coreTestMessage* msg = new coreTestMessage();
    remote_component->send(msg);

    if ( output_message_info ) {
        std::cout << "Sent message: " << message_counter_sent << " (time=" << getCurrentSimTimeMicro() << "us)"
                  << std::endl;
    }

    message_counter_sent++;

    // return false so we keep going
    if ( message_counter_sent == total_message_send_count ) { return true; }
    else {
        return false;
    }
}

// Element Libarary / Serialization stuff
