// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_POLLINGLINKQUEUE_H
#define SST_POLLINGLINKQUEUE_H

#include <set>

#include <sst/core/activityQueue.h>
#include <sst/core/serialization.h>

#include <cstdio> // For printf

namespace SST {

class PollingLinkQueue : public ActivityQueue {
public:
    PollingLinkQueue();
    ~PollingLinkQueue();

    bool empty();
    int size();
    void insert(Activity* activity);
    Activity* pop();
    Activity* front();
    
    
private:
    std::multiset<Activity*,Activity::less_time> data;
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        printf("begin PollingLinkQueue::serialize\n");
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ActivityQueue);
        printf("  - PollingLinkQueue::data\n");
        ar & BOOST_SERIALIZATION_NVP(data);
        printf("end PollingLinkQueue::serialize\n");
    }
};

}

BOOST_CLASS_EXPORT_KEY(SST::PollingLinkQueue)

#endif // SST_POLLINGLINKQUEUE_H
