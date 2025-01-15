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

#ifndef SST_CORE_SYNC_SYNCQUEUE_H
#define SST_CORE_SYNC_SYNCQUEUE_H

#include "sst/core/activityQueue.h"
#include "sst/core/rankInfo.h"
#include "sst/core/threadsafe.h"

#include <vector>

namespace SST {

/**
   \class SyncQueue

   Internal API

   Base class for all Sync Queues
*/
class SyncQueue : public ActivityQueue
{
public:
    SyncQueue(RankInfo to_rank) : ActivityQueue(), to_rank(to_rank) {}
    ~SyncQueue() {}

    /** Accessor method to get to_rank */
    RankInfo getToRank() { return to_rank; }

private:
    RankInfo to_rank;
};

/**
 * \class SyncQueue
 *
 * Internal API
 *
 * Activity Queue for use by Sync Objects
 */
class RankSyncQueue : public SyncQueue
{
public:
    struct Header
    {
        uint32_t mode;
        uint32_t count;
        uint32_t buffer_size;
    };

    RankSyncQueue(RankInfo to_rank);
    ~RankSyncQueue();

    bool      empty() override;
    int       size() override;
    void      insert(Activity* activity) override;
    Activity* pop() override; // Not a good idea for this particular class
    Activity* front() override;

    // Not part of the ActivityQueue interface
    /** Clear elements from the queue */
    void  clear();
    /** Accessor method to the internal queue */
    char* getData();

    uint64_t getDataSize() { return buf_size + (activities.capacity() * sizeof(Activity*)); }

private:
    char*                  buffer;
    size_t                 buf_size;
    std::vector<Activity*> activities;

    Core::ThreadSafe::Spinlock slock;
};

class ThreadSyncQueue : public SyncQueue
{
public:
    ThreadSyncQueue(RankInfo to_rank) : SyncQueue(to_rank) {}
    ~ThreadSyncQueue() {}

    /** Returns true if the queue is empty */
    bool empty() override { return activities.empty(); }

    /** Returns the number of activities in the queue */
    int size() override { return activities.size(); }

    /** Not supported */
    Activity* pop() override
    {
        // Need to fatal
        return nullptr;
    }

    /** Insert a new activity into the queue */
    void insert(Activity* activity) override { activities.push_back(activity); }

    /** Not supported */
    Activity* front() override
    {
        // Need to fatal
        return nullptr;
    }

    void clear() { activities.clear(); }

    std::vector<Activity*>& getVector() { return activities; }

private:
    std::vector<Activity*> activities;
};

} // namespace SST

#endif // SST_CORE_SYNC_SYNCQUEUE_H
