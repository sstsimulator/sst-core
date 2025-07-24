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

#include "sst/core/testElements/coreTest_ComponentExtension.h"

#include "sst/core/testElements/coreTest_ComponentEvent.h"

#include "sst/core/event.h"

#include <assert.h>

using namespace SST;
using namespace SST::CoreTestComponent;


////////////////////////////////////////////////////
// CoreTestComponentExt2 - Handles links
////////////////////////////////////////////////////
coreTestComponentExt2::coreTestComponentExt2(ComponentId_t id, int neighbor) :
    SST::ComponentExtension(id)
{
    neighbor_ = neighbor;

    // configure out links
    N_ = configureLink("Nlink", new Event::Handler2<coreTestComponentExt2, &coreTestComponentExt2::handleEvent>(this));
    S_ = configureLink("Slink", new Event::Handler2<coreTestComponentExt2, &coreTestComponentExt2::handleEvent>(this));
    E_ = configureLink("Elink", new Event::Handler2<coreTestComponentExt2, &coreTestComponentExt2::handleEvent>(this));
    W_ = configureLink("Wlink", new Event::Handler2<coreTestComponentExt2, &coreTestComponentExt2::handleEvent>(this));

    count_N_ = registerStatistic<int>("N");
    count_S_ = registerStatistic<int>("S");
    count_E_ = registerStatistic<int>("E");
    count_W_ = registerStatistic<int>("W");

    assert(N_);
    assert(S_);
    assert(E_);
    assert(W_);
}


void
coreTestComponentExt2::send(Event* ev)
{
    // find target
    neighbor_ = neighbor_ + 1;
    neighbor_ = neighbor_ % 4;

    // send
    switch ( neighbor_ ) {
    case 0:
        N_->send(ev);
        count_N_->addData(1);
        break;
    case 1:
        S_->send(ev);
        count_S_->addData(1);
        break;
    case 2:
        E_->send(ev);
        count_E_->addData(1);
        break;
    case 3:
        W_->send(ev);
        count_W_->addData(1);
        break;
    default:
        getSimulationOutput().output("bad neighbor\n");
    }
}

// incoming events are scanned and deleted
void
coreTestComponentExt2::handleEvent(Event* ev)
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

void
coreTestComponentExt2::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::ComponentExtension::serialize_order(ser);
    SST_SER(neighbor_);

    SST_SER(N_);
    SST_SER(S_);
    SST_SER(E_);
    SST_SER(W_);

    SST_SER(count_N_);
    SST_SER(count_S_);
    SST_SER(count_E_);
    SST_SER(count_W_);
}


////////////////////////////////////////////////////
// CoreTestComponentExt - Handles event generation
////////////////////////////////////////////////////
coreTestComponentExt::coreTestComponentExt(
    ComponentId_t id, int64_t comm_freq, std::string clk, int64_t work_per_cycle, int64_t comm_size) :
    SST::ComponentExtension(id)
{
    comm_freq_      = comm_freq;
    rng_            = new SST::RNG::MarsagliaRNG(11, 272727);
    work_per_cycle_ = work_per_cycle;
    comm_size_      = comm_size;

    registerClock(clk, new Clock::Handler2<coreTestComponentExt, &coreTestComponentExt::clockTic>(this));

    int neighbor = generateNext() % 4;

    ext_ = loadComponentExtension<coreTestComponentExt2>(neighbor);
}

coreTestComponentExt::~coreTestComponentExt()
{
    delete rng_;
}

int32_t
coreTestComponentExt::generateNext()
{
    return rng_->generateNextInt32();
}

bool
coreTestComponentExt::communicate()
{
    return (generateNext() % comm_freq_) == 0;
}

// each clock tick we do 'work_per_cycle_' iterations of a coreTest loop.
// We have a 1/commFreq chance of sending an event of size comm_size_ to
// one of our neighbors.
bool
coreTestComponentExt::clockTic(Cycle_t)
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
    for ( int i = 0; i < work_per_cycle_; ++i ) {
        v += 1;
    }

    // communicate?
    if ( communicate() ) {
        // yes, communicate
        // create event
        coreTestComponentEvent* e = new coreTestComponentEvent();
        // fill payload with comm_size_ bytes
        for ( int i = 0; i < (comm_size_); ++i ) {
            e->payload.push_back(1);
        }
        ext_->send(e);
    }

    // return false so we keep going
    return false;
}

void
coreTestComponentExt::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::ComponentExtension::serialize_order(ser);
    SST_SER(rng_);
    SST_SER(comm_freq_);
    SST_SER(ext_);
    SST_SER(comm_size_);
    SST_SER(work_per_cycle_);
}

////////////////////////////////////////////////////
// CoreTestComponentExtMain - Main component
////////////////////////////////////////////////////
coreTestComponentExtMain::coreTestComponentExtMain(ComponentId_t id, Params& params) :
    SST::Component(id)
{
    bool found;

    // get parameters
    int64_t workPerCycle = params.find<int64_t>("workPerCycle", 0, found);
    if ( !found ) {
        getSimulationOutput().fatal(CALL_INFO, -1, "couldn't find work per cycle\n");
    }

    int64_t commFreq = params.find<int64_t>("commFreq", 0, found);
    if ( !found ) {
        getSimulationOutput().fatal(CALL_INFO, -1, "couldn't find communication frequency\n");
    }

    int64_t commSize = params.find<int64_t>("commSize", 16);

    std::string clockFrequency = params.find<std::string>("clockFrequency", "1GHz");

    ext_ = loadComponentExtension<coreTestComponentExt>(commFreq, clockFrequency, workPerCycle, commSize);

    // init randomness

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

coreTestComponentExtMain::~coreTestComponentExtMain() {}

void
coreTestComponentExtMain::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::Component::serialize_order(ser);
    SST_SER(ext_);
}

// Element Library / Serialization stuff
