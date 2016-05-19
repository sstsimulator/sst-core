// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_ACTIVITYQUEUE_H
#define SST_CORE_ACTIVITYQUEUE_H

#include <sst/core/serialization.h>

#include <sst/core/activity.h>

namespace SST {

/** Base Class for a queue of Activities
 */
class ActivityQueue {
public:
    ActivityQueue() {}
    virtual ~ActivityQueue() {}

    /** Returns true if the queue is empty */
    virtual bool empty() = 0;
    /** Returns the number of activities in the queue */
    virtual int size() = 0;
    /** Remove and return the next activity */
    virtual Activity* pop() = 0;
    /** Insert a new activity into the queue */
    virtual void insert(Activity* activity) = 0;
    /** Returns the next activity */
    virtual Activity* front() = 0;

private:
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
    }
};
 
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::ActivityQueue)

#endif // SST_CORE_ACTIVITYQUEUE_H
