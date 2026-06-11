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

// #include <assert.h>

#include "sst_config.h"

#include "sst/core/testElements/coreTest_ClockerComponent.h"

#include "sst/core/warnmacros.h"

#include <cstdint>
#include <iostream>
#include <ostream>

namespace SST::CoreTestClockerComponent {

void
coreTestClockerComponent::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::Component::serialize_order(ser);
    SST_SER(id_);
    SST_SER(inst_count_);
    SST_SER(done_);
    SST_SER(left_);
    SST_SER(right_);
    SST_SER(inst_link_);
    SST_SER(master_);
    SST_SER(clocks_);
}


coreTestClockerComponent::coreTestClockerComponent(ComponentId_t id, Params& UNUSED(params)) :
    Component(id)
{
    test_tc = TimeConverter(test_period);

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Setup the links and set my id_ if I am ID 0
    left_ = configureLink(
        "left", new Event::Handler<coreTestClockerComponent, &coreTestClockerComponent::handle_left>(this));
    right_ = configureLink("right");

    if ( nullptr == left_ ) id_ = 0;

    inst_link_ = configureSelfLink(
        "inst_link", new Event::Handler<coreTestClockerComponent, &coreTestClockerComponent::inst_handler>(this));

    // Register the master clock to fire every 50ns, but immediately remove it if I'm not ID 0
    master_ = registerClock<coreTestClockerComponent, &coreTestClockerComponent::master_handler>(master_period, this);
    if ( id_ != 0 ) {
        master_->deactivate();
    }

    // Register the "test" handlers


    // Old style clock registration
    Clock::HandlerBase* handler =
        new Clock::Handler<coreTestClockerComponent, &coreTestClockerComponent::test_handler, int>(this, 0);
    registerClock(test_tc, handler);
    clocks_.push_back({ handler, test_tc, total_count, false });

    handler = new Clock::Handler<coreTestClockerComponent, &coreTestClockerComponent::test_handler, int>(this, 1);
    registerClock(test_tc, handler);
    clocks_.push_back({ handler, test_tc, total_count, false });

    // New style clock registration
    handler = registerClock<coreTestClockerComponent, &coreTestClockerComponent::test_handler, int>("1ns", this, 2);
    clocks_.push_back({ handler, test_tc, total_count, true });

    handler = registerClock<coreTestClockerComponent, &coreTestClockerComponent::test_handler, int>("1ns", this, 3);
    clocks_.push_back({ handler, test_tc, total_count, true });

    // Unregister all the clocks for now
    unregisterClock(test_tc, clocks_[0].handler);
    unregisterClock(test_tc, clocks_[1].handler);
    clocks_[2].handler->deactivate();
    clocks_[3].handler->deactivate();
}

void
coreTestClockerComponent::setup()
{
    if ( id_ != 0 ) return;
    getSimulationOutput().output("%d: starting test sequence at %" PRIu64 "\n", id_, getCurrentSimCycle());
    if ( right_ == nullptr ) return;
    IntEvent* ev = new IntEvent(1);
    // Delay this first one by a clock period to line up starting times
    right_->send(1, master_period, ev);
}

void
coreTestClockerComponent::handle_left(Event* ev)
{
    IntEvent* int_ev = static_cast<IntEvent*>(ev);
    id_              = int_ev->data++;
    if ( right_ ) right_->send(int_ev);
    master_->activate();
    getSimulationOutput().output("%d: Starting test sequence at %" PRIu64 "\n", id_, getCurrentSimCycle());
}


bool
coreTestClockerComponent::master_handler(Cycle_t UNUSED(cycle))
{
    if ( done_ ) return true;
    OpEvent* ev = new OpEvent(instructions[inst_count_++]);
    inst_link_->send(ev);
    return false;
}

void
coreTestClockerComponent::inst_handler(Event* ev)
{
    OpEvent* op_ev = static_cast<OpEvent*>(ev);

    OpBundle op = op_ev->data;
    delete op_ev;

    ClockInfo& info = clocks_[op.clock];
    Cycle_t    next = 0;

    switch ( op.op ) {
    case Op::nop:
        break;
    case Op::start:
        if ( info.new_style ) {
            next = info.handler->activate();
        }
        else {
            next = reregisterClock(info.period, info.handler);
        }
        getSimulationOutput().output("%d: Clock %d will restart at cycle %" PRIu64 "\n", id_, op.clock, next);
        break;
    case Op::stop:
        if ( info.new_style ) {
            info.handler->deactivate();
        }
        else {
            unregisterClock(info.period, info.handler);
        }
        getSimulationOutput().output(
            "%d: Stopping Clock %d at time %" PRIu64 "\n", id_, op.clock, getCurrentSimCycle());
        break;
    case Op::term:
        primaryComponentOKToEndSim();
        done_ = true;
        getSimulationOutput().output("%d: Terminating test sequence at %" PRIu64 "\n", id_, getCurrentSimCycle());
        break;
    }
}

bool
coreTestClockerComponent::test_handler(Cycle_t cycle, int clock_index)
{
    getSimulationOutput().output("%d: Clock %d at cycle %" PRIu64 "\n", id_, clock_index, cycle);
    int64_t& counter = clocks_[clock_index].counter;
    switch ( clock_index ) {
    case 0:
    case 2:
        counter--;
        if ( counter == 0 ) {
            getSimulationOutput().output(
                "%d: Self stopping Clock %d at time %" PRIu64 "\n", id_, clock_index, getCurrentSimCycle());
            counter = total_count;
            return true;
        }
        break;
    case 1:
    case 3:
    default:
        break;
    }
    return false;
}

} // namespace SST::CoreTestClockerComponent
