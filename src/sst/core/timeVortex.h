// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_TIMEVORTEX_H
#define SST_CORE_TIMEVORTEX_H

#include <sst/core/activityQueue.h>
#include <sst/core/module.h>

namespace SST {

class Output;

/**
 * Primary Event Queue
 */
class TimeVortex : public ActivityQueue, public Module {
public:
	TimeVortex() {
        max_depth = MAX_SIMTIME_T;
    }
    ~TimeVortex() {}

    // Inherited from ActivityQueue
    virtual bool empty() override = 0;
    virtual int size() override = 0;
    virtual void insert(Activity* activity) override = 0;
    virtual Activity* pop() override = 0;
    virtual Activity* front() override = 0;

    /** Print the state of the TimeVortex */
    virtual void print(Output &out) const = 0;
    virtual uint64_t getMaxDepth() const { return max_depth; }
    virtual uint64_t getCurrentDepth() const = 0;

    
protected:
    uint64_t max_depth;
    
};

} //namespace SST

#endif // SST_CORE_TIMEVORTEX_H
