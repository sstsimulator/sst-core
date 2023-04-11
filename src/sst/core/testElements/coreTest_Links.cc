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

#include "sst/core/testElements/coreTest_Links.h"

#include "sst/core/event.h"

using namespace SST;
using namespace SST::CoreTestComponent;

coreTestLinks::coreTestLinks(ComponentId_t id, Params& params) : Component(id), recv_count(0)
{
    bool found_sendlat;
    bool found_recvlat;

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    my_id = params.find<int>("id", 0);

    UnitAlgebra link_tb  = params.find<UnitAlgebra>("link_time_base", "1ns");
    UnitAlgebra send_lat = params.find<UnitAlgebra>("added_send_latency", found_sendlat);
    UnitAlgebra recv_lat = params.find<UnitAlgebra>("added_recv_latency", found_recvlat);

    // configure out links
    E = configureLink(
        "Elink", link_tb.toString(),
        new Event::Handler<coreTestLinks, std::string>(this, &coreTestLinks::handleEvent, "East"));
    W = configureLink(
        "Wlink", link_tb.toString(),
        new Event::Handler<coreTestLinks, std::string>(this, &coreTestLinks::handleEvent, "West"));

    if ( found_sendlat ) {
        E->addSendLatency(1, send_lat.toString());
        W->addSendLatency(1, send_lat.toString());
    }

    if ( found_recvlat ) {
        E->addRecvLatency(1, recv_lat.toString());
        W->addRecvLatency(1, recv_lat.toString());
    }

    // set our clock
    registerClock("100 MHz", new Clock::Handler<coreTestLinks>(this, &coreTestLinks::clockTic));
}

coreTestLinks::~coreTestLinks() {}

// incoming events are scanned and deleted
void
coreTestLinks::handleEvent(Event* ev, std::string from)
{
    getSimulationOutput().output(
        "%d: received event at: %" PRIu64 " ns on link %s\n", my_id, getCurrentSimTimeNano(), from.c_str());
    delete ev;
    recv_count++;
    if ( recv_count == 8 ) { primaryComponentOKToEndSim(); }
}

bool
coreTestLinks::clockTic(Cycle_t cycle)
{
    // Each clock cycle, send with increasing addtional latency, for 4 cycles, end of 5th
    if ( cycle == 5 ) { return true; }

    E->send(cycle, nullptr);
    W->send(cycle, nullptr);

    // return false so we keep going
    return false;
}

// Element Libarary / Serialization stuff
