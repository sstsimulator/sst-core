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


#include "sst_config.h"

#include <sst/core/impl/timevortex/timeVortexPQ.h>

#include <sst/core/output.h>

#include <sst/core/clock.h>
#include <sst/core/simulation.h>

namespace SST {
namespace IMPL {

TimeVortexPQ::TimeVortexPQ(Params& UNUSED(params)) :
    TimeVortex(),
    insertOrder(0),
    current_depth(0),
    max_depth(0)
{}

TimeVortexPQ::~TimeVortexPQ()
{
    // Activities in TimeVortexPQ all need to be deleted
    while ( !data.empty() ) {
        Activity *it = data.top();
        delete it;
        data.pop();
    }
}

bool TimeVortexPQ::empty()
{
    return data.empty();
}

int TimeVortexPQ::size()
{
    return data.size();
}

void TimeVortexPQ::insert(Activity* activity)
{
    activity->setQueueOrder(insertOrder++);
    data.push(activity);
    current_depth++;
    if ( current_depth > max_depth ) {
        max_depth = current_depth;
    }
}

Activity* TimeVortexPQ::pop()
{
    if ( data.empty() ) return NULL;
    Activity* ret_val = data.top();
    data.pop();
    current_depth--;
    return ret_val;

}

Activity* TimeVortexPQ::front()
{
    return data.top();
}

void TimeVortexPQ::print(Output &out) const
{
    out.output("TimeVortex state:\n");

//  STL's priority_queue does not support iteration.
//
//    dataType_t::iterator it;
//    for ( it = data.begin(); it != data.end(); it++ ) {
//        (*it)->print("  ", out);
//    }
}


} // namespace IMPL
} // namespace SST
