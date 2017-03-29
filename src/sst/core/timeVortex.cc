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


#include "sst_config.h"

#include <sst/core/timeVortex.h>
#include <sst/core/output.h>

#include <sst/core/clock.h>
#include <sst/core/simulation.h>

namespace SST {

TimeVortex::TimeVortex() :
    ActivityQueue(),
    insertOrder(0),
    current_depth(0),
    max_depth(0)
{}

TimeVortex::~TimeVortex()
{
    // Activities in TimeVortex all need to be deleted
    while ( !data.empty() ) {
        Activity *it = data.top();
        delete it;
        data.pop();
    }
}

bool TimeVortex::empty()
{
    return data.empty();
}

int TimeVortex::size()
{
    return data.size();
}

void TimeVortex::insert(Activity* activity)
{
    activity->setQueueOrder(insertOrder++);
    data.push(activity);
    current_depth++;
    if ( current_depth > max_depth ) {
        max_depth = current_depth;
    }
}

Activity* TimeVortex::pop()
{
    if ( data.empty() ) return NULL;
    Activity* ret_val = data.top();
    data.pop();
    current_depth--;
    return ret_val;

}

Activity* TimeVortex::front()
{
    return data.top();
}

void TimeVortex::print(Output &out) const
{
    out.output("TimeVortex state:\n");

//  STL's priority_queue does not support iteration.
//
//    dataType_t::iterator it;
//    for ( it = data.begin(); it != data.end(); it++ ) {
//        (*it)->print("  ", out);
//    }
}



} // namespace SST
