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

#include "sst/core/testElements/coreTest_MemPoolTest.h"

#include "sst/core/event.h"
#include "sst/core/mempoolAccessor.h"
#include "sst/core/output.h"
#include "sst/core/simulation.h"

#include <string>
#include <vector>

using namespace SST;
using namespace SST::CoreTestMemPoolTest;

MemPoolTestComponent::MemPoolTestComponent(ComponentId_t id, Params& params) :
    Component(id),
    events_sent(0),
    events_recv(0),
    event_rate(0.0)
{
    // Get the event size to send
    event_size = params.find<int>("event_size");
    if ( event_size < 1 || event_size > 4 ) {
        fatal(CALL_INFO, 1, "ERROR: Invalid event_size value: %d, valide range is 1 - 4\n", event_size);
    }

    initial_events = params.find<int>("initial_events", 256);

    undeleted_events = params.find<int>("undeleted_events", 0);

    check_overflow = params.find<bool>("check_overflow", true);

    // Connect to all the links
    bool done  = false;
    int  count = 0;
    while ( !done ) {
        std::string port_name("port");
        port_name += std::to_string(count);
        Link* link = configureLink(
            port_name, new Event::Handler<MemPoolTestComponent, int>(this, &MemPoolTestComponent::eventHandler, count));
        if ( nullptr == link ) {
            done = true;
            break;
        }
        links.push_back(link);
        count++;
    }

    if ( links.size() >= 4 ) { fatal(CALL_INFO, 1, "ERROR: MemPoolTestComponent only supports up to 4 components\n"); }

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

void
MemPoolTestComponent::eventHandler(Event* ev, int port)
{
    // Whenever I get an event, just delete it and send one back.  We
    // will not delete some events, set by the undeleted_events
    // parameter.  This will allow us to check to see if the
    // undeleted_event detection works.
    if ( undeleted_events > 0 ) { undeleted_events--; }
    else {
        delete ev;
    }
    events_recv++;
    links[port]->send(createEvent());
}

void
MemPoolTestComponent::setup()
{
    // Just send an event on each of my links
    for ( int i = 0; i < initial_events; ++i ) {
        for ( auto* x : links ) {
            x->send(createEvent());
            events_sent++;
        }
    }
}


void
MemPoolTestComponent::complete(unsigned int phase)
{
    if ( phase == 0 ) {
        event_rate = (double)events_recv / getRunPhaseElapsedRealTime();

        auto* rate_event = new MemPoolTestPerformanceEvent();
        rate_event->rate = event_rate;

        // Send on link 0
        if ( event_size != 1 )
            links[0]->sendUntimedData(rate_event);
        else
            delete rate_event;
    }
    else if ( event_size == 1 ) {
        // Check each of my links for rate events.  Only component 0
        // (the one with size 1) will get messages.
        for ( auto* link : links ) {
            Event* ev = link->recvUntimedData();
            if ( ev != nullptr ) { event_rate += (static_cast<MemPoolTestPerformanceEvent*>(ev))->rate; }
            delete ev;
        }

        getSimulationOutput().output("# Event rate = %lf Mmsgs/s\n", event_rate / 1000000);
    }
}

void
MemPoolTestComponent::finish()
{
    if ( !check_overflow ) return;
    // Need to get the number of mempool arenas created.  There
    // shouldn't be more than N (where N is the number of components,
    // up to 4, each on their own thread).
    size_t actual_event_size = 0;
    switch ( event_size ) {
    case 1:
        actual_event_size = sizeof(MemPoolTestEvent1);
        break;
    case 2:
        actual_event_size = sizeof(MemPoolTestEvent2);
        break;
    case 3:
        actual_event_size = sizeof(MemPoolTestEvent3);
        break;
    case 4:
        actual_event_size = sizeof(MemPoolTestEvent4);
        break;
    default:
        fatal(CALL_INFO, 1, "ERROR: Invalid event_size value: %d, valide range is 1 - 4\n", event_size);
    }
    size_t num_arenas = Core::MemPoolAccessor::getNumArenas(actual_event_size);

    event_rate = (double)events_recv / getRunPhaseElapsedRealTime();

    if ( num_arenas > (links.size() + 1) ) {
        getSimulationOutput().output("FAIL: MemPool overflow test failed for size: %zu\n", actual_event_size);
    }
    else {
        getSimulationOutput().output("PASS: MemPool overflow test passed for size: %zu\n", actual_event_size);
    }
}

Event*
MemPoolTestComponent::createEvent()
{
    switch ( event_size ) {
    case 1:
        return new MemPoolTestEvent1();
    case 2:
        return new MemPoolTestEvent2();
    case 3:
        return new MemPoolTestEvent3();
    case 4:
        return new MemPoolTestEvent4();
    default:
        fatal(CALL_INFO, 1, "ERROR: Invalid event_size value: %d, valide range is 1 - 4\n", event_size);
    }
    return nullptr;
}
