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

#ifndef SST_CORE_WATCHPOINT_H
#define SST_CORE_WATCHPOINT_H

#include "sst/core/clock.h"
#include "sst/core/event.h"
#include "sst/core/serialization/objectMap.h"
#include "sst/core/stringize.h"

namespace SST {


/**
   Class that can attach to Clock and Event Handlers to monitor the
   state of variables
 */
class WatchPoint : public Clock::HandlerBase::AttachPoint, public Event::HandlerBase::AttachPoint
{
public:
    /**
       Base class for performing comparisons and logic operations for
       determining when the WatchPoint triggers
     */
    class Logic
    {
    public:
        virtual bool check() = 0;
        virtual ~Logic()     = default;
    };

    WatchPoint(const std::string& name, Core::Serialization::ObjectMapComparison* obj) :
        Clock::HandlerBase::AttachPoint(),
        Event::HandlerBase::AttachPoint(),
        // obj_(obj),
        name_(name)
    {
        addComparison(obj);
    }

    ~WatchPoint() { delete obj_; }

    // Inherited from both Event and Clock handler AttachPoints.
    // WatchPoint doesn't use the key, so just return 0.
    uintptr_t registerHandler(const AttachPointMetaData& UNUSED(mdata)) override { return 0; }

    // Functions inherited from Event::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Event* UNUSED(ev)) override {}

    void afterHandler(uintptr_t UNUSED(key)) override
    {
        printf("  After Event Handler\n");
        check();
        if ( tb_ ) {
            SimTime_t cycle  = getCurrentSimCycle();
            bool      invoke = tb_->sampleT(trigger, cycle);
            trigger          = false;
            if ( invoke ) {
                invokeAction();
                setBufferReset();
            }
        }
        else {
            printf("    No trace buffer\n");
            if ( trigger ) {
                invokeAction();
                setBufferReset();
            }
        }
    }

    // Functions inherited from Clock::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Cycle_t& UNUSED(cycle)) override {}

    void afterHandler(uintptr_t UNUSED(key), const bool& UNUSED(ret)) override {}

    std::string getName() { return name_; }

    size_t getBufferSize()
    {
        if ( tb_ != nullptr ) {
            return tb_->getBufferSize();
        }
        else {
            return 0;
        }
    }

    void printTrace()
    {
        if ( tb_ != nullptr ) {
            tb_->dumpTriggerRecord();
            tb_->dumpTraceBufferT();
        }
        else {
            printf("  No tracing enabled\n");
        }
    }

    void printWatchpoint()
    {
        // TODO: print the logic values
        for ( size_t i = 0; i < numCmpObj_; i++ ) { // Print trigger tests
            cmpObjects_[i]->print();
        }
        std::cout << " : ";

        if ( tb_ != nullptr ) { // print trace buffer config
            tb_->printConfig();
            std::cout << " : ";
        }
        printAction();
        std::cout << std::endl;
    }

    void setBufferReset()
    {
        if ( tb_ != nullptr ) {
            printf("    Set Buffer Reset\n");
            tb_->setBufferReset();
        }
    }

    void resetTraceBuffer()
    {
        if ( tb_ != nullptr ) {
            printf("    Reset Trace Buffer\n");
            tb_->resetTraceBuffer();
        }
    }


    enum WPACTION : unsigned { // Watchpoint Action
        INTERACTIVE  = 0,
        PRINT_TRACE  = 1,
        CHECKPOINT   = 2,
        PRINT_STATUS = 3,
        HEARTBEAT    = 4,
        INVALID      = 5
    };
    enum LogicOp : unsigned { // Logical Op for trigger tests
        AND       = 0,
        OR        = 1,
        UNDEFINED = 2
    };

    void setAction(WPACTION actionType) { wpAction = actionType; }

    std::string actionToString(WPACTION wpa)
    {

        switch ( wpa ) {
        case INTERACTIVE:
            return "interactive";
        case PRINT_TRACE:
            return "printTrace";
        case CHECKPOINT:
            return "checkpoint";
        case HEARTBEAT:
            return "heartbeat";
        case INVALID:
            return "invalid action";
        default:
            return "undefined action";
        }
    }

    void printAction() { std::cout << actionToString(wpAction); }

    void addTraceBuffer(Core::Serialization::TraceBuffer* tb) { tb_ = tb; }

    void addObjectBuffer(Core::Serialization::ObjectBuffer* ob) { tb_->addObjectBuffer(ob); }

    void addComparison(Core::Serialization::ObjectMapComparison* cmp)
    {
        cmpObjects_.push_back(cmp);
        numCmpObj_++;
    }

    void addLogicOp(LogicOp op) { logicOps_.push_back(op); }


protected:
    void      setEnterInteractive();
    void      setInteractiveMsg(const std::string& msg);
    SimTime_t getCurrentSimCycle();
    void      setCheckpoint();
    void      printStatus();
    void      heartbeat();

private:
    Core::Serialization::ObjectMapComparison*              obj_;
    size_t                                                 numCmpObj_ = 0;
    std::vector<Core::Serialization::ObjectMapComparison*> cmpObjects_;
    std::vector<LogicOp>                                   logicOps_;
    std::string                                            name_;
    Core::Serialization::TraceBuffer*                      tb_ = nullptr;
#if 0
    enum CHECK_HANDLER : unsigned { 
      // Do we want to be able to specify clock/event?
      // Could expand to choose before/after also
        CLOCK = 0, // check in CLOCK handler ONLY
        EVENT = 1, // check in EVENT hanlder ONLY
        BOTH  = 2,  // check in BOTH CLOCK and EVENT handlers
    };
    CHECK_HANDLER checkType = EVENT;
#endif
    int checkType = 0; // clock only - Not currently used. I just hard-coded in event

    bool     trigger  = false;
    WPACTION wpAction = INTERACTIVE;


    void invokeAction()
    {
        switch ( wpAction ) {
        case INTERACTIVE:
            setEnterInteractive(); // Trigger action
            setInteractiveMsg(format_string("Watch point %s buffer", name_.c_str()));
            break;
        case PRINT_TRACE:
            tb_->dumpTraceBufferT();
            break;
        case CHECKPOINT:
            setCheckpoint();
            break;
        case PRINT_STATUS:
            printStatus();
            break;
        case HEARTBEAT:
            heartbeat();
            break;
        default:
            printf("ERROR: invalid watchpoint action\n");
        }
    }

    void check()
    {
        bool result = false;

        if ( cmpObjects_[0]->compare() ) {
            result = true;
        }
        std::cout << std::boolalpha;
        std::cout << "    WatchPoint " << name_.c_str() << " tests:\n";
        std::cout << "      ";
        cmpObjects_[0]->print();
        std::cout << " -> " << result << std::endl;

        for ( size_t i = 1; i < numCmpObj_; i++ ) {
            bool result2 = false;
            if ( cmpObjects_[i]->compare() ) {
                result2 = true;
            }
            std::cout << "      ";
            cmpObjects_[i]->print();
            std::cout << " -> " << result2 << std::endl;
            // printf("      comparison%ld = %d\n", i, result2);

            if ( logicOps_[i - 1] == LogicOp::AND ) {
                result = result && result2;
                std::cout << "        AND -> " << result << std::endl;
            }
            else if ( logicOps_[i - 1] == LogicOp::OR ) {
                result = result || result2;
                std::cout << "        OR -> " << result << std::endl;
            }
            else {
                std::cout << "    ERROR: invalid LogicOp\n";
                // Should trigger some error?
            }
        }
        if ( result == true ) trigger = true;
    }
};


} // namespace SST


#endif // SST_CORE_WATCHPOINT_H
