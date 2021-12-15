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

#include "sst_config.h"

#include "sst/core/testElements/coreTest_PerfComponent.h"

#include "sst/core/testElements/coreTest_ComponentEvent.h"

#include "sst/core/event.h"
#include "sst/core/simulation.h"

#include <assert.h>
#include <math.h>

using namespace SST;
using namespace SST::CoreTestPerfComponent;

coreTestPerfComponent::coreTestPerfComponent(SST::ComponentId_t id, SST::Params& params) :
    coreTestPerfComponentBase2(id)
{
    bool found;

    rng = new SST::RNG::MarsagliaRNG(11, 272727);

    // get parameters
    workPerCycle = params.find<int64_t>("workPerCycle", 0, found);
    if ( !found ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "couldn't find work per cycle\n");
    }

    commFreq = params.find<int64_t>("commFreq", 0, found);
    if ( !found ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(
            CALL_INFO, -1, "couldn't find communication frequency\n");
    }

    commSize = params.find<int64_t>("commSize", 16);

    // init randomness
    srand(1);
    neighbor = rng->generateNextInt32() % 4;

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // configure out links
    N = configureLink("Nlink", new Event::Handler<coreTestPerfComponent>(this, &coreTestPerfComponent::handleEvent));
    S = configureLink("Slink", new Event::Handler<coreTestPerfComponent>(this, &coreTestPerfComponent::handleEvent));
    E = configureLink("Elink", new Event::Handler<coreTestPerfComponent>(this, &coreTestPerfComponent::handleEvent));
    W = configureLink("Wlink", new Event::Handler<coreTestPerfComponent>(this, &coreTestPerfComponent::handleEvent));

    countN = registerStatistic<int>("N");
    countS = registerStatistic<int>("S");
    countE = registerStatistic<int>("E");
    countW = registerStatistic<int>("W");

    assert(N);
    assert(S);
    assert(E);
    assert(W);

    // set our clock
    auto clockHandler = new Clock::Handler<coreTestPerfComponent>(this, &coreTestPerfComponent::clockTic);
    registerClock("1GHz", clockHandler);
    registerClockHandler(clockHandler);
}

coreTestPerfComponent::~coreTestPerfComponent()
{
    delete rng;
}

coreTestPerfComponent::coreTestPerfComponent() : coreTestPerfComponentBase2(-1)
{
    // for serialization only
}

// incoming events are scanned and deleted
void
coreTestPerfComponent::handleEvent(Event* ev)
{
    // printf("recv\n");
    SST::CoreTestComponent::coreTestComponentEvent* event =
        dynamic_cast<SST::CoreTestComponent::coreTestComponentEvent*>(ev);
    if ( event ) {
        // scan through each element in the payload and do something to it
        volatile int sum = 0;
        for ( SST::CoreTestComponent::coreTestComponentEvent::dataVec::iterator i = event->payload.begin();
              i != event->payload.end(); ++i ) {
            sum += *i;
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
bool coreTestPerfComponent::clockTic(Cycle_t)
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

    // As this is meant to test the performance counter infrastructure,
    // we'll give this loop more to do - trig functions are typically
    // quite slow so this should eat up some CPU cycles
    volatile int    v   = 0;
    volatile double sum = 0.0;
    for ( int i = 0; i < workPerCycle; ++i ) {
        sum = sum + sin(double(i));
        v++;
    }

    // communicate?
    if ( (rng->generateNextInt32() % commFreq) == 0 ) {
        // yes, communicate
        // create event
        SST::CoreTestComponent::coreTestComponentEvent* e = new SST::CoreTestComponent::coreTestComponentEvent();
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

// Element Libarary / Serialization stuff
