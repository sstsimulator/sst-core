// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_ACTIVITY_H
#define SST_CORE_ACTIVITY_H

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>

#include <sst/core/serialization/serializable.h>

#include <sst/core/output.h>
#include <sst/core/mempool.h>

#include <unordered_map>
#include <cinttypes>
#include <cstring>

#include <errno.h>

// #include <sst/core/serialization/serializable.h>

// Default Priority Settings
#define STOPACTIONPRIORITY     01
#define THREADSYNCPRIORITY     20
#define SYNCPRIORITY           25
#define INTROSPECTPRIORITY     30
#define CLOCKPRIORITY          40
#define EVENTPRIORITY          50
#define MEMEVENTPRIORITY       50
#define BARRIERPRIORITY        75
#define ONESHOTPRIORITY        80
#define STATISTICCLOCKPRIORITY 85
#define FINALEVENTPRIORITY     98
#define EXITPRIORITY           99

#define SST_ENFORCE_EVENT_ORDERING

extern int main(int argc, char **argv);


namespace SST {

/** Base class for all Activities in the SST Event Queue */
class Activity : public SST::Core::Serialization::serializable {
public:
    Activity() {}
    virtual ~Activity() {}

    /** Function which will be called when the time for this Activity comes to pass. */
    virtual void execute(void) = 0;

#ifdef SST_ENFORCE_EVENT_ORDERING

    /** To use with STL container classes. */
    class less_time_priority_order {
    public:
        /** Compare based off pointers */
        inline bool operator()(const Activity* lhs, const Activity* rhs) const {
            if ( lhs->delivery_time == rhs->delivery_time ) {
                if ( lhs->priority == rhs->priority ) {
                    /* TODO:  Handle 64-bit wrap-around */
                    return lhs->enforce_link_order > rhs->enforce_link_order;
                } else {
               	    return lhs->priority > rhs->priority;
                }
            } else {
            	return lhs->delivery_time > rhs->delivery_time;
            }
        }

        /** Compare based off references */
        inline bool operator()(const Activity& lhs, const Activity& rhs) const {
            if ( lhs.delivery_time == rhs.delivery_time ) {
                if ( lhs.priority == rhs.priority ) {
                    /* TODO:  Handle 64-bit wrap-around */
                    return lhs.enforce_link_order > rhs.enforce_link_order;
                } else {
                    return lhs.priority > rhs.priority;
                }
            } else {
            	return lhs.delivery_time > rhs.delivery_time;
            }
        }
    };
    
    /** To use with STL priority queues, that order in reverse. */
    class pq_less_time_priority_order {
    public:
        /** Compare based off pointers */
        inline bool operator()(const Activity* lhs, const Activity* rhs) const {
            if ( lhs->delivery_time == rhs->delivery_time ) {
                if ( lhs->priority == rhs->priority ) {
                    if ( lhs->enforce_link_order == rhs->enforce_link_order ) {
                        /* TODO:  Handle 64-bit wrap-around */
                        return lhs->queue_order > rhs->queue_order;
                    }
                    else {
                        return lhs->enforce_link_order > rhs->enforce_link_order;
                    }
                } else {
               	    return lhs->priority > rhs->priority;
                }
            } else {
            	return lhs->delivery_time > rhs->delivery_time;
            }
        }

