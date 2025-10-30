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

//clang-format off
#include "sst_config.h"

#include "sst/core/watchPoint.h"

#include "sst/core/output.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_types.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
//clang-format on

namespace SST {

void
WatchPoint::InteractiveWPAction::invokeAction(WatchPoint* wp)
{
    msg("    SetInteractive");
    wp->setEnterInteractive(); // Trigger action
    std::string handlerStr = wp->handlerToString(wp->triggerHandler);
    wp->setInteractiveMsg(format_string("  WP%ld: %s : %s ...", wp->wpIndex, handlerStr.c_str(), wp->name_.c_str()));
    // Note that the interactive action is delayed and
    // we want to be able to print the Trace Buffer there.
    // So, resetTraceBuffer for this case is in handlers
}

void
WatchPoint::PrintTraceWPAction::invokeAction(WatchPoint* wp)
{
    wp->printTrace();
    if ( wp->checkReset() ) wp->resetTraceBuffer();
}

void
WatchPoint::CheckpointWPAction::invokeAction(WatchPoint* wp)
{
    wp->setCheckpoint();
    if ( wp->checkReset() ) wp->resetTraceBuffer();
}

void
WatchPoint::PrintStatusWPAction::invokeAction(WatchPoint* wp)
{
    wp->printStatus();
    if ( wp->checkReset() ) wp->resetTraceBuffer();
}

void
WatchPoint::SetVarWPAction::invokeAction(WatchPoint* wp)
{
    try {
        obj_->set(valStr_);
    }
    catch ( std::exception& e ) {
        printf("Invalid set var: %s\n", valStr_.c_str());
        return;
    }

    // Can this somehow be tied to debug?
    wp->printTriggerRecord();
    printf("%s\n", actionToString().c_str());

    if ( wp->checkReset() ) wp->resetTraceBuffer();
}

void
WatchPoint::ShutdownWPAction::invokeAction(WatchPoint* wp)
{
    wp->printTriggerRecord();
    printf("  Trigger action shutting down simulation\n");
    wp->simulationShutdown();
    return;
}

WatchPoint::WatchPoint(size_t index, const std::string& name, Core::Serialization::ObjectMapComparison* obj) :
    Clock::HandlerBase::AttachPoint(),
    Event::HandlerBase::AttachPoint(),
    name_(name),
    wpIndex(index)
{
    addComparison(obj);
}

WatchPoint::~WatchPoint() {}

void
WatchPoint::beforeHandler(uintptr_t UNUSED(key), const Event* UNUSED(ev))
{
    if ( handler & BEFORE_EVENT ) {
        msg("  Before Event Handler");
        check();
        if ( tb_ ) {

            if ( reset_ && !getInteractive() ) {
                tb_->resetTraceBuffer();
                reset_ = false;
            }

            SimTime_t cycle  = getCurrentSimCycle();
            bool      invoke = tb_->sampleT(trigger, cycle, "BE");
            trigger          = false;
            if ( invoke ) {
                triggerHandler = HANDLER::BEFORE_EVENT;
                setBufferReset();
                wpAction->invokeAction(this);
                triggerHandler = HANDLER::NONE;
            }
        }
        else {
            msg("    No trace buffer");
            if ( trigger ) {
                triggerHandler = HANDLER::BEFORE_EVENT;
                wpAction->invokeAction(this);
                trigger        = false;
                triggerHandler = HANDLER::NONE;
            }
        }
    } // if BEFORE_EVENT
}

void
WatchPoint::afterHandler(uintptr_t UNUSED(key))
{
    if ( handler & AFTER_EVENT ) {
        msg("  After Event Handler");
        check();
        if ( tb_ ) {

            if ( reset_ && !getInteractive() ) {
                tb_->resetTraceBuffer();
                reset_ = false;
            }

            SimTime_t cycle  = getCurrentSimCycle();
            bool      invoke = tb_->sampleT(trigger, cycle, "AE");
            trigger          = false;
            if ( invoke ) {
                triggerHandler = HANDLER::AFTER_EVENT;
                setBufferReset();
                wpAction->invokeAction(this);
                triggerHandler = HANDLER::NONE;
            }
        }
        else {
            msg("    No trace buffer");
            if ( trigger ) {
                triggerHandler = HANDLER::AFTER_EVENT;
                wpAction->invokeAction(this);
                trigger        = false;
                triggerHandler = HANDLER::NONE;
            }
        }
    } // if AFTER_EVENT
}

void
WatchPoint::beforeHandler(uintptr_t UNUSED(key), const Cycle_t& UNUSED(cycle))
{
    if ( handler & BEFORE_CLOCK ) {
        msg("  Before Clock Handler");
        check();
        if ( tb_ ) {
            if ( reset_ && !getInteractive() ) {
                tb_->resetTraceBuffer();
                reset_ = false;
            }

            SimTime_t cycle  = getCurrentSimCycle();
            bool      invoke = tb_->sampleT(trigger, cycle, "BC");
            trigger          = false;
            if ( invoke ) {
                triggerHandler = HANDLER::BEFORE_CLOCK;
                setBufferReset();
                wpAction->invokeAction(this);
                triggerHandler = HANDLER::NONE;
            }
        }
        else {
            msg("    No trace buffer");
            if ( trigger ) {
                triggerHandler = HANDLER::BEFORE_CLOCK;
                wpAction->invokeAction(this);
                trigger        = false;
                triggerHandler = HANDLER::NONE;
            }
        }
    } // if AFTER_CLOCK
}

void
WatchPoint::afterHandler(uintptr_t UNUSED(key), const bool& UNUSED(ret))
{
    if ( handler & AFTER_CLOCK ) {
        msg("  After Clock Handler");
        check();
        if ( tb_ ) {
            if ( reset_ && !getInteractive() ) {
                tb_->resetTraceBuffer();
                reset_ = false;
            }

            SimTime_t cycle  = getCurrentSimCycle();
            bool      invoke = tb_->sampleT(trigger, cycle, "AC");
            trigger          = false;
            if ( invoke ) {
                triggerHandler = HANDLER::AFTER_CLOCK;
                setBufferReset();
                wpAction->invokeAction(this);
                triggerHandler = HANDLER::NONE;
            }
        }
        else {
            msg("    No trace buffer");
            if ( trigger ) {
                triggerHandler = HANDLER::AFTER_CLOCK;
                wpAction->invokeAction(this);
                trigger        = false;
                triggerHandler = HANDLER::NONE;
            }
        }
    } // if AFTER_CLOCK
}

size_t
WatchPoint::getBufferSize()
{
    if ( tb_ != nullptr ) {
        return tb_->getBufferSize();
    }
    else {
        return 0;
    }
}

void
WatchPoint::printTriggerRecord()
{
    if ( tb_ != nullptr ) {
        tb_->dumpTriggerRecord();
    }
}

void
WatchPoint::printTrace()
{
    if ( tb_ != nullptr ) {
        tb_->dumpTriggerRecord();
        tb_->dumpTraceBufferT();
    }
    else {
        printf("  No tracing enabled\n");
    }
}

void
WatchPoint::setHandler(unsigned handlerType)
{
    handler = static_cast<HANDLER>(handlerType);
}

std::string
WatchPoint::handlerToString(HANDLER h)
{
    std::string result = "";
    if ( h == HANDLER::NONE ) {
        result = "NONE";
    }
    else if ( h == HANDLER::ALL ) {
        result = "ALL";
    }
    else {
        if ( h & HANDLER::BEFORE_CLOCK ) {
            result = "BC";
        }
        if ( h & HANDLER::AFTER_CLOCK ) {
            if ( result == "" ) {
                result = "AC";
            }
            else {
                result = result + " AC";
            }
        }
        if ( h & HANDLER::BEFORE_EVENT ) {
            if ( result == "" ) {
                result = "BE";
            }
            else {
                result = result + " BE";
            }
        }
        if ( h & HANDLER::AFTER_EVENT ) {
            if ( result == "" ) {
                result = "AE";
            }
            else {
                result = result + " AE";
            }
        }
    }
    return result;
}

void
WatchPoint::printHandler()
{
    std::cout << handlerToString(handler);
    std::cout << " : ";
}

void
WatchPoint::printWatchpoint()
{
    printHandler();
    // TODO: print the logic values
    for ( size_t i = 0; i < numCmpObj_; i++ ) { // Print trigger tests
        cmpObjects_[i]->print(std::cout);
    }
    std::cout << " : ";

    if ( tb_ != nullptr ) { // print trace buffer config
        tb_->printConfig();
        std::cout << " : ";
    }
    printAction();
    std::cout << std::endl;
}

void
WatchPoint::resetTraceBuffer()
{
    if ( tb_ != nullptr ) {
        tb_->resetTraceBuffer();
    }
    else {
        std::cout << "No tracing enabled\n";
    }
}

bool
WatchPoint::getInteractive()
{
    return Simulation_impl::getSimulation()->enter_interactive_;
}

void
WatchPoint::setEnterInteractive()
{
    Simulation_impl::getSimulation()->enter_interactive_ = true;
}

void
WatchPoint::setInteractiveMsg(const std::string& msg)
{
    Simulation_impl::getSimulation()->interactive_msg_ = msg;
}

SimTime_t
WatchPoint::getCurrentSimCycle()
{
    return Simulation_impl::getSimulation()->getCurrentSimCycle();
}

void
WatchPoint::setCheckpoint()
{
    if ( Simulation_impl::getSimulation()->checkpoint_directory_ == "" ) {
        std::cout << "Invalid action: checkpointing not enabled (use --checkpoint-enable cmd line option)\n";
    }
    else {
        Simulation_impl::getSimulation()->scheduleCheckpoint();
    }
}

void
WatchPoint::printStatus()
{
    Simulation_impl::getSimulation()->printStatus(true);
}

void
WatchPoint::heartbeat()
{
    // do nothing for now, need to add all the functions similar to RTAction
    // Could it just use RTAction?
}

void
WatchPoint::simulationShutdown()
{
    Simulation_impl::getSimulation()->endSimulation();
}

void
WatchPoint::printAction()
{
    std::cout << wpAction->actionToString();
}

void
WatchPoint::addTraceBuffer(Core::Serialization::TraceBuffer* tb)
{
    tb_ = tb;
}

void
WatchPoint::addObjectBuffer(Core::Serialization::ObjectBuffer* ob)
{
    tb_->addObjectBuffer(ob);
}

void
WatchPoint::addComparison(Core::Serialization::ObjectMapComparison* cmp)
{
    cmpObjects_.push_back(cmp);
    numCmpObj_++;
}

void
WatchPoint::setBufferReset()
{
    if ( tb_ != nullptr ) {
        printf("    Set Buffer Reset\n");
        tb_->setBufferReset();
        reset_ = true;
    }
}

void
WatchPoint::check()
{
    bool result = false;

    if ( cmpObjects_[0]->compare() ) {
        result = true;
    }
    std::stringstream s;
    s << std::boolalpha;
    s << "    WatchPoint " << name_.c_str() << " tests:\n";
    s << "      ";
    cmpObjects_[0]->print(s);
    s << " -> " << result << std::endl;

    for ( size_t i = 1; i < numCmpObj_; i++ ) {
        bool result2 = false;
        if ( cmpObjects_[i]->compare() ) {
            result2 = true;
        }
        s << "      ";
        cmpObjects_[i]->print(s);
        s << " -> " << result2 << std::endl;
        // printf("      comparison%ld = %d\n", i, result2);

        if ( logicOps_[i - 1] == LogicOp::AND ) {
            result = result && result2;
            s << "        AND -> " << result << std::endl;
        }
        else if ( logicOps_[i - 1] == LogicOp::OR ) {
            result = result || result2;
            s << "        OR -> " << result << std::endl;
        }
        else {
            s << "    ERROR: invalid LogicOp\n";
            // Should trigger some error?
        }
    }
    if ( result == true ) trigger = true;

    // print the message if verbosity mask matches
    msg(s.str());
}

} // namespace SST
