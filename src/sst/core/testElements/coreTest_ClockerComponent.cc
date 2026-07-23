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

/********** ClockSubComponentAPI functions **********/
void
ClockSubComponentAPI::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST::SubComponent::serialize_order(ser);
    SST_SER(handler_);
}

/********** ClockSubComponent functions **********/
ClockSubComponent::ClockSubComponent(
    ComponentId_t id, Params& UNUSED(p), coreTestClockerComponent* comp, TimeConverter period, int clock) :
    ClockSubComponentAPI(id),
    comp_(comp)
{
    handler_ = registerOrderedClock<ClockSubComponent, &ClockSubComponent::clock_handler, int>(period, this, clock);
}

bool
ClockSubComponent::clock_handler(Cycle_t cycle, int clock)
{
    return comp_->test_ordered_handler(cycle, clock);
}

void
ClockSubComponent::serialize_order(SST::Core::Serialization::serializer& ser)
{
    ClockSubComponentAPI::serialize_order(ser);
    SST_SER(comp_);
}


/********** coreTestClockerComponent functions **********/
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
    SST_SER(ordered_var_test_);
    SST_SER(ordered_handler_remove_);
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

    // Register the master clock, but immediately remove it if I'm not ID 0
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


    // Ordered clock testing
    handler = registerOrderedClock<coreTestClockerComponent, &coreTestClockerComponent::test_ordered_handler, int>(
        "1ns", this, 4);
    handler->deactivate();
    clocks_.push_back({ handler, test_tc, total_count, true });

    handler = registerOrderedClock<coreTestClockerComponent, &coreTestClockerComponent::test_ordered_handler, int>(
        "1ns", this, 5);
    handler->deactivate();
    clocks_.push_back({ handler, test_tc, total_count, true });

    // Register some clocks in subcomponents
    Params empty_params;

    // Anonymous subcomponent
    ClockSubComponentAPI* sub = loadAnonymousSubComponent<ClockSubComponentAPI>(
        "coreTestElement.ClockSubComponent", "anon_slot", 0, ComponentInfo::SHARE_NONE, empty_params, this, test_tc, 6);
    handler = sub->getClockHandler();
    handler->deactivate();
    clocks_.push_back({ handler, test_tc, total_count, true });

    // User subcomponent
    sub     = loadUserSubComponent<ClockSubComponentAPI>("user_slot", ComponentInfo::SHARE_NONE, this, test_tc, 7);
    handler = sub->getClockHandler();
    handler->deactivate();
    clocks_.push_back({ handler, test_tc, total_count, true });

    // Back to registering clock here
    handler = registerOrderedClock<coreTestClockerComponent, &coreTestClockerComponent::test_ordered_handler, int>(
        "1ns", this, 8);
    handler->deactivate();
    clocks_.push_back({ handler, test_tc, total_count, true });
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
    const std::vector<OpBundle>& insts = instructions[inst_count_++];
    for ( auto& x : insts ) {
        OpEvent* ev = new OpEvent(x);
        inst_link_->send(ev);
    }
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

bool
coreTestClockerComponent::test_ordered_handler(Cycle_t cycle, int clock_index)
{
    // int64_t& counter = clocks_[clock_index].counter;
    switch ( clock_index ) {
    case 4:
        // Clock 4 turns on ordered handler remove
        ordered_handler_remove_ = true;
        break;
    case 5:
        // Clock 5 will set the variable to 36
        ordered_var_test_ = 36;
        break;
    case 6:
        // Clock 6 will divide by 2
        ordered_var_test_ /= 2;
        break;
    case 7:
        // Clock 7 will subtract 6
        ordered_var_test_ -= 6;
        break;
    case 8:
        // Clock 8 will print the result
        getSimulationOutput().output(
            "%d: ordered clock result = %d (cycle = %" PRIu64 ")\n", id_, ordered_var_test_, cycle);
        break;
    default:
        break;
    }
    if ( ordered_handler_remove_ ) return true;
    return false;
}


} // namespace SST::CoreTestClockerComponent
