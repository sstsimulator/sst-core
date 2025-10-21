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
    static const uint32_t VMASK = 0x10; // see simpleDebug.h::VERBOSITY_MASK

    /**
       Base class for performing comparisons and logic operations for
       determining when the WatchPoint triggers
     */
    class Logic
    {
    public:
        virtual bool check() = 0;
        virtual ~Logic()     = default;
    }; // class Logic

    /**
        WatchPoint Action Inner Classes
    */
    class WPAction
    {
    public:
        WPAction() {}
        virtual ~WPAction()                              = default;
        virtual std::string actionToString()             = 0;
        virtual void        invokeAction(WatchPoint* wp) = 0;
        inline void         setVerbosity(uint32_t v) { verbosity = v; }

    protected:
        uint32_t    verbosity = 0;
        inline void msg(const std::string& msg)
        {
            if ( WatchPoint::VMASK & verbosity ) std::cout << msg << std::endl;
        }
    }; // class WPAction

    class InteractiveWPAction : public WPAction
    {
    public:
        InteractiveWPAction() {}
        virtual ~InteractiveWPAction() = default;
        inline std::string actionToString() override { return "interactive"; }
        void               invokeAction(WatchPoint* wp) override;
    }; // class InteractiveWPAction

    class PrintTraceWPAction : public WPAction
    {
    public:
        PrintTraceWPAction() {}
        virtual ~PrintTraceWPAction() = default;
        inline std::string actionToString() override { return "printTrace"; }
        void               invokeAction(WatchPoint* wp) override;
    }; // class PrintTraceWPAction

    class CheckpointWPAction : public WPAction
    {
    public:
        CheckpointWPAction() {}
        virtual ~CheckpointWPAction() = default;
        inline std::string actionToString() override { return "checkpoint"; }
        void               invokeAction(WatchPoint* wp) override;
    }; // class CheckpointWPAction

    class PrintStatusWPAction : public WPAction
    {
    public:
        PrintStatusWPAction() {}
        virtual ~PrintStatusWPAction() = default;
        inline std::string actionToString() override { return "printStatus"; }
        void               invokeAction(WatchPoint* wp) override;
    }; // class PrintStatusWPAction

    class SetVarWPAction : public WPAction
    {
    public:
        SetVarWPAction(std::string vname, Core::Serialization::ObjectMap* obj, std::string tval) :
            name_(vname),
            obj_(obj),
            valStr_(tval)
        {}
        virtual ~SetVarWPAction() = default;
        inline std::string actionToString() override { return "set " + name_ + " " + valStr_; }
        void               invokeAction(WatchPoint* wp) override;

    private:
        std::string                     name_   = "";
        Core::Serialization::ObjectMap* obj_    = nullptr;
        std::string                     valStr_ = "";
    }; // class SetVarWPAction

    class ShutdownWPAction : public WPAction
    {
    public:
        ShutdownWPAction() {}
        virtual ~ShutdownWPAction() = default;
        inline std::string actionToString() override { return "shutdown"; }
        void               invokeAction(WatchPoint* wp) override;
    }; // class ShutdownWPAction

    // Construction
    WatchPoint(size_t index, const std::string& name, Core::Serialization::ObjectMapComparison* obj);
    ~WatchPoint();

    // Inherited from both Event and Clock handler AttachPoints.
    // WatchPoint doesn't use the key, so just return 0
    uintptr_t registerHandler(const AttachPointMetaData& UNUSED(mdata)) override { return 0; }

    // Functions inherited from Event::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Event* UNUSED(ev)) override;
    void afterHandler(uintptr_t UNUSED(key)) override;

    // Functions inherited from Clock::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Cycle_t& UNUSED(cycle)) override;
    void afterHandler(uintptr_t UNUSED(key), const bool& UNUSED(ret)) override;

    // Local
    inline std::string getName() { return name_; }
    size_t             getBufferSize();
    void               printTriggerRecord();
    void               printTrace();

    enum HANDLER : unsigned {
        // Select which handlers do check and sample
        NONE         = 0,
        BEFORE_CLOCK = 1,
        AFTER_CLOCK  = 2,
        BEFORE_EVENT = 4,
        AFTER_EVENT  = 8,
        ALL          = 15
    };

    // TODO can we avoid code duplication in WatchPoint and the WatchPointAction?
    inline void setVerbosity(uint32_t v)
    {
        verbosity = v;
        wpAction->setVerbosity(v);
    }
    inline void msg(const std::string& msg)
    {
        if ( VMASK & verbosity ) std::cout << msg << std::endl;
    }
    void        setHandler(unsigned handlerType);
    std::string handlerToString(HANDLER h);
    void        printHandler();
    void        printWatchpoint();
    void        resetTraceBuffer();
    inline bool checkReset() { return reset_; }
    void        printAction();
    void        addTraceBuffer(Core::Serialization::TraceBuffer* tb);
    void        addObjectBuffer(Core::Serialization::ObjectBuffer* ob);
    void        addComparison(Core::Serialization::ObjectMapComparison* cmp);

    enum LogicOp : unsigned { // Logical Op for trigger tests
        AND       = 0,
        OR        = 1,
        UNDEFINED = 2
    };
    inline void addLogicOp(LogicOp op) { logicOps_.push_back(op); }
    inline void setAction(WPAction* action) { wpAction = action; }

protected:
    bool      getInteractive();
    void      setEnterInteractive();
    void      setInteractiveMsg(const std::string& msg);
    SimTime_t getCurrentSimCycle();
    void      setCheckpoint();
    void      printStatus();
    void      heartbeat();
    void      simulationShutdown();

private:
    size_t                                                 numCmpObj_ = 0;
    std::vector<Core::Serialization::ObjectMapComparison*> cmpObjects_;
    std::vector<LogicOp>                                   logicOps_;
    std::string                                            name_;
    Core::Serialization::TraceBuffer*                      tb_ = nullptr;
    size_t                                                 wpIndex;
    HANDLER                                                handler        = ALL;
    bool                                                   trigger        = false;
    HANDLER                                                triggerHandler = HANDLER::NONE;
    bool                                                   reset_         = false;
    WPAction*                                              wpAction;

    void     setBufferReset();
    void     check();
    uint32_t verbosity = 0;


}; // class WatchPoint


} // namespace SST


#endif // SST_CORE_WATCHPOINT_H
