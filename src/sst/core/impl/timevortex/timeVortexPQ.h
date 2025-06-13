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

#ifndef SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H
#define SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H

#include "sst/core/activity.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/threadsafe.h"
#include "sst/core/timeVortex.h"

#include <atomic>
#include <cstdint>
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
    explicit TimeVortexPQBase(Params& params);
    TimeVortexPQBase() = delete;
    ~TimeVortexPQBase();

    bool      empty() override;
    int       size() override;
    void      insert(Activity* activity) override;
    Activity* pop() override;
    Activity* front() override;

    uint64_t getCurrentDepth() const override { return current_depth; }
    uint64_t getMaxDepth() const override { return max_depth; }

    void dbg_print(Output& out) const override;

    void getContents(std::vector<Activity*>& activities) const override;

private:
    using dataType_t = std::priority_queue<Activity*, std::vector<Activity*>, Activity::greater<true, true, true>>;

    // Get a const reference to the underlying data
    const std::vector<Activity*>& getContainer() const
    {
        struct UnderlyingContainer : dataType_t
        {
            using dataType_t::c; // access protected container
        };
        return static_cast<const UnderlyingContainer&>(data).c;
    }

    // Data
    dataType_t data;
    uint64_t   insertOrder;

    // Stats about usage
    uint64_t max_depth;

    // Need current depth to be atomic if we are thread safe
    std::conditional_t<TS, std::atomic<uint64_t>, uint64_t> current_depth;

    CACHE_ALIGNED(SST::Core::ThreadSafe::Spinlock, slock);
};


} // namespace IMPL
} // namespace SST

#endif // SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXPQ_H
