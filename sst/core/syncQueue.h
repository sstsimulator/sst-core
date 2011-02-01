// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_SYNCQUEUE_H
#define SST_SYNCQUEUE_H

#include <vector>

#include <sst/core/activityQueue.h>

#include <cstdio> // For printf

namespace SST {

class SyncQueue : public ActivityQueue {
public:
    SyncQueue();
    ~SyncQueue();

    bool empty();
    int size();
    void insert(Activity* activity);
    Activity* pop(); // Not a good idea for this particular class
    Activity* front();

    // Not part of the ActivityQueue interface
    void clear();
    std::vector<Activity*>* getVector();
    
private:
    std::vector<Activity*> data;
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        printf("begin SyncQueue::serialize\n");
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ActivityQueue);
        printf("  - SyncQueue::data\n");
        ar & BOOST_SERIALIZATION_NVP(data);
        printf("end SyncQueue::serialize\n");
    }
};

}

BOOST_CLASS_EXPORT_KEY(SST::SyncQueue)

#endif // SST_SYNCQUEUE_H
