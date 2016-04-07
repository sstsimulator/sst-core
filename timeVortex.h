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

#ifndef SST_CORE_TIMEVORTEX_H
#define SST_CORE_TIMEVORTEX_H

#include <sst/core/serialization.h>

#include <cstdio> // For printf
#include <functional>
#include <queue>
#include <vector>

#include <sst/core/activityQueue.h>

namespace SST {

class Output;

/**
 * Primary Event Queue
 */
class TimeVortex : public ActivityQueue {
public:
	TimeVortex();
    ~TimeVortex();

    bool empty();
    int size();
    void insert(Activity* activity);
    Activity* pop();
    Activity* front();

    /** Print the state of the TimeVortex */
    void print(Output &out) const;

    uint64_t getCurrentDepth() const { return current_depth; }
    uint64_t getMaxDepth() const { return max_depth; }
    
private:
#ifdef SST_ENFORCE_EVENT_ORDERING
    typedef std::priority_queue<Activity*, std::vector<Activity*>, Activity::pq_less_time_priority_order> dataType_t;
#else
    typedef std::priority_queue<Activity*, std::vector<Activity*>, Activity::pq_less_time_priority> dataType_t;
#endif
    dataType_t data;
    uint64_t insertOrder;

    uint64_t current_depth;
    uint64_t max_depth;
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        printf("begin TimeVortex::serialize\n");
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ActivityQueue);
        printf("  - TimeVortex::data\n");
//        ar & BOOST_SERIALIZATION_NVP(data);
        printf("end TimeVortex::serialize\n");
    }
};

} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::TimeVortex)

#endif // SST_CORE_TIMEVORTEX_H
