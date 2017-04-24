// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_TIMEVORTEX_H
#define SST_CORE_TIMEVORTEX_H

#include <functional>
#include <queue>
#include <vector>

#include <sst/core/activityQueue.h>

namespace SST {

class Output;

/**
 * Primary Event Queue
 */
class TimeVortex : public ActivityQueue {
public:
	TimeVortex();
    ~TimeVortex();

    bool empty() override;
    int size() override;
    void insert(Activity* activity) override;
    Activity* pop() override;
    Activity* front() override;

    /** Print the state of the TimeVortex */
    void print(Output &out) const;

    uint64_t getCurrentDepth() const { return current_depth; }
    uint64_t getMaxDepth() const { return max_depth; }
    
private:
#ifdef SST_ENFORCE_EVENT_ORDERING
    typedef std::priority_queue<Activity*, std::vector<Activity*>, Activity::pq_less_time_priority_order> dataType_t;
#else
    typedef std::priority_queue<Activity*, std::vector<Activity*>, Activity::pq_less_time_priority> dataType_t;
#endif
    dataType_t data;
    uint64_t insertOrder;

    uint64_t current_depth;
    uint64_t max_depth;
    
};

} //namespace SST

#endif // SST_CORE_TIMEVORTEX_H
