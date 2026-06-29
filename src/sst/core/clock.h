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

#ifndef SST_CORE_CLOCK_H
#define SST_CORE_CLOCK_H

#include "sst/core/action.h"
#include "sst/core/ssthandler.h"
#include "sst/core/timeConverter.h"

#include <cinttypes>
#include <string>
#include <vector>

#define _CLE_DBG(fmt, args...) __DBG(DBG_CLOCK, Clock, fmt, ##args)

namespace SST {

class BaseComponent;
class Simulation;
class TimeConverter;

/**
 * A Clock class.
 *
 * Calls callback functions (handlers) on a specified period
 */
class Clock : public Action
{
public:
    /** Create a new clock with a specified period */
    Clock(TimeConverter period, int priority = CLOCKPRIORITY);
    ~Clock() = default; // Handlers are owned by BaseComponent and are deleted there


    /**
       Base handler for clock functions.
     */

    /**
       Base Class for Clock::Handlers.  We need to implement HandlerBase as a class because we need a base class with
       more functionality than SSTHandlerBase provides. BaseHandler will track the clock that is associated with,
       whether or not it is active, and an index the Clock object can use to more efficiently remove handlers.
    */
    class HandlerBase : public SSTHandlerBase<bool, Cycle_t>
    {
    public:
        HandlerBase() = default;

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            SSTHandlerBase<bool, Cycle_t>::serialize_order(ser);
            SST_SER(active_);
            // No need to serialize clock_ or index_ as they will be initialized on register on restart
        }

        ImplementVirtualSerializable(HandlerBase);

        /**
           Get the period of the associated clock as a SimTime_t in units of the core atomic timebase
        */
        TimeConverter getClockPeriod() { return clock_->getPeriod(); }

        /**
           Get the priority of the associated clock as a SimTime_t in units of the core atomic timebase
        */
        int getClockPriority() { return clock_->getPriority(); }

        /**
           Query whether handler is currently active
        */
        bool isActive() { return active_; }

        /**
           Activate this Clock Handler

           @return Time of next time clock handler will fire
        */
        Cycle_t activate()
        {
            clock_->registerHandler(this);
            return clock_->getNextCycle();
        }

        /**
           Deactivate this Clock Handler
        */
        void deactivate()
        {
            bool empty = false;
            clock_->unregisterHandler(this, empty);
        }

    private:
        friend class BaseComponent;
        friend class Clock;

        /**
           Clock object associated with this Handler. Clock::Handlers should only ever be registered with one clock
           frequency. If you need multiple clock frequencies, each should get their own handler.

           NOTE: Registering with two different clocks will continue to "work", though it may have unintended
           results. For now, a warning will be issued, but starting at SST 17, this will become an error condition.
        */
        Clock* clock_ = nullptr;

        /**
           Whether or not the handler is currently active on a clock list.  The state is set by Clock whenever a
           handler is registered, reregistered, or unregistered (either by calling unregisterHandler() or by returning
           true from the handler execution)
         */
        bool active_ = false;

        /**
           Variable used by Clock to record the index where this handler is stored in the handler vector.  It will be
           set to -1 if not currently in the vector.
        */
        int index_ = -1;

        /**
           Mark the handler as active
        */
        void markAsActive(int index)
        {
            active_ = true;
            index_  = index;
        }

        /**
           Mark the handler as inactive
        */
        void markAsInactive()
        {
            active_ = false;
            index_  = -1;
        }
    };

    /**
       Used to create handlers for clock.  The callback function is
       expected to be in the form of:

         bool func(Cycle_t cycle)

       In which case, the class is created with:

         new Clock::Handler<classname, &classname::function_name>(this)

       Or, to add static data, the callback function is:

         bool func(Cycle_t cycle, dataT data)

       and the class is created with:

         new Clock::Handler<classname, &classname::function_name, dataT>(this, data)

       In both cases, the boolean that's returned indicates whether
       the handler should be removed from the list or not.  On return
       of true, the handler will be removed.  On return of false, the
       handler will be left in the clock list.
    */
    template <typename classT, auto funcT, typename dataT = void>
    using Handler = SSTHandler<bool, Cycle_t, classT, dataT, funcT, HandlerBase>;

    /**
       Handler2 version which is now the same as Handler and is provided for backward compatibility until SST 17
    */
    template <typename classT, auto funcT, typename dataT = void>
    using Handler2 [[deprecated(
        "The name Handler2 has been deprecated and will be removed in SST 17. Please rename Handler2 to Handler.")]]
    = SSTHandler<bool, Cycle_t, classT, dataT, funcT, HandlerBase>;

    /**
     * Activates this clock object, by inserting into the simulation's
     * timeVortex for future execution.
     */
    void schedule();

    /** Return the time of the next clock tick */
    Cycle_t getNextCycle();

    /**
     * Update current cycle count - needed at simulation end if clock has run
     * ahead of simulation end and to return correct cycle count in getNextCycle()
     * for clocks that are currently not scheduled
     */
    void updateCurrentCycle();

    /**
       Add a handler to be called on this clock's tick
    */
    void registerHandler(Clock::HandlerBase* handler);

    /**
       Remove a handler from the list of handlers to be called on the clock tick
    */
    bool unregisterHandler(Clock::HandlerBase* handler, bool& empty);

    /**
       Add a handler to this Clock object during restart
    */
    void registerHandler_restart(Clock::HandlerBase* handler);

    /**
       Checks to see if a handler is registered with this clock
    */
    bool isHandlerRegistered(Clock::HandlerBase* handler);

    std::string toString() const override;

    /**
       Get the period of the clock as a TimeConverter
    */
    TimeConverter getPeriod() { return period_; }

private:
    using StaticHandlerMap_t = std::vector<Clock::HandlerBase*>;

    Clock() {}

    Clock(const Clock&)            = delete;
    Clock& operator=(const Clock&) = delete;

    void execute() override;

    Cycle_t            current_cycle_;
    TimeConverter      period_;
    StaticHandlerMap_t static_handler_map_;
    SimTime_t          next_;
    bool               scheduled_;

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Clock)
};


class ClockHandlerMetaData : public AttachPointMetaData
{
public:
    const ComponentId_t comp_id;
    const std::string   comp_name;
    const std::string   comp_type;

    ClockHandlerMetaData(ComponentId_t id, const std::string& cname, const std::string& ctype) :
        comp_id(id),
        comp_name(cname),
        comp_type(ctype)
    {}

    ~ClockHandlerMetaData() {}
};


} // namespace SST

#endif // SST_CORE_CLOCK_H
