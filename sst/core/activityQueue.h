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


#ifndef SST_ACTIVITYQUEUE_H
#define SST_ACTIVITYQUEUE_H

#include <sst/core/activity.h>

namespace SST {

class ActivityQueue {
public:
    ActivityQueue() {}
    virtual ~ActivityQueue() {}

    virtual bool empty() = 0;
    virtual int size() = 0;
    virtual Activity* pop() = 0;
    virtual void insert(Activity* activity) = 0;
    virtual Activity* front() = 0;
    
private:
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
    }
};
 
}

BOOST_CLASS_EXPORT_KEY(SST::ActivityQueue)

#endif // SST_ACTIVITYQUEUE_H
