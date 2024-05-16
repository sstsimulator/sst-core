// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXBINNEDMAP_H
#define SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXBINNEDMAP_H

#include <atomic>
#include <queue>
#include <sst/core/timeVortex.h>
#include <vector>

namespace SST {

class Output;

namespace IMPL {


template <typename T>
class Pool
{

    std::vector<T*> pool;

public:
    ~Pool()
    {
        for ( auto x : pool ) {
            delete x;
        }
    }

    T* remove()
    {
        if ( pool.empty() ) { pool.push_back(new T()); }
        auto ret = pool.back();
        pool.pop_back();
        return ret;
    }

    void insert(T* item) { pool.push_back(item); }
};

/**
 * Primary Event Queue
 */
template <bool TS>
class TimeVortexBinnedMapBase : public TimeVortex
{

private:
    //  Class to hold a vector and a delivery time
    class TimeUnit
    {
    private:
        SimTime_t              sort_time;
        std::vector<Activity*> activities;
        bool                   sorted;

        CACHE_ALIGNED(SST::Core::ThreadSafe::Spinlock, tu_lock);

    public:
        // Create with initial event
        TimeUnit() : sorted(false) {}

        ~TimeUnit()
        {
            for ( auto x : activities ) {
                delete x;
            }
        }

        inline SimTime_t getSortTime() { return sort_time; }
        inline void      setSortTime(SimTime_t time) { sort_time = time; }


        // Inserts can happen by multiple threads
        void insert(Activity* act)
        {
            if ( TS ) tu_lock.lock();
            activities.push_back(act);
            sorted = false;
            if ( TS ) tu_lock.unlock();
        }

        // pop only happens by one thread
        Activity* pop()
        {
            if ( 0 == activities.size() ) return nullptr;
            if ( !sorted ) sort();
            auto ret = activities.back();
            activities.pop_back();
            return ret;
        }

        // front only happens by one thread
        Activity* front()
        {
            if ( 0 == activities.size() ) return nullptr;
            if ( !sorted ) sort();
            return activities.back();
        }

        void sort();

        inline bool operator<(const TimeUnit& rhs) { return this->sort_time < rhs.sort_time; }

        /** To use with STL priority queues, that order in reverse. */
        class pq_less
        {
        public:
            /** Compare pointers */
            inline bool operator()(const TimeUnit* lhs, const TimeUnit* rhs) { return lhs->sort_time > rhs->sort_time; }

            /** Compare references */
            inline bool operator()(const TimeUnit& lhs, const TimeUnit& rhs) { return lhs.sort_time > rhs.sort_time; }
        };
    };


public:
    TimeVortexBinnedMapBase(Params& params);
    ~TimeVortexBinnedMapBase();

    bool      empty() override;
    int       size() override;
    void      insert(Activity* activity) override;
    Activity* pop() override;
    Activity* front() override;


    /** Print the state of the TimeVortex */
    void print(Output& out) const override;

    uint64_t getCurrentDepth() const override { return current_depth; }
    uint64_t getMaxDepth() const override { return max_depth; }

private:
    // Should only ever be accessed by the "active" thread.  Not safe
    // for concurrent access.
    TimeUnit* current_time_unit;

    typedef std::map<SimTime_t, TimeUnit*> mapType_t;

    // Accessed by multiple threads, must be locked when accessing
    mapType_t                                                            map;
    typename std::conditional<TS, std::atomic<uint64_t>, uint64_t>::type insertOrder;

    typename std::conditional<TS, std::atomic<uint64_t>, uint64_t>::type current_depth;

    // Should only ever be accessed by the "active" thread, or in a
    // mutex.  There are no internal mutexes.
    Pool<TimeUnit> pool;

    CACHE_ALIGNED(SST::Core::ThreadSafe::Spinlock, slock);
};

} // namespace IMPL
} // namespace SST

#endif // SST_CORE_IMPL_TIMEVORTEX_TIMEVORTEXBINNEDMAP_H
