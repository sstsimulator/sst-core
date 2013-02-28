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


#ifndef SST_INITQUEUE_H
#define SST_INITQUEUE_H

#include <deque>

#include <sst/core/activityQueue.h>

#include <cstdio> // For printf

namespace SST {

class InitQueue : public ActivityQueue {
public:
    InitQueue();
    ~InitQueue();

    bool empty();
    int size();
    void insert(Activity* activity);
    Activity* pop();
    Activity* front();
    
    
private:
    std::deque<Activity*> data;
    
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

BOOST_CLASS_EXPORT_KEY(SST::InitQueue)

#endif // SST_INITQUEUE_H
