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

#ifndef SST_CORE_ONESHOTMANAGER_H
#define SST_CORE_ONESHOTMANAGER_H

#include "sst/core/action.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"

#include <cinttypes>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace SST {

class Simulation_impl;

namespace Core {

// Forward declare OneShotManager
class OneShotManager;

// General using for these classes
using TimeStamp_t = std::pair<SimTime_t, int>;


class OneShot : public Action
{
public:

    /**
       Base handler for OneShot callbacks.
     */
    using HandlerBase = SSTHandlerBaseNoArgs<void>;

    /**
       Used to create checkpointable handlers for OneShot.  The callback function is
       expected to be in the form of:

         void func()

       In which case, the class is created with:

         new OneShot::Handler2<classname, &classname::function_name>(this)

       Or, to add static data, the callback function is:

         void func(dataT data)

       and the class is created with:

         new OneShot::Handler2<classname, &classname::function_name, dataT>(this, data)
     */
    template <typename classT, auto funcT, typename dataT = void>
    using Handler2 = SSTHandler2<void, void, classT, dataT, funcT>;

    using HandlerList_t = std::vector<OneShot::HandlerBase*>;

    OneShotManager* manager;
    HandlerList_t&  handlers;
    TimeStamp_t     time;

    OneShot(TimeStamp_t time, OneShotManager* manager, HandlerList_t& handlers) :
        manager(manager),
        handlers(handlers),
        time(time)
    {
        setDeliveryTime(time.first);
        setPriority(time.second);
    }

    ~OneShot() {}

    void execute() override;
};


/**
   Manages the OneShot actions for the core.  This allows handlers to
   be registered with the core that will be called at a specific time
   and priority.
*/
class OneShotManager
{

    using HandlerVectorMap_t = std::map<TimeStamp_t, std::pair<OneShot::HandlerList_t, bool>>;

public:

    /////////////////////////////////////////////////

    /** Create a new One Shot for a specified time that will callback the
        handler function.
        Note: OneShot cannot be canceled, and will always callback after
              the timedelay.
    */
    OneShotManager(Simulation_impl* sim);
    ~OneShotManager();

    /** Add a handler to be called on this OneShot Event */
    template <class classT, auto funcT, typename dataT>
    void registerRelativeHandler(SimTime_t trigger_time, int priority, classT* obj, dataT metadata)
    {
        OneShot::HandlerBase* handler = new OneShot::Handler2<classT, funcT, dataT>(obj, metadata);
        registerHandlerBase(trigger_time, priority, true, handler);
    }

    /** Add a handler to be called on this OneShot Event */
    template <class classT, auto funcT>
    void registerRelativeHandler(SimTime_t trigger_time, int priority, classT* obj)
    {
        OneShot::HandlerBase* handler = new OneShot::Handler2<classT, funcT>(obj);
        registerHandlerBase(trigger_time, priority, true, handler);
    }

    /** Add a handler to be called on this OneShot Event */
    template <class classT, auto funcT, typename dataT>
    void registerAbsoluteHandler(SimTime_t trigger_time, int priority, classT* obj, dataT metadata)
    {
        OneShot::HandlerBase* handler = new OneShot::Handler2<classT, funcT, dataT>(obj, metadata);
        registerHandlerBase(trigger_time, priority, false, handler);
    }

    /** Add a handler to be called on this OneShot Event */
    template <class classT, auto funcT>
    void registerAbsoluteHandler(SimTime_t trigger_time, int priority, classT* obj)
    {
        OneShot::HandlerBase* handler = new OneShot::Handler2<classT, funcT>(obj);
        registerHandlerBase(trigger_time, priority, false, handler);
    }


private:
    friend class OneShot;

    HandlerVectorMap_t handler_vector_map_;
    Simulation_impl*   sim_ = nullptr;

    /**
       Registers a handler for delivery at the specified time
     */
    void registerHandlerBase(SimTime_t trigger_time, int priority, bool relative, OneShot::HandlerBase* handler);

    /**
       Schedules the first entry in handler_vector_map_ if it is not
       already scheduled.
     */
    void scheduleNextOneShot();

    // Function that will be called when a oneshot is done calling its
    // handlers.  This allows the OneShotManager to clean things up
    // and schedule the next OneShot
    void oneshotCallback(TimeStamp_t time, OneShot* oneshot);
};

} // namespace Core
} // namespace SST

#endif // SST_CORE_ONESHOTMANAGER_H
