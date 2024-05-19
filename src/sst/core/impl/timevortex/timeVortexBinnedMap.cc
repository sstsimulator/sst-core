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

#include "sst_config.h"

#include "sst/core/impl/timevortex/timeVortexBinnedMap.h"

#include "sst/core/clock.h"
#include "sst/core/output.h"

#include <algorithm>

namespace SST {
namespace IMPL {

// We sort backwards so we can work from the bottom of the vector
// (faster delete)
static Activity::greater<true, true, true> my_less;

// Sort only happens in one thread
template <bool TS>
void
TimeVortexBinnedMapBase<TS>::TimeUnit::sort()
{
    // Need to sort array in preparation for running through the
    // activities
    std::sort(activities.begin(), activities.end(), my_less);
    sorted = true;
}


template <bool TS>
TimeVortexBinnedMapBase<TS>::TimeVortexBinnedMapBase(Params& UNUSED(params)) :
    TimeVortex(),
    insertOrder(0),
    current_depth(0)
{
    max_depth = 0;

    // Initialize things with with time = 0 TimeUnit
    auto entry = pool.remove();
    entry->setSortTime(0);
    // map.emplace_hint(map.end(), 0, entry);
    current_time_unit = entry;
}

template <bool TS>
TimeVortexBinnedMapBase<TS>::~TimeVortexBinnedMapBase()
{
    // Activities in TimeVortex all need to be deleted, but that
    // happens when the TimeUnit pool goes out of scope.
}

template <bool TS>
bool
TimeVortexBinnedMapBase<TS>::empty()
{
    if ( TS ) slock.lock();
    bool ret = current_depth == 0;
    if ( TS ) slock.unlock();
    return ret;
}

template <bool TS>
int
TimeVortexBinnedMapBase<TS>::size()
{
    return current_depth;
}

template <bool TS>
void
TimeVortexBinnedMapBase<TS>::insert(Activity* activity)
{
    activity->setQueueOrder(insertOrder++);
    SimTime_t sort_time = activity->getDeliveryTime();

    current_depth++;

    // This is not really thread safe, but it's only used for stats,
    // so is okay if it misses something.
    if ( UNLIKELY(current_depth > max_depth) ) { max_depth = current_depth; }

    // Check to see if this event is supposed to be delivered at the
    // current time.  This can only happen if it comes in on a
    // SelfLink with no added latency.  This means that only one
    // thread at a time can access the current time unit.  Thus, no
    // mutex.
    if ( UNLIKELY(sort_time == current_time_unit->getSortTime()) ) {
        current_time_unit->insert(activity);
        return;
    }

    // Look to see if we already have a TimeUnit for this delivery
    // time.  Any access to the map must be protected with a mutex.
    if ( TS ) slock.lock();
    auto element = map.find(sort_time);
    if ( element == map.end() ) {
        // Need to create a new entry in map for this delivery time,
        // put in this activity.
        auto entry = pool.remove();
        entry->setSortTime(sort_time);
        map.emplace_hint(map.end(), sort_time, entry);
        slock.unlock();
        entry->insert(activity);
    }
    else {
        // Don't need to add a new TimeUnit, so drop the lock before
        // inserting.
        slock.unlock();
        // Just drop this into the existing vector
        (*element).second->insert(activity);
    }
}

template <bool TS>
Activity*
TimeVortexBinnedMapBase<TS>::pop()
{
    if ( current_depth == 0 ) return nullptr;
    current_depth--;

    Activity* ret = current_time_unit->pop();
    if ( ret == nullptr ) {
        if ( TS ) slock.lock();
        // Need to get the next TimeUnit

        // Return current time unit to pool
        pool.insert(current_time_unit);
        // Get next time unit
        current_time_unit = map.begin()->second;
        // Erase this time unit from map
        map.erase(current_time_unit->getSortTime());
        if ( TS ) slock.unlock();
        ret = current_time_unit->pop();
    }
    return ret;
}

template <bool TS>
Activity*
TimeVortexBinnedMapBase<TS>::front()
{
    // if ( TS ) slock.lock();
    Activity* ret = current_time_unit->front();
    // Check to see if we need to look at the next timeunit
    if ( ret == nullptr ) {
        auto it = map.begin();
        ret     = it->second->front();
    }
    // if ( TS ) slock.unlock();
    return ret;
}

template <bool TS>
void
TimeVortexBinnedMapBase<TS>::print(Output& out) const
{
    out.output("TimeVortex state:\n");

    // Still need to figure out a reasonable way to print the state
}


class TimeVortexBinnedMap : public TimeVortexBinnedMapBase<false>
{
public:
    SST_ELI_REGISTER_DERIVED(
        TimeVortex,
        TimeVortexBinnedMap,
        "sst",
        "timevortex.map.binned",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "[EXPERIMENTAL] TimeVortex based on std::map with events binned in time buckets.")


    TimeVortexBinnedMap(Params& params) : TimeVortexBinnedMapBase<false>(params) {}
    SST_ELI_EXPORT(TimeVortexBinnedMap)
};

class TimeVortexBinnedMap_ts : public TimeVortexBinnedMapBase<true>
{
public:
    SST_ELI_REGISTER_DERIVED(
        TimeVortex,
        TimeVortexBinnedMap_ts,
        "sst",
        "timevortex.map.binned.ts",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "[EXPERIMENTAL] Thread safe verion of TimeVortex based on std::map with events binned into time buckets.  Do not reference this element directly, just specify sst.timevortex.map.binned and this version will be selected when it is needed based on other parameters.")


    TimeVortexBinnedMap_ts(Params& params) : TimeVortexBinnedMapBase<true>(params) {}
    SST_ELI_EXPORT(TimeVortexBinnedMap_ts)
};


} // namespace IMPL
} // namespace SST
