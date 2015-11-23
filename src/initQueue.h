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

#ifndef SST_CORE_INITQUEUE_H
#define SST_CORE_INITQUEUE_H

#include <sst/core/serialization.h>

#include <cstdio> // For printf
#include <deque>

#include <sst/core/activityQueue.h>

namespace SST {

/**
 * ActivityQueue for use during the init() phase
 */
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

} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::InitQueue)

#endif // SST_CORE_INITQUEUE_H
