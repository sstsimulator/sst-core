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

#ifndef SST_CORE_SYNCQUEUE_H
#define SST_CORE_SYNCQUEUE_H

//#include <sst/core/serialization.h>

#include <vector>

#include <sst/core/activityQueue.h>
#include <sst/core/threadsafe.h>

namespace SST {

/**
 * \class SyncQueue
 *
 * Internal API
 *
 * Activity Queue for use by Sync Objects
 */
class SyncQueue : public ActivityQueue {
public:

    struct Header {
        uint32_t mode;
        uint32_t count;
        uint32_t buffer_size;
    };
    
    SyncQueue();
    ~SyncQueue();

    bool empty() override;
    int size() override;
    void insert(Activity* activity) override;
    Activity* pop() override; // Not a good idea for this particular class
    Activity* front() override;

    // Not part of the ActivityQueue interface
    /** Clear elements from the queue */
    void clear();
    /** Accessor method to the internal queue */
    char* getData();

    uint64_t getDataSize() {
        return buf_size + (activities.capacity() * sizeof(Activity*));
    }
    
private:
    char* buffer;
    size_t buf_size;
    std::vector<Activity*> activities;

    Core::ThreadSafe::Spinlock slock;
};

 
} //namespace SST

#endif // SST_CORE_SYNCQUEUE_H
