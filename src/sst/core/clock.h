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
#include <utility>
#include <vector>

#define _CLE_DBG(fmt, args...) __DBG(DBG_CLOCK, Clock, fmt, ##args)

namespace SST {

class BaseComponent;
class Component;
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
    class HandlerBase;

private:
    friend class BaseComponent;
    friend class Component;
    friend class Simulation;
    // Clock Group for handling ordered clocks
    class Group
    {
        friend class Clock;

        std::vector<HandlerBase*> handlers_;
        int32_t                   inactive_start_ = 0;
        int32_t                   handler_count_  = 0;
        Clock*                    clock_          = nullptr;
        int32_t                   index_;

    public:

        /**
           Get the period of the associated clock as a SimTime_t in units of the core atomic timebase
        */
        TimeConverter getClockPeriod() { return clock_->getPeriod(); }

        /**
           Get the priority of the associated clock as a SimTime_t in units of the core atomic timebase
        */
        int getClockPriority() { return clock_->getPriority(); }

        /**
           Get the next cycle for the clock associated with this group
        */
        SimTime_t getNextCycle() { return clock_->getNextCycle(); }

        /**
           Registers a handler with this group and marks it as active.  This function should only be called once when
           the handler is registered.  To activate or deactivate, call activateHandler()/deactivateHandler().
        */
        void registerHandler(HandlerBase* handler);

        /**
           Activates a handler.
        */
        void activateHandler(HandlerBase* handler);

        /**
           Deactivates a Handler
        */
        void deactivateHandler(HandlerBase* handler);

        /**
           Activates the Group if there are any active handlers
        */
        void activateOnRestart();

        /**
           Call all active handlers
        */
        bool execute(Cycle_t current_cycle);

        void serialize_order(SST::Core::Serialization::serializer& ser);

    private:

        /**
           Swaps locations of an active handler with an inactive handler.  It will also swap the index state of the two
           handlers. No checks will be done to ensure the active state
        */
        static void swapActiveWithInactive(HandlerBase*& active, HandlerBase*& inactive)
        {
            // First swap the indices, then do the actual pointer swap
            int32_t tmp      = -1 - inactive->index_;
            inactive->index_ = -1 - active->index_;
            active->index_   = tmp;

            std::swap(active, inactive);
        }

        /**
           Swaps locations of two handlers with the same active state.  It will also swap the index state of the two
           handlers. No checks will be done to ensure the active states are the same
        */
        static void swapSameActiveState(HandlerBase*& h1, HandlerBase*& h2)
        {
            // First swap the indices, then do the actual pointer swap
            std::swap(h1->index_, h2->index_);
            std::swap(h1, h2);
        }
    };

public:
    /** Create a new clock with a specified period */
    Clock(TimeConverter period, int priority = CLOCKPRIORITY);
    ~Clock() = default; // Handlers are owned by BaseComponent and are deleted there


    /**
       Base classes for clock handlers.  There are three classes:

       HandlerBase: This is a publicly available class that is the base class that contains the API used by elements,
       including checking active status and starting and stopping the handler.
     */

    class HandlerBase : public SSTHandlerBase<bool, Cycle_t>
    {
    public:
        HandlerBase()  = default;
        ~HandlerBase() = default;

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            SSTHandlerBase<bool, Cycle_t>::serialize_order(ser);
            SST_SER(index_);
            SST_SER(order_);
            // No need to serialize clock_ or group_ as they get reset on restart
        }

        ImplementVirtualSerializable(HandlerBase);

        /**
           Get the period of the associated clock as a SimTime_t in units of the core atomic timebase
        */
        TimeConverter getClockPeriod() { return (order_ < 0) ? clock_->getPeriod() : group_->getClockPeriod(); }

        /**
           Get the priority of the associated clock as a SimTime_t in units of the core atomic timebase
        */
        int getClockPriority() { return (order_ < 0) ? clock_->getPriority() : group_->getClockPriority(); }

        /**
           Query whether handler is currently active
        */
        bool isActive() { return index_ >= 0; }

        /**
           Activate this Clock Handler

           @return Time of next time clock handler will fire
        */
        Cycle_t activate()
        {
            if ( order_ < 0 ) {
                clock_->registerHandler(this);
                return clock_->getNextCycle();
            }
            else {
                group_->activateHandler(this);
                return group_->getNextCycle();
            }
        }

        /**
           Deactivate this Clock Handler
        */
        void deactivate()
        {
            if ( order_ < 0 ) {
                bool empty = false;
                clock_->unregisterHandler(this, empty);
            }
            else {
                group_->deactivateHandler(this);
            }
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
        union {
            Clock* clock_ = nullptr;
            Group* group_;
        };


        /**
           Tracks the order for this handler.  If it is less than 0, then the handler is not ordered. If it is ordered,
           then it's contained in a Clock::Group rather than a Clock
        */
        int32_t order_ = -1;

        /**
           Dual purpose variable.  Because adding a bool to this class would require 8-bytes due to padding, the index_
           variable will also be used to indicate if the handler is active or not.  If < 0, then then it is not
           active. The other purpose of the variable is to hold the index where the handler currently resides in the
           handler list of either the Clock or Clock::Group object.  The index for inactive handlers will be stored
           starting at -1 (i.e. -1 means inactive at index 0, -2 means inactive at index 1, etc). If an implementation
           of either Clock or Clock::Group does not store inactive handers in the array, then any negative number can be
           used to indicate that the handler is inactive.
        */
        int32_t index_ = -1;

        /**
           Mark the handler as active.
        */
        void markAsActive()
        {
            if ( index_ >= 0 ) return;
            index_ = -1 - index_;
        }

        /**
           Mark the handler as inactive.
        */
        void markAsInactive()
        {
            if ( index_ < 0 ) return;
            index_ = -1 - index_;
        }

        /**
           Toggle the active state of the handler.
        */
        void toggleActiveState() { index_ = -1 - index_; }
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

    /**
       Register a group with this Clock object
    */
    void registerGroup(Group* group);

    /**
       Activates a group that is inactive.  If group is already active, nothing is done.  There is no need for a
       deactivateGroup() call as deactivation is only done based on return value during execute()
    */
    void activateGroup(Group* group);

    std::string toString() const override;

    /**
       Get the period of the clock as a TimeConverter
    */
    TimeConverter getPeriod() { return period_; }

private:

    Clock();
    Clock(const Clock&)            = delete;
    Clock& operator=(const Clock&) = delete;

    void execute() override;

    // Vectors to hold the handlers and groups
    std::vector<HandlerBase*> handlers_;
    std::vector<Group*>       groups_;
    int32_t                   groups_inactive_start_ = 0;

    Cycle_t       current_cycle_ = 0;
    TimeConverter period_;
    SimTime_t     next_      = 0;
    bool          scheduled_ = false;
    Simulation*   sim_       = nullptr;

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
