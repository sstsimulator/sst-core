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
    // WatchPoint doesn't use the key, so just return 0
    uintptr_t registerHandler(const AttachPointMetaData& UNUSED(mdata)) override { return 0; }

    // Functions inherited from Event::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Event* UNUSED(ev)) override
    {
        if ( handler & BEFORE_EVENT ) {
            printf("  Before Event Handler\n");
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
                    setBufferReset();
                    wpAction->invokeAction(this);
                }
            }
            else {
                printf("    No trace buffer\n");
                if ( trigger ) {
                    wpAction->invokeAction(this);
                }
            }
        } // if AFTER_EVENT
    }

    void afterHandler(uintptr_t UNUSED(key)) override
    {
        if ( handler & AFTER_EVENT ) {
            printf("  After Event Handler\n");
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
                    setBufferReset();
                    wpAction->invokeAction(this);
                }
            }
            else {
                printf("    No trace buffer\n");
                if ( trigger ) {
                    wpAction->invokeAction(this);
                }
            }
        } // if AFTER_EVENT
    }

    // Functions inherited from Clock::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Cycle_t& UNUSED(cycle)) override
    {
        if ( handler & BEFORE_CLOCK ) {
            printf("  Before Clock Handler\n");
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
                    setBufferReset();
                    wpAction->invokeAction(this);
                }
            }
            else {
                printf("    No trace buffer\n");
                if ( trigger ) {
                    wpAction->invokeAction(this);
                }
            }
        } // if AFTER_CLOCK
    }

    void afterHandler(uintptr_t UNUSED(key), const bool& UNUSED(ret)) override
    {
        if ( handler & AFTER_CLOCK ) {
            printf("  After Clock Handler\n");
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
                    setBufferReset();
                    wpAction->invokeAction(this);
                }
            }
            else {
                printf("    No trace buffer\n");
                if ( trigger ) {
                    wpAction->invokeAction(this);
                }
            }
        } // if AFTER_CLOCK
    }

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

    void printTriggerRecord()
    {
        if ( tb_ != nullptr ) {
            tb_->dumpTriggerRecord();
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

    enum HANDLER : unsigned {
        // Select which handlers do check and sample
        BEFORE_CLOCK = 1,
        AFTER_CLOCK  = 2,
        BEFORE_EVENT = 4,
        AFTER_EVENT  = 8,
        ALL          = 15
    };

    void setHandler(unsigned handlerType) { handler = handlerType; }

    void printHandler()
    {
        if ( handler == ALL ) {
            std::cout << "ALL ";
        }
        else {
            if ( handler & BEFORE_CLOCK ) std::cout << "BC ";
            if ( handler & AFTER_CLOCK ) std::cout << "AC ";
            if ( handler & BEFORE_EVENT ) std::cout << "BE ";
            if ( handler & AFTER_EVENT ) std::cout << "AE ";
        }
        std::cout << ": ";
    }

    void printWatchpoint()
    {
        printHandler();
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

    void resetTraceBuffer()
    {
        if ( tb_ != nullptr ) {
            tb_->resetTraceBuffer();
        }
        else {
            std::cout << "No tracing enabled\n";
        }
    }

    bool checkReset() { return reset_; }

    class WPAction
    {
    public:
        WPAction() {}
        virtual ~WPAction() = default;

        virtual std::string actionToString()             = 0;
        virtual void        invokeAction(WatchPoint* wp) = 0;
    };

    class InteractiveWPAction : public WPAction
    {
    public:
        InteractiveWPAction() {}
        virtual ~InteractiveWPAction() = default;

        std::string actionToString() override { return "interactive"; }

        void invokeAction(WatchPoint* wp) override
        {
            printf("    SetInteractive\n");
            wp->setEnterInteractive(); // Trigger action
            wp->setInteractiveMsg(format_string("Watch point %s buffer", wp->name_.c_str()));
            // Note that the interactive action is delayed and
            // we want to be able to print the Trace Buffer there.
            // So, resetTraceBuffer for this case is in handlers
        }
    };

    class PrintTraceWPAction : public WPAction
    {
    public:
        PrintTraceWPAction() {}
        virtual ~PrintTraceWPAction() = default;

        std::string actionToString() override { return "printTrace"; }

        void invokeAction(WatchPoint* wp) override
        {
            wp->printTrace();
            if ( wp->checkReset() ) wp->resetTraceBuffer();
        }
    };

    class CheckpointWPAction : public WPAction
    {
    public:
        CheckpointWPAction() {}
        virtual ~CheckpointWPAction() = default;

        std::string actionToString() override { return "checkpoint"; }

        void invokeAction(WatchPoint* wp) override
        {
            wp->setCheckpoint();
            if ( wp->checkReset() ) wp->resetTraceBuffer();
        }
    };

    class PrintStatusWPAction : public WPAction
    {
    public:

        PrintStatusWPAction() {}
        virtual ~PrintStatusWPAction() = default;

        std::string actionToString() override { return "printStatus"; }

        void invokeAction(WatchPoint* wp) override
        {
            wp->printStatus();
            if ( wp->checkReset() ) wp->resetTraceBuffer();
        }
    };

    class SetVarWPAction : public WPAction
    {
    public:
        SetVarWPAction(std::string vname, Core::Serialization::ObjectMap* obj, std::string tval) :
            name_(vname),
            obj_(obj),
            valStr_(tval)
        {}

        virtual ~SetVarWPAction() = default;

        std::string actionToString() override { return "set " + name_ + " " + valStr_; }

        void invokeAction(WatchPoint* wp) override
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

    private:
        std::string                     name_   = "";
        Core::Serialization::ObjectMap* obj_    = nullptr;
        std::string                     valStr_ = "";
    };

    void setAction(WPAction* action) { wpAction = action; }

    void printAction() { std::cout << wpAction->actionToString(); }

    void addTraceBuffer(Core::Serialization::TraceBuffer* tb) { tb_ = tb; }

    void addObjectBuffer(Core::Serialization::ObjectBuffer* ob) { tb_->addObjectBuffer(ob); }

    void addComparison(Core::Serialization::ObjectMapComparison* cmp)
    {
        cmpObjects_.push_back(cmp);
        numCmpObj_++;
    }

    enum LogicOp : unsigned { // Logical Op for trigger tests
        AND       = 0,
        OR        = 1,
        UNDEFINED = 2
    };

    void addLogicOp(LogicOp op) { logicOps_.push_back(op); }


protected:
    bool      getInteractive();
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

    unsigned  handler = ALL;
    bool      trigger = false;
    bool      reset_  = false;
    WPAction* wpAction;

    void setBufferReset()
    {
        if ( tb_ != nullptr ) {
            printf("    Set Buffer Reset\n");
            tb_->setBufferReset();
            reset_ = true;
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
