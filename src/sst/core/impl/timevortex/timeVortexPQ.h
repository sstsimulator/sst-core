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

#ifndef SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H
#define SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H

#include <functional>
#include <queue>
#include <vector>

#include <sst/core/timeVortex.h>
#include <sst/core/elementinfo.h>

namespace SST {

class Output;

namespace IMPL {


/**
 * Primary Event Queue
 */
class TimeVortexPQ : public TimeVortex {

public:
    SST_ELI_REGISTER_MODULE(
        TimeVortexPQ,
        "sst",
        "timevortex.priority_queue",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "TimeVortex based on std::priority_queue.",
        "SST::TimeVortex")


public:
	// TimeVortexPQ();
	TimeVortexPQ(Params& params);
    ~TimeVortexPQ();

    bool empty() override;
    int size() override;
    void insert(Activity* activity) override;
    Activity* pop() override;
    Activity* front() override;

    /** Print the state of the TimeVortex */
    void print(Output &out) const override;

    uint64_t getCurrentDepth() const override { return current_depth; }
    uint64_t getMaxDepth() const override { return max_depth; }
    
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

} // namespace IMPL
} //namespace SST

#endif // SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H

