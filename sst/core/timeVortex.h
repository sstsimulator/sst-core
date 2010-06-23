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


#ifndef SST_TIMEVORTEX_H
#define SST_TIMEVORTEX_H

#include <set>

#include <sst/core/activityQueue.h>

namespace SST {

class TimeVortex : public ActivityQueue {
public:
	TimeVortex();
    ~TimeVortex();

    bool empty();
    int size();
    void insert(Activity* activity);
    Activity* pop();
    Activity* front();
    
private:
    std::multiset<Activity*,Activity::less_time_priority> data;
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ActivityQueue);
        ar & BOOST_SERIALIZATION_NVP(data);
    }
};

}

#endif // SST_TIMEVORTEX_H