        /** Compare based off references */
        inline bool operator()(const Activity& lhs, const Activity& rhs) const {
            if ( lhs.delivery_time == rhs.delivery_time ) {
                if ( lhs.priority == rhs.priority ) {
                    if ( lhs.enforce_link_order == rhs.enforce_link_order ) {
                        return lhs.queue_order > rhs.queue_order;
                    }
                    else {
                        return lhs.enforce_link_order > rhs.enforce_link_order;
                    }
                } else {
                    return lhs.priority > rhs.priority;
                }
            } else {
            	return lhs.delivery_time > rhs.delivery_time;
            }
        }
    };
    
#endif
    
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
                } else {
               	    return lhs->priority > rhs->priority;
                }
            } else {
            	return lhs->delivery_time > rhs->delivery_time;
            }
        }

        /** Compare based off references */
        inline bool operator()(const Activity& lhs, const Activity& rhs) const {
            if ( lhs.delivery_time == rhs.delivery_time ) {
                if ( lhs.priority == rhs.priority ) {
                    /* TODO:  Handle 64-bit wrap-around */
                    return lhs.queue_order > rhs.queue_order;
                } else {
                    return lhs.priority > rhs.priority;
                }
            } else {
            	return lhs.delivery_time > rhs.delivery_time;
            }
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

#ifdef __SST_DEBUG_EVENT_TRACKING__
    virtual void printTrackingInfo(const std::string& header, Output &out) const {
    }
#endif

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
        Core::MemPool *pool = NULL;
        size_t nPools = memPools.size();
        std::thread::id tid = std::this_thread::get_id();
        for ( size_t i = 0 ; i < nPools ; i++ ) {
            PoolInfo_t &p = memPools[i];
            if ( p.tid == tid && p.size == size ) {
                pool = p.pool;
                break;
            }
        }
        if ( NULL == pool ) {
            /* Still can't find it, alloc a new one */
            pool = new Core::MemPool(size+sizeof(PoolData_t));

            std::lock_guard<std::mutex> lock(poolMutex);
            memPools.emplace_back(tid, size, pool);
        }

        PoolData_t *ptr = (PoolData_t*)pool->malloc();
        if ( !ptr ) {
            fprintf(stderr, "Memory Pool failed to allocate a new object.  Error: %s\n", strerror(errno));
            return NULL;
        }
        *ptr = pool;
        return (void*)(ptr+1);
    }


    /** Returns memory for this Activity to the appropriate memory pool */
	void operator delete(void* ptr)
    {
        /* 1) Decrement pointer
         * 2) Determine Pool Pointer
         * 2b) Set Pointer field to NULL to allow tracking
         * 3) Return to pool
         */
        PoolData_t *ptr8 = ((PoolData_t*)ptr) - 1;
        Core::MemPool* pool = *ptr8;
        *ptr8 = NULL;

        pool->free(ptr8);
    }
    void operator delete(void* ptr, std::size_t UNUSED(sz)){
        /* 1) Decrement pointer
         * 2) Determine Pool Pointer
         * 2b) Set Pointer field to NULL to allow tracking
         * 3) Return to pool
         */
        PoolData_t *ptr8 = ((PoolData_t*)ptr) - 1;
        Core::MemPool* pool = *ptr8;
        *ptr8 = NULL;

        pool->free(ptr8);
    };

    static void getMemPoolUsage(uint64_t& bytes, uint64_t& active_activities) {
        bytes = 0;
        active_activities = 0;
        for ( auto && entry : Activity::memPools ) {
            bytes += entry.pool->getBytesMemUsed();
            active_activities += entry.pool->getUndeletedEntries();
        }
    }
    
    static void printUndeletedActivities(const std::string& header, Output &out, SimTime_t before = MAX_SIMTIME_T) {
        for ( auto && entry : Activity::memPools ) {
            const std::list<uint8_t*>& arenas = entry.pool->getArenas();
            size_t arenaSize = entry.pool->getArenaSize();
            size_t elemSize = entry.pool->getElementSize();
            size_t nelem = arenaSize / elemSize;
            for ( auto iter = arenas.begin(); iter != arenas.end(); ++iter ) {
                for ( size_t j = 0; j < nelem; j++ ) {
                    PoolData_t* ptr = (PoolData_t*)((*iter) + (elemSize*j));
                    if ( *ptr != NULL ) {
                        Activity* act = (Activity*)(ptr + 1);
                        if ( act->delivery_time <= before ) {
                            act->print(header, out);
#ifdef __SST_DEBUG_EVENT_TRACKING__
                            act->printTrackingInfo(header + "    ", out);
#endif
                        }
                    }
                }
            }
        }
    }
    
#endif

    // /* Serializable interface methods */
    // void serialize_order(serializer &ser);


    
    
protected:
    /** Set the priority of the Activity */
    void setPriority(int priority) {
        this->priority = priority;
    }

    // Function used by derived classes to serialize data members.
    // This class is not serializable, because not all class that
    // inherit from it need to be serializable.
    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        ser & queue_order;
        ser & delivery_time;
        ser & priority;
#ifdef SST_ENFORCE_EVENT_ORDERING
        ser & enforce_link_order;
#endif
    }    

#ifdef SST_ENFORCE_EVENT_ORDERING
    int32_t   enforce_link_order;
#endif

private:
    uint64_t  queue_order;
    SimTime_t delivery_time;
    int       priority;
#ifdef USE_MEMPOOL
    friend int ::main(int argc, char **argv);
    /* Defined in event.cc */
    typedef Core::MemPool* PoolData_t;
    struct PoolInfo_t {
        std::thread::id tid;
        size_t size;
        Core::MemPool *pool;
        PoolInfo_t(std::thread::id tid, size_t size, Core::MemPool *pool) : tid(tid), size(size), pool(pool)
        { }
    };
    static std::mutex poolMutex;
	static std::vector<PoolInfo_t> memPools;
#endif
};


} //namespace SST


#endif // SST_CORE_ACTIVITY_H
