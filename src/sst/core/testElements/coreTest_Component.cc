// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
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

#include "sst/core/testElements/coreTest_Component.h"
#include "sst/core/testElements/coreTest_ComponentEvent.h"

#include <assert.h>
#include "sst/core/event.h"

using namespace SST;
using namespace SST::CoreTestComponent;

coreTestComponent::coreTestComponent(ComponentId_t id, Params& params) :
  Component(id)
{
    bool found;

    rng = new SST::RNG::MarsagliaRNG(11, 272727);

    // get parameters
    workPerCycle = params.find<int64_t>("workPerCycle", 0, found);
    if (!found) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,"couldn't find work per cycle\n");
    }

    commFreq = params.find<int64_t>("commFreq", 0, found);
    if (!found) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,"couldn't find communication frequency\n");
    }

    commSize = params.find<int64_t>("commSize", 16);

    // init randomness
    srand(1);
    neighbor = rng->generateNextInt32() % 4;

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // configure out links
    N = configureLink("Nlink", new Event::Handler<coreTestComponent>(this,
                               &coreTestComponent::handleEvent));
    S = configureLink("Slink", new Event::Handler<coreTestComponent>(this,
                               &coreTestComponent::handleEvent));
    E = configureLink("Elink", new Event::Handler<coreTestComponent>(this,
                               &coreTestComponent::handleEvent));
    W = configureLink("Wlink", new Event::Handler<coreTestComponent>(this,
                               &coreTestComponent::handleEvent));

    assert(N);
    assert(S);
    assert(E);
    assert(W);

    //set our clock
    registerClock("1GHz", new Clock::Handler<coreTestComponent>(this,
                  &coreTestComponent::clockTic));
}

coreTestComponent::~coreTestComponent()
{
	delete rng;
}

coreTestComponent::coreTestComponent() : Component(-1)
{
    // for serialization only
}

// incoming events are scanned and deleted
void coreTestComponent::handleEvent(Event *ev)
{
    //printf("recv\n");
    coreTestComponentEvent *event = dynamic_cast<coreTestComponentEvent*>(ev);
    if (event) {
        // scan through each element in the payload and do something to it
        volatile int sum = 0;
        for (coreTestComponentEvent::dataVec::iterator i = event->payload.begin();
             i != event->payload.end(); ++i) {
            sum += *i;
        }
        delete event;
    } else {
        printf("Error! Bad Event Type!\n");
    }
}

// each clock tick we do 'workPerCycle' iterations of a coreTest loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool coreTestComponent::clockTic( Cycle_t )
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
    for (int i = 0; i < workPerCycle; ++i) {
        v++;
    }

    // communicate?
    if ((rng->generateNextInt32() % commFreq) == 0) {
        // yes, communicate
        // create event
        coreTestComponentEvent *e = new coreTestComponentEvent();
        // fill payload with commSize bytes
        for (int i = 0; i < (commSize); ++i) {
            e->payload.push_back(1);
        }
        // find target
        neighbor = neighbor+1;
        neighbor = neighbor % 4;
        // send
        switch (neighbor) {
        case 0:
            N->send(e);
            break;
        case 1:
            S->send(e);
            break;
        case 2:
            E->send(e);
            break;
        case 3:
            W->send(e);
            break;
        default:
            printf("bad neighbor\n");
        }
        //printf("sent\n");
    }

    // return false so we keep going
    return false;
}

// Element Libarary / Serialization stuff



