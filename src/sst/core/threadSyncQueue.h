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


#ifndef SST_CORE_THREADSYNCQUEUE_H
#define SST_CORE_THREADSYNCQUEUE_H

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
    bool empty() override {
        return activities.empty();
    }
    
    /** Returns the number of activities in the queue */
    int size() override {
        return activities.size();
    }
    
    /** Not supported */
    Activity* pop() override {
        // Need to fatal
        return NULL;
    }
    
    /** Insert a new activity into the queue */
    void insert(Activity* activity) override {
        activities.push_back(activity);
    }
    
    /** Not supported */
    Activity* front() override {
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
