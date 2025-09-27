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

#include "sst/core/testElements/coreTest_OverheadMeasure.h"

#include "sst/core/event.h"

using namespace SST;
using namespace SST::CoreTestOverhead;

OverheadMeasure::OverheadMeasure(ComponentId_t id, Params& params) :
    Component(id)
{
    id_ = params.find<int>("id");

    // configure out links
    bool done = false;
    ports_    = 0;
    while ( !done ) {
        std::string port_name("left_");
        port_name     = port_name + std::to_string(ports_);
        auto* handler = new Event::Handler2<OverheadMeasure, &OverheadMeasure::handleEvent, int>(this, ports_);
        Link* link    = configureLink(port_name, "1ns", handler);
        ++ports_;
        if ( !link ) {
            delete handler;
            done = true;
        }
    }

    while ( !done ) {
        std::string port_name("right_");
        port_name     = port_name + std::to_string(ports_);
        auto* handler = new Event::Handler2<OverheadMeasure, &OverheadMeasure::handleEvent, int>(this, ports_);
        Link* link    = configureLink(port_name, "1ns", handler);
        ++ports_;
        if ( !link ) {
            delete handler;
            done = true;
        }
    }

    // set our clock
    registerClock("100 MHz", new Clock::Handler2<OverheadMeasure, &OverheadMeasure::clockTic>(this));

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

// incoming events are scanned and deleted
void
OverheadMeasure::handleEvent(Event* UNUSED(ev), int UNUSED(port))
{
    // Does not send events.  Just needed to measure overhead
}

bool
OverheadMeasure::clockTic(Cycle_t UNUSED(cycle))
{
    // Just end simulation
    primaryComponentOKToEndSim();

    // Take myself off the clock list
    return true;
}
