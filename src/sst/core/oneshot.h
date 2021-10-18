// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_ONESHOT_H
#define SST_CORE_ONESHOT_H

#include "sst/core/action.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"

#include <cinttypes>

#define _ONESHOT_DBG(fmt, args...) __DBG(DBG_ONESHOT, OneShot, fmt, ##args)

namespace SST {

class TimeConverter;

/**
 * A OneShot Event class.
 *
 * Calls callback functions (handlers) on a specified period
 */
class OneShot : public Action
{
public:
    /**
       Base handler for OneShot callbacks.
     */
    using HandlerBase = SSTHandlerBaseNoArgs<void, false>;

    /**
       Used to create handlers for clock.  The callback function is
       expected to be in the form of:

         void func()

       In which case, the class is created with:

         new OneShot::Handler<classname>(this, &classname::function_name)

       Or, to add static data, the callback function is:

         void func(dataT data)

       and the class is created with:

         new OneShot::Handler<classname, dataT>(this, &classname::function_name, data)
     */
    template <typename classT, typename dataT = void>
    using Handler = SSTHandlerNoArgs<void, classT, false, dataT>;


    /////////////////////////////////////////////////

    /** Create a new One Shot for a specified time that will callback the
        handler function.
        Note: OneShot cannot be canceled, and will always callback after
              the timedelay.
    */
    OneShot(TimeConverter* timeDelay, int priority = ONESHOTPRIORITY);
    ~OneShot();

    /** Is OneShot scheduled */
    bool isScheduled() { return m_scheduled; }

    /** Add a handler to be called on this OneShot Event */
    void registerHandler(OneShot::HandlerBase* handler);

    /** Print details about the OneShot */
    void print(const std::string& header, Output& out) const override;

private:
    typedef std::vector<OneShot::HandlerBase*> HandlerList_t;

    // Since this only gets fixed latency events, the times will fire
    // in order of arrival.  No need to use a full map, a double ended
    // queue will work just as well
    // typedef std::map<SimTime_t, HandlerList_t*> HandlerVectorMap_t;
    typedef std::deque<std::pair<SimTime_t, HandlerList_t*>> HandlerVectorMap_t;

    // Generic constructor for serialization
    OneShot() {}

    // Called by the Simulation (Activity Queue) when delay time as elapsed
    void execute(void) override;

    // Activates this OneShot object, by inserting into the simulation's
    // timeVortex for future execution.
    void      scheduleOneShot();
    SimTime_t computeDeliveryTime();

    TimeConverter*     m_timeDelay;
    HandlerVectorMap_t m_HandlerVectorMap;
    bool               m_scheduled;
};

} // namespace SST

#endif // SST_CORE_ONESHOT_H
