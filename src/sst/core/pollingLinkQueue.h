// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_POLLINGLINKQUEUE_H
#define SST_CORE_POLLINGLINKQUEUE_H

#include <set>

#include <sst/core/activityQueue.h>

namespace SST {

/**
 * A link queue which is used for polling only.
 */
class PollingLinkQueue : public ActivityQueue {
public:
    PollingLinkQueue();
    ~PollingLinkQueue();

    bool empty() override;
    int size() override;
    void insert(Activity* activity) override;
    Activity* pop() override;
    Activity* front() override;
    
    
private:
    std::multiset<Activity*,Activity::less_time> data;
    
};

} //namespace SST

#endif // SST_CORE_POLLINGLINKQUEUE_H
