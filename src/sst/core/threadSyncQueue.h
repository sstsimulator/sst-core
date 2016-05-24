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


#ifndef SST_CORE_THREADSYNCQUEUE_H
#define SST_CORE_THREADSYNCQUEUE_H

#include <sst/core/serialization.h>

#include <sst/core/activityQueue.h>

namespace SST {

/** Base Class for a queue of Activities
 */
class ThreadSyncQueue : public ActivityQueue {
public:
    ThreadSyncQueue() :
        ActivityQueue()
        {}
    ~ThreadSyncQueue() {}

    /** Returns true if the queue is empty */
    bool empty() {
        return activities.empty();
    }
    
    /** Returns the number of activities in the queue */
    int size() {
        return activities.size();
    }
    
    /** Not supported */
    Activity* pop() {
        // Need to fatal
        return NULL;
    }
    
    /** Insert a new activity into the queue */
    void insert(Activity* activity) {
        activities.push_back(activity);
    }
    
    /** Not supported */
    Activity* front() {
        // Need to fatal
        return NULL;
    }

    void clear() {
        activities.clear();
    }

    std::vector<Activity*>& getVector() {
        return activities;
    }
    
private:
    std::vector<Activity*> activities;
    
};
 
} //namespace SST

#endif // SST_CORE_THREADSYNCQUEUE_H
