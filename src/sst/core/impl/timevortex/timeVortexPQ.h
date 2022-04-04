// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H
#define SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/timeVortex.h"

#include <functional>
#include <queue>
#include <vector>

namespace SST {

class Output;

namespace IMPL {

/**
 * Primary Event Queue
 */
template <bool TS>
class TimeVortexPQBase : public TimeVortex
{

public:
    // TimeVortexPQ();
    TimeVortexPQBase(Params& params);
    ~TimeVortexPQBase();

    bool      empty() override;
    int       size() override;
    void      insert(Activity* activity) override;
    Activity* pop() override;
    Activity* front() override;

    /** Print the state of the TimeVortex */
    void print(Output& out) const override;

    uint64_t getCurrentDepth() const override { return current_depth; }
    uint64_t getMaxDepth() const override { return max_depth; }

private:
    typedef std::priority_queue<Activity*, std::vector<Activity*>, Activity::greater<true, true, true>> dataType_t;

    // Data
    dataType_t data;
    uint64_t   insertOrder;

    // Stats about usage
    uint64_t max_depth;

    // Need current depth to be atomic if we are thread safe
    typename std::conditional<TS, std::atomic<uint64_t>, uint64_t>::type current_depth;

    CACHE_ALIGNED(SST::Core::ThreadSafe::Spinlock, slock);
};


} // namespace IMPL
} // namespace SST

#endif // SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H
