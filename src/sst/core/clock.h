// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CLOCK_H
#define SST_CORE_CLOCK_H

#include "sst/core/action.h"
#include "sst/core/ssthandler.h"

#include <cinttypes>
#include <vector>

#define _CLE_DBG(fmt, args...) __DBG(DBG_CLOCK, Clock, fmt, ##args)

namespace SST {

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
    Clock(TimeConverter* period, int priority = CLOCKPRIORITY);
    ~Clock();

    /**
       Base handler for clock functions.
     */
    using HandlerBase = SSTHandlerBase<bool, Cycle_t>;

    /**
       Used to create handlers for clock.  The callback function is
       expected to be in the form of:

         bool func(Cycle_t cycle)

       In which case, the class is created with:

         new Clock::Handler<classname>(this, &classname::function_name)

       Or, to add static data, the callback function is:

         bool func(Cycle_t cycle, dataT data)

       and the class is created with:

         new Clock::Handler<classname, dataT>(this, &classname::function_name, data)

       In both cases, the boolean that's returned indicates whether
       the handler should be removed from the list or not.  On return
       of true, the handler will be removed.  On return of false, the
       handler will be left in the clock list.
     */
    template <typename classT, typename dataT = void>
    using Handler = SSTHandler<bool, Cycle_t, classT, dataT>;

    /**
       New style (checkpointable) SSTHandler
    */
    template <typename classT, auto funcT, typename dataT = void>
    using Handler2 = SSTHandler2<bool, Cycle_t, classT, dataT, funcT>;

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

    /** Add a handler to be called on this clock's tick */
    bool registerHandler(Clock::HandlerBase* handler);
    /** Remove a handler from the list of handlers to be called on the clock tick */
    bool unregisterHandler(Clock::HandlerBase* handler, bool& empty);

    /**
       Checks to see if a handler is registered with this clock
    */
    bool isHandlerRegistered(Clock::HandlerBase* handler);

    std::string toString() const override;

private:
    /*     typedef std::list<Clock::HandlerBase*> HandlerMap_t; */
    typedef std::vector<Clock::HandlerBase*> StaticHandlerMap_t;

    Clock() {}

    void execute(void) override;

    Cycle_t            currentCycle;
    TimeConverter*     period;
    StaticHandlerMap_t staticHandlerMap;
    SimTime_t          next;
    bool               scheduled;

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Clock)
};


class ClockHandlerMetaData : public HandlerMetaData
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
