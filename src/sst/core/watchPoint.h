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
        obj_(obj),
        name_(name)
    { 
      printf("Constructor: checkType = %d\n", checkType);
      //traceBuffer.resize(bufSize);
      //cycleBuffer.resize(bufSize);
      //bufferTags.resize(bufSize);

    }

    ~WatchPoint() { delete obj_; }

    // Inherited from both Event and Clock handler AttachPoints.
    // WatchPoint doesn't use the key, so just return 0.
    uintptr_t registerHandler(const AttachPointMetaData& UNUSED(mdata)) override { return 0; }

    // Functions inherited from Event::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Event* UNUSED(ev)) override 
    { 
    }

    void afterHandler(uintptr_t UNUSED(key)) override 
    {
        printf("  In after Event Handler\n");
        check();
        if (tb_) {
            bool invoke = tb_->sampleT(trigger);
            trigger = false;
            if (invoke) {
                invokeAction();
                tb_->resetTraceBuffer();
            }
        } else {
            printf("    no trace buffer\n");
            invokeAction();
            tb_->resetTraceBuffer();
        }

    }

    // Functions inherited from Clock::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Cycle_t& UNUSED(cycle)) override {}

    void afterHandler(uintptr_t UNUSED(key), const bool& UNUSED(ret)) override 
    { 
    }

    std::string getName() { return name_; }

protected:
    void setEnterInteractive();
    void setInteractiveMsg(const std::string& msg);
    SimTime_t getCurrentSimCycle();

private:
    Core::Serialization::ObjectMapComparison* obj_;
    std::string                               name_;

#if 0
    enum CHECK_HANDLER : unsigned { //Could expand tddo before/after also
        CLOCK = 0, // check in CLOCK handler ONLY
        EVENT = 1, // check in EVENT hanlder ONLY
        BOTH  = 2,  // check in BOTH CLOCK and EVENT handlers
    };
    CHECK_HANDLER checkType = EVENT;
#endif

    bool trigger = false;
    int checkType = 0; // clock only

    void invokeAction() {

        tb_->dumpTraceBufferT();
        setEnterInteractive();  // Trigger action
        setInteractiveMsg(format_string("Watch point %s buffer", name_.c_str()));

        // Reset trace buffer?
    }

    void check()
    {

        if ( obj_->compare() ) {
            printf("Watch point %s triggered\n", name_.c_str());
            trigger = true;
        }

    }

    // More generic TraceBuffer handling
    Core::Serialization::TraceBuffer* tb_ = nullptr;
public:
    void addTraceBuffer(Core::Serialization::TraceBuffer* tb) {
        tb_ = tb;
    }

    void addObjectBuffer(Core::Serialization::ObjectBuffer* ob) {
        tb_->addObjectBuffer(ob);
    }


};


} // namespace SST


#endif // SST_CORE_WATCHPOINT_H
