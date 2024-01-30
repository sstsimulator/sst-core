// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/impl/timevortex/timeVortexPQ.h"

#include "sst/core/clock.h"
#include "sst/core/output.h"

namespace SST {
namespace IMPL {

template <bool TS>
TimeVortexPQBase<TS>::TimeVortexPQBase(Params& UNUSED(params)) :
    TimeVortex(),
    insertOrder(0),
    max_depth(0),
    current_depth(0)
{}

template <bool TS>
TimeVortexPQBase<TS>::TimeVortexPQBase() : TimeVortex(), insertOrder(0), max_depth(0), current_depth(0)
{}

template <bool TS>
TimeVortexPQBase<TS>::~TimeVortexPQBase()
{
    // Activities in TimeVortexPQ all need to be deleted
    while ( !data.empty() ) {
        Activity* it = data.top();
        delete it;
        data.pop();
    }
}

template <bool TS>
bool
TimeVortexPQBase<TS>::empty()
{
    if ( TS ) slock.lock();
    auto ret = data.empty();
    if ( TS ) slock.unlock();
    return ret;
}

template <bool TS>
int
TimeVortexPQBase<TS>::size()
{
    if ( TS ) slock.lock();
    auto ret = data.size();
    if ( TS ) slock.unlock();
    return ret;
}

template <bool TS>
void
TimeVortexPQBase<TS>::insert(Activity* activity)
{
    if ( TS ) slock.lock();
    activity->setQueueOrder(insertOrder++);
    data.push(activity);
    current_depth++;
    if ( current_depth > max_depth ) { max_depth = current_depth; }
    if ( TS ) slock.unlock();
}

template <bool TS>
Activity*
TimeVortexPQBase<TS>::pop()
{
    if ( TS ) slock.lock();
    if ( data.empty() ) return nullptr;
    Activity* ret_val = data.top();
    data.pop();
    current_depth--;
    if ( TS ) slock.unlock();
    return ret_val;
}

template <bool TS>
Activity*
TimeVortexPQBase<TS>::front()
{
    if ( TS ) slock.lock();
    auto ret = data.top();
    if ( TS ) slock.unlock();
    return ret;
}

template <bool TS>
void
TimeVortexPQBase<TS>::print(Output& out) const
{
    out.output("TimeVortex state:\n");

    //  STL's priority_queue does not support iteration.
    /*std::vector<Activity*>& act = const_cast<TimeVortexPQBase<TS>*>(this)->getContainer(data);
        for ( auto it = act.begin(); it != act.end(); it++ ) {
            (*it)->print("  ", out);
        }*/
}

template <bool TS>
void
TimeVortexPQBase<TS>::dbg_print(Output& out)
{
    out.output("TimeVortex state:\n");

    //  STL's priority_queue does not support iteration.
    std::vector<Activity*>& act = getContainer(data);
    for ( auto it = act.begin(); it != act.end(); it++ ) {
        (*it)->print("  ", out);
    }
}

template <bool TS>
void
TimeVortexPQBase<TS>::serialize_order(SST::Core::Serialization::serializer& ser)
{
    TimeVortex::serialize_order(ser);
    ser& insertOrder;
    ser& max_depth;
    ser& current_depth;
    ser& data;
}

template <bool TS>
void
TimeVortexPQBase<TS>::fixup_handlers()
{
    std::vector<Activity*>& act = getContainer(data);
    for ( auto it = act.begin(); it != act.end(); it++ ) {
        fixup(*it);
    }
}

class TimeVortexPQ : public TimeVortexPQBase<false>
{
public:
    SST_ELI_REGISTER_DERIVED(
        TimeVortex,
        TimeVortexPQ,
        "sst",
        "timevortex.priority_queue",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "TimeVortex based on std::priority_queue.")


    TimeVortexPQ(Params& params) : TimeVortexPQBase<false>(params) {}
    TimeVortexPQ() : TimeVortexPQBase<false>() {} // For serialization only
    ~TimeVortexPQ() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) { TimeVortexPQBase<false>::serialize_order(ser); }

    SST_ELI_EXPORT(TimeVortexPQ)
};

class TimeVortexPQ_ts : public TimeVortexPQBase<true>
{
public:
    SST_ELI_REGISTER_DERIVED(
        TimeVortex,
        TimeVortexPQ_ts,
        "sst",
        "timevortex.priority_queue.ts",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Thread safe verion of TimeVortex based on std::priority_queue.  Do not reference this element directly, just specify sst.timevortex.priority_queue and this version will be selected when it is needed based on other parameters.")


    TimeVortexPQ_ts(Params& params) : TimeVortexPQBase<true>(params) {}
    TimeVortexPQ_ts() : TimeVortexPQBase<true>() {} // For serialization only
    ~TimeVortexPQ_ts() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) { TimeVortexPQBase<true>::serialize_order(ser); }

    SST_ELI_EXPORT(TimeVortexPQ_ts)
};

} // namespace IMPL
} // namespace SST
