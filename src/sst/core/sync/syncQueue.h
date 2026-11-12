// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SYNC_SYNCQUEUE_H
#define SST_CORE_SYNC_SYNCQUEUE_H

#include "sst/core/activityQueue.h"
#include "sst/core/rankInfo.h"
#include "sst/core/serialization/chunked_buffer.h"
#include "sst/core/sst_mpi.h"
#include "sst/core/threadsafe.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace SST {

namespace Profile {
class SyncProfileToolList;
}

/**
   \class SyncQueue

   Internal API

   Base class for all Sync Queues
*/
class SyncQueue : public ActivityQueue
{
public:
    explicit SyncQueue(RankInfo to_rank) :
        ActivityQueue(),
        to_rank(to_rank)
    {}
    ~SyncQueue() {}

    /** Accessor method to get to_rank */
    RankInfo getToRank() { return to_rank; }

    virtual void setProfileTools(Profile::SyncProfileToolList* UNUSED(profile_tools)) {}

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
    /**
       Header at the start of the serialized data sent to another rank.

       The data is held in a chunked_buffer and sent as one MPI message per chunk. The first message is received into
       a buffer of a size known to the sender (see RankSyncRecvBuffer). If the first chunk fits in it, the first message
       holds the first chunk (mode 0). Otherwise the first message holds only this header (mode 1), so that the
       receiver can grow its buffer, and the first chunk is sent with the remaining chunks using a second tag.
    */
    struct Header
    {
        uint32_t mode;
        uint32_t count;
        uint64_t buffer_size; // Total size of the data, including this header
        uint64_t chunk_size;  // Maximum chunk size used by the sender
    };

    explicit RankSyncQueue(RankInfo to_rank);
    ~RankSyncQueue() = default;

    bool      empty() override;
    int       size() override;
    void      insert(Activity* activity) override;
    Activity* pop() override; // Not a good idea for this particular class
    Activity* front() override;

    // Not part of the ActivityQueue interface
    /** Clear elements from the queue */
    void                                 clear();
    /** Serialize the queued activities, starting with a Header, and clear the queue */
    Core::Serialization::chunked_buffer& getData();

#ifdef SST_CONFIG_HAVE_MPI
    /**
       Start sending the data from the last call to getData() to rank dest. The first message uses tag and the rest use
       follow_tag. remote_size is the size of the first message the receiver can accept, and is updated when the
       receiver will grow its buffer. Requests for the sends are appended to reqs.
    */
    void sendData(int dest, int tag, int follow_tag, size_t& remote_size, std::vector<MPI_Request>& reqs);
#endif

    uint64_t getDataSize() { return buffer.capacity() + (activities.capacity() * sizeof(Activity*)); }

    void setProfileTools(Profile::SyncProfileToolList* profile_tools) override { profile_tools_ = profile_tools; }

private:
    Core::Serialization::chunked_buffer buffer;
    std::vector<Activity*>              activities;
    Profile::SyncProfileToolList*       profile_tools_ = nullptr;

    Core::ThreadSafe::Spinlock slock;
};

/**
   \class RankSyncRecvBuffer

   Internal API

   Receives the data sent by RankSyncQueue::sendData() from one remote rank. The buffer is kept between transfers and
   grows as needed.
*/
class RankSyncRecvBuffer
{
public:
    RankSyncRecvBuffer();

    /** Size of the first message that a new RankSyncRecvBuffer accepts. Senders start with this as remote_size. */
    static size_t initialSize();

#ifdef SST_CONFIG_HAVE_MPI
    /** Post the receive for the first message from rank src */
    void postFirstRecv(int src, int tag, MPI_Request* req);

    /**
       After the first message has arrived, post the receives for the remaining messages from rank src. Requests for
       the receives are appended to reqs.
    */
    void postRemainingRecvs(int src, int follow_tag, std::vector<MPI_Request>& reqs);
#endif

    /** After all messages have arrived, deserialize the activities */
    void unpack(std::vector<Activity*>& activities);

    uint64_t getDataSize() const { return buffer.capacity(); }

private:
    Core::Serialization::chunked_buffer buffer;
    size_t                              first_size; // Size of the first message which can be received
};

class ThreadSyncQueue : public SyncQueue
{
public:
    explicit ThreadSyncQueue(RankInfo to_rank) :
        SyncQueue(to_rank)
    {}
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
