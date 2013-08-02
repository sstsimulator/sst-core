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


#ifndef SST_TIMEVORTEX_H
#define SST_TIMEVORTEX_H

#include <set>

#include <sst/core/activityQueue.h>
#include <sst/core/serialization.h>

#include <cstdio> // For printf

namespace SST {

class Output;

class TimeVortex : public ActivityQueue {
public:
	TimeVortex();
    ~TimeVortex();

    bool empty();
    int size();
    void insert(Activity* activity);
    Activity* pop();
    Activity* front();

    void print(Output &out) const;
    
private:
    std::multiset<Activity*,Activity::less_time_priority> data;
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        printf("begin TimeVortex::serialize\n");
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ActivityQueue);
        printf("  - TimeVortex::data\n");
        ar & BOOST_SERIALIZATION_NVP(data);
        printf("end TimeVortex::serialize\n");
    }
};

}

BOOST_CLASS_EXPORT_KEY(SST::TimeVortex)

#endif // SST_TIMEVORTEX_H
