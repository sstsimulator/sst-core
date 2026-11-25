// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/testElements/coreTest_Payload.h"

#include <cinttypes>
#include <cstddef>
#include <cstdint>

using namespace SST;
using namespace SST::CoreTestPayload;

namespace {
// Value of word i of the payload of the event with sequence number seq
uint32_t
payloadWord(uint64_t seq, size_t i)
{
    return static_cast<uint32_t>(seq * 2654435761u + i * 40503u + 7);
}
} // namespace

coreTestPayloadEvent::coreTestPayloadEvent(uint64_t seq, size_t size) :
    seq(seq),
    words(size),
    str(size, '\0')
{
    for ( size_t i = 0; i < size; ++i ) {
        words[i] = payloadWord(seq, i);
        str[i]   = static_cast<char>(payloadWord(seq, i) >> 24);
    }
}

bool
coreTestPayloadEvent::check(size_t size) const
{
    if ( words.size() != size || str.size() != size ) return false;
    for ( size_t i = 0; i < size; ++i ) {
        if ( words[i] != payloadWord(seq, i) || str[i] != static_cast<char>(payloadWord(seq, i) >> 24) ) return false;
    }
    return true;
}

coreTestPayloadComponent::coreTestPayloadComponent(ComponentId_t id, Params& params) :
    Component(id)
{
    num_cycles       = params.find<uint64_t>("num_cycles", 20);
    events_per_cycle = params.find<uint64_t>("events_per_cycle", 4);
    max_payload      = params.find<uint64_t>("max_payload", 4096);
    init_payload     = params.find<uint64_t>("init_payload", 1000);
    setup_events     = params.find<uint64_t>("setup_events", 0);
    setup_payload    = params.find<uint64_t>("setup_payload", 4096);

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    links[0].name = "Elink";
    links[1].name = "Wlink";
    for ( int i = 0; i < 2; ++i ) {
        links[i].link = configureLink<&coreTestPayloadComponent::handleEvent, int>(links[i].name, "1ns", this, i);
        if ( links[i].link == nullptr ) fatal(CALL_INFO, 1, "ERROR: port %s is not connected\n", links[i].name.c_str());
    }

    registerClock("1GHz", new Clock::Handler<coreTestPayloadComponent, &coreTestPayloadComponent::clockTic>(this));
}

size_t
coreTestPayloadComponent::payloadSize(uint64_t seq) const
{
    if ( seq <= setup_events ) return setup_payload;
    return 1 + (seq * seq * 7919) % max_payload;
}

void
coreTestPayloadComponent::init(unsigned int phase)
{
    // Send one large event on each link in the first phase, and check the events received in later phases
    if ( phase == 0 ) {
        for ( auto& info : links )
            info.link->sendUntimedData(new coreTestPayloadEvent(0, init_payload));
    }

    for ( auto& info : links ) {
        while ( Event* ev = info.link->recvUntimedData() ) {
            auto* pev = dynamic_cast<coreTestPayloadEvent*>(ev);
            if ( pev == nullptr || pev->seq != 0 || !pev->check(init_payload) ) info.errors++;
            info.init++;
            delete ev;
        }
    }
}

void
coreTestPayloadComponent::setup()
{
    // Events sent during setup are transferred between ranks before the run phase starts
    for ( uint64_t i = 0; i < setup_events; ++i ) {
        uint64_t seq = next_seq++;
        for ( auto& info : links )
            info.link->send(new coreTestPayloadEvent(seq, payloadSize(seq)));
    }
}

void
coreTestPayloadComponent::handleEvent(Event* ev, int index)
{
    link_info& info = links[index];
    auto*      pev  = dynamic_cast<coreTestPayloadEvent*>(ev);

    if ( pev == nullptr || pev->seq == 0 || !pev->check(payloadSize(pev->seq)) )
        info.errors++;
    else
        info.bytes += pev->bytes();
    info.events++;

    delete ev;
    checkDone();
}

bool
coreTestPayloadComponent::clockTic(Cycle_t)
{
    if ( cycles_sent == num_cycles ) {
        sending = false;
        checkDone();
        return true;
    }
    cycles_sent++;

    // Both neighbors send events with the same sequence numbers, so each link carries the same payloads in each
    // direction
    for ( uint64_t i = 0; i < events_per_cycle; ++i ) {
        uint64_t seq = next_seq++;
        for ( auto& info : links )
            info.link->send(new coreTestPayloadEvent(seq, payloadSize(seq)));
    }

    // return false so we keep going
    return false;
}

void
coreTestPayloadComponent::checkDone()
{
    if ( ended || sending ) return;
    for ( auto& info : links )
        if ( info.events != setup_events + num_cycles * events_per_cycle ) return;

    ended = true;
    primaryComponentOKToEndSim();
}

void
coreTestPayloadComponent::finish()
{
    for ( auto& info : links ) {
        getSimulationOutput().output("%s %s: received %" PRIu64 " init events, %" PRIu64 " events with %" PRIu64
                                     " payload bytes, %" PRIu64 " errors\n",
            getName().c_str(), info.name.c_str(), info.init, info.events, info.bytes, info.errors);
    }
}
