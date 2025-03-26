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
    };

    WatchPoint(const std::string& name, Core::Serialization::ObjectMapComparison* obj) :
        Clock::HandlerBase::AttachPoint(),
        Event::HandlerBase::AttachPoint(),
        obj_(obj),
        name_(name)
    {}

    ~WatchPoint() { delete obj_; }

    // Inherited from both Event and Clock handler AttachPoints.
    // WatchPoint doesn't use the key, so just return 0.
    uintptr_t registerHandler(const AttachPointMetaData& UNUSED(mdata)) override { return 0; }

    // Functions inherited from Event::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Event* UNUSED(ev)) override { check(); }

    void afterHandler(uintptr_t UNUSED(key)) override {}

    // Functions inherited from Clock::HandlerBase::AttachPoint
    void beforeHandler(uintptr_t UNUSED(key), const Cycle_t& UNUSED(cycle)) override {}

    void afterHandler(uintptr_t UNUSED(key), const bool& UNUSED(ret)) override { check(); }

    std::string getName() { return name_; }

protected:
    void setEnterInteractive();
    void setInteractiveMsg(const std::string& msg);

private:
    Core::Serialization::ObjectMapComparison* obj_;
    std::string                               name_;

    void check()
    {
        if ( obj_->compare() ) {
            setEnterInteractive();
            setInteractiveMsg(format_string("Watch point %s triggered", name_.c_str()));
        }
    }
};

} // namespace SST

#endif // SST_CORE_WATCHPOINT_H
