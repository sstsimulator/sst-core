// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_POLLINGLINKQUEUE_H
#define SST_CORE_POLLINGLINKQUEUE_H

#include "sst/core/activityQueue.h"

#include <set>

namespace SST {

/**
 * A link queue which is used for polling only.
 */
class PollingLinkQueue : public ActivityQueue
{
public:
    PollingLinkQueue() = default;
    ~PollingLinkQueue();

    bool      empty() override;
    int       size() override;
    void      insert(Activity* activity) override;
    Activity* pop() override;
    Activity* front() override;

    void serialize_order(SST::Core::Serialization::serializer& ser);

private:
    std::multiset<Activity*, Activity::less<true, false, false>> data;
};

} // namespace SST

#endif // SST_CORE_POLLINGLINKQUEUE_H
