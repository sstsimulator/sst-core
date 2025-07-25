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

#include "sst/core/testElements/coreTest_Component.h"

#include "sst/core/testElements/coreTest_ComponentEvent.h"

#include "sst/core/event.h"

#include <assert.h>

using namespace SST;
using namespace SST::CoreTestComponent;

coreTestComponent::coreTestComponent(ComponentId_t id, Params& params) :
    coreTestComponentBase2(id)
{
    bool found;

    rng = new SST::RNG::MarsagliaRNG(11, 272727);

    // get parameters
    workPerCycle = params.find<int64_t>("workPerCycle", 0, found);
    if ( !found ) {
        getSimulationOutput().fatal(CALL_INFO, -1, "couldn't find work per cycle\n");
    }

    commFreq = params.find<int64_t>("commFreq", 0, found);
    if ( !found ) {
        getSimulationOutput().fatal(CALL_INFO, -1, "couldn't find communication frequency\n");
    }

    commSize = params.find<int64_t>("commSize", 16);


    std::string clockFrequency = params.find<std::string>("clockFrequency", "1GHz");

    // init randomness
    srand(1);
    neighbor = rng->generateNextInt32() % 4;

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // configure out links
    N = configureLink("Nlink", new Event::Handler2<coreTestComponent, &coreTestComponent::handleEvent>(this));
    S = configureLink("Slink", new Event::Handler2<coreTestComponent, &coreTestComponent::handleEvent>(this));
    E = configureLink("Elink", new Event::Handler2<coreTestComponent, &coreTestComponent::handleEvent>(this));
    W = configureLink("Wlink", new Event::Handler2<coreTestComponent, &coreTestComponent::handleEvent>(this));

    countN = registerStatistic<int>("N");
    countS = registerStatistic<int>("S");
    countE = registerStatistic<int>("E");
    countW = registerStatistic<int>("W");

    assert(N);
    assert(S);
    assert(E);
    assert(W);

    // set our clock
    registerClock(clockFrequency, new Clock::Handler2<coreTestComponent, &coreTestComponent::clockTic>(this));

    last_event_id = SST::Event::NO_ID;
}

coreTestComponent::~coreTestComponent()
{
    delete rng;
}

// incoming events are scanned and deleted
void
coreTestComponent::handleEvent(Event* ev)
{
    // printf("recv\n");
    coreTestComponentEvent* event = dynamic_cast<coreTestComponentEvent*>(ev);
    if ( event ) {
        // scan through each element in the payload and do something to it
        for ( coreTestComponentEvent::dataVec::iterator i = event->payload.begin(); i != event->payload.end(); ++i ) {
            event->payload[0] += *i;
        }
        delete event;
    }
    else {
        printf("Error! Bad Event Type!\n");
    }
}

// each clock tick we do 'workPerCycle' iterations of a coreTest loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool
coreTestComponent::clockTic(Cycle_t)
{
    // do work
    // loop becomes:
    /*  00001ab5        movl    0xe0(%ebp),%eax
        00001ab8        incl    %eax
        00001ab9        movl    %eax,0xe0(%ebp)
        00001abc        incl    %edx
        00001abd        cmpl    %ecx,%edx
        00001abf        jne     0x00001ab5

        6 instructions.
    */

    volatile int v = 0;
    for ( int i = 0; i < workPerCycle; ++i ) {
        v++;
    }

    // communicate?
    if ( (rng->generateNextInt32() % commFreq) == 0 ) {
        // yes, communicate
        // create event
        coreTestComponentEvent* e = new coreTestComponentEvent();

        // assign a unique ID to the event
        e->setId();
        if ( last_event_id != SST::Event::NO_ID ) {
            sst_assert(e->id.first > last_event_id.first, CALL_INFO_LONG, EXIT_FAILURE,
                "Assigned a non-monotonically increasing event ID. id=%" PRIu64 ", last id=%" PRIu64 "\n", e->id.first,
                last_event_id.first);
        }
        last_event_id = e->id;

        // fill payload with commSize bytes
        for ( int i = 0; i < (commSize); ++i ) {
            e->payload.push_back(1);
        }
        // find target
        neighbor = neighbor + 1;
        neighbor = neighbor % 4;

        // send
        switch ( neighbor ) {
        case 0:
            N->send(e);
            countN->addData(1);
            break;
        case 1:
            S->send(e);
            countS->addData(1);
            break;
        case 2:
            E->send(e);
            countE->addData(1);
            break;
        case 3:
            W->send(e);
            countW->addData(1);
            break;
        default:
            printf("bad neighbor\n");
        }
        // printf("sent\n");
    }

    // return false so we keep going
    return false;
}

void
coreTestComponent::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::CoreTestComponent::coreTestComponentBase2::serialize_order(ser);
    SST_SER(workPerCycle);
    SST_SER(commFreq);
    SST_SER(commSize);
    SST_SER(neighbor);

    SST_SER(rng);
    SST_SER(N);
    SST_SER(S);
    SST_SER(E);
    SST_SER(W);

    SST_SER(countN);
    SST_SER(countS);
    SST_SER(countE);
    SST_SER(countW);
}

// Element Library / Serialization stuff
