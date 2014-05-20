// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_ACTIVITY_H
#define SST_CORE_ACTIVITY_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <sst/core/output.h>
#include <sst/core/mempool.h>

#include <cstring>
#include <errno.h>

namespace SST {

/** Base class for all Activities in the SST Event Queue */
class Activity {
public:
    Activity() {}
    virtual ~Activity() {}

    /** Function which will be called when the time for this Activity comes to pass. */
    virtual void execute(void) = 0;

    /** Comparator class to use with STL container classes. */
    class less_time_priority {
    public:
        /** Compare based off pointers */
        inline bool operator()(const Activity* lhs, const Activity* rhs) {
            if (lhs->delivery_time == rhs->delivery_time ) return lhs->priority < rhs->priority;
            else return lhs->delivery_time < rhs->delivery_time;
        }

        /** Compare based off references */
        inline bool operator()(const Activity& lhs, const Activity& rhs) {
            if (lhs.delivery_time == rhs.delivery_time ) return lhs.priority < rhs.priority;
            else return lhs.delivery_time < rhs.delivery_time;
        }
    };

    /** To use with STL priority queues, that order in reverse. */
    class pq_less_time_priority {
    public:
        /** Compare based off pointers */
        inline bool operator()(const Activity* lhs, const Activity* rhs) const {
            if ( lhs->delivery_time == rhs->delivery_time ) {
                if ( lhs->priority == rhs->priority ) {
                    /* TODO:  Handle 64-bit wrap-around */
                    return lhs->queue_order > rhs->queue_order;
                }
                return lhs->priority > rhs->priority;
            }
            return lhs->delivery_time > rhs->delivery_time;
        }

        /** Compare based off references */
        inline bool operator()(const Activity& lhs, const Activity& rhs) {
            if ( lhs.delivery_time == rhs.delivery_time ) {
                if ( lhs.priority == rhs.priority ) {
                    /* TODO:  Handle 64-bit wrap-around */
                    return lhs.queue_order > rhs.queue_order;
                }
                return lhs.priority > rhs.priority;
            }
            return lhs.delivery_time > rhs.delivery_time;
        }
    };

    /** Comparator class to use with STL container classes. */
    class less_time {
    public:
        /** Compare pointers */
        inline bool operator()(const Activity* lhs, const Activity* rhs) {
            return lhs->delivery_time < rhs->delivery_time;
        }

        /** Compare references */
        inline bool operator()(const Activity& lhs, const Activity& rhs) {
            return lhs.delivery_time < rhs.delivery_time;
        }
    };

    /** Set the time for which this Activity should be delivered */
    void setDeliveryTime(SimTime_t time) {
        delivery_time = time;
    }

    /** Return the time at which this Activity will be delivered */
    inline SimTime_t getDeliveryTime() const {
        return delivery_time;
    }

    /** Return the Priority of this Activity */
    inline int getPriority() const {
        return priority;
    }

    /** Generic print-print function for this Activity.
     * Subclasses should override this function.
     */
    virtual void print(const std::string& header, Output &out) const {
        out.output("%s Generic Activity to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), delivery_time, priority);
    }

    /** Set a new Queue order */
    void setQueueOrder(uint64_t order) {
        queue_order = order;
    }

#ifdef USE_MEMPOOL
    /** Allocates memory from a memory pool for a new Activity */
	void* operator new(std::size_t size) throw()
    {
        /* 1) Find memory pool
         * 1.5) If not found, create new
         * 2) Alloc item from pool
         * 3) Append PoolID to item, increment pointer
         */
        size_t poolSize = memPools.size();
        Core::MemPool *pool = NULL;
        uint32_t poolID = 0;
        for ( uint32_t i = 0 ; i < poolSize ; i++ ) {
            if ( memPools[i].first == size ) {
                pool = memPools[i].second;
                poolID = i;
                break;
            }
        }
        if ( NULL == pool ) {
            // MemPool size is element size + our poolID
            pool = new Core::MemPool(size+sizeof(uint32_t));
            memPools.push_back(std::make_pair(size, pool));
            poolID = memPools.size() - 1;
        }
        uint32_t *ptr = (uint32_t*)pool->malloc();
        if ( !ptr ) {
            fprintf(stderr, "Memory Pool failed to allocate a new object.  Error: %s\n", strerror(errno));
            return NULL;
        }
        *ptr = poolID;
        return (void*)(ptr+1);
    }


    /** Returns memory for this Activity to the appropriate memory pool */
	void operator delete(void* ptr)
    {
        /* 1) Decrement pointer
         * 2) Determine Pool ID
         * 3) Return to pool
         */
        uint32_t *ptr8 = ((uint32_t*)ptr) - 1;
        uint32_t poolID = *ptr8;
        Core::MemPool* pool = memPools[poolID].second;

        pool->free(ptr8);
    }
#endif

protected:
    /** Set the priority of the Activity */
    void setPriority(int priority) {
        this->priority = priority;
    }


private:
    uint64_t  queue_order;
    SimTime_t delivery_time;
    int       priority;
#ifdef USE_MEMPOOL
	static std::vector<std::pair<size_t, Core::MemPool*> > memPools;
#endif

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_NVP(delivery_time);
        ar & BOOST_SERIALIZATION_NVP(priority);
    }
};

} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Activity)

#endif // SST_CORE_ACTIVITY_H
