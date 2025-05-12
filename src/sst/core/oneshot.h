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

#ifndef SST_CORE_ONESHOT_H
#define SST_CORE_ONESHOT_H

#ifndef COMPILING_ONESHOT_CC
#warning \
    "OneShot was not intended to be part of the public element API and has known bugs that will not be fixed. It is being removed from the public API and oneshot.h will be removed in SST 16"
#endif

#include "sst/core/action.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"

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
    using HandlerBase = SSTHandlerBaseNoArgs<void>;

    /**
       Used to create handlers for OneShot.  The callback function is
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
    using Handler
        [[deprecated("Handler has been deprecated. Please use Handler2 instead as it supports checkpointing.")]] =
            SSTHandlerNoArgs<void, classT, dataT>;

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


    /////////////////////////////////////////////////

    /** Create a new One Shot for a specified time that will callback the
        handler function.
        Note: OneShot cannot be canceled, and will always callback after
              the timedelay.
    */
    OneShot(TimeConverter* timeDelay, int priority = ONESHOTPRIORITY) {}
    ~OneShot() {}

    /** Add a handler to be called on this OneShot Event */
    void registerHandler(OneShot::HandlerBase* handler) {}
};

} // namespace SST

#endif // SST_CORE_ONESHOT_H
