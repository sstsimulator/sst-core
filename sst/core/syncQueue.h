// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SYNCQUEUE_H
#define SST_CORE_SYNCQUEUE_H

//#include <sst/core/serialization.h>

#include <cstdio> // For printf
#include <vector>

#include <sst/core/activityQueue.h>

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

    bool empty();
    int size();
    void insert(Activity* activity);
    Activity* pop(); // Not a good idea for this particular class
    Activity* front();

    // Not part of the ActivityQueue interface
    /** Clear elements from the queue */
    void clear();
    /** Accessor method to the internal queue */
    char* getData();
    
private:
    std::vector<char> buffer;
    std::vector<Activity*> activities;
    
};

 
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::SyncQueue)

#endif // SST_CORE_SYNCQUEUE_H
