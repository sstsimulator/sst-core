// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/mempool.h"

#include "sst/core/mempoolAccessor.h"
#include "sst/core/output.h"
#include "sst/core/threadsafe.h"

#include <list>
#include <sstream>
#include <sys/mman.h>
#include <sys/time.h>
#include <vector>

namespace SST {
namespace Core {

#ifdef USE_MEMPOOL

///////////////////////////////////////////////////////////////////////
// Mempool classes optimized to minimize cross thread interferance     //
///////////////////////////////////////////////////////////////////////

/**
   Class to hold full overflow lists for transfering to other threads.
   This class will require mutexes because it is accessed by multiple
   threads, but the accesser are fast (just swapping vector contents)
   and don't happen often (only when an overflow list reaches max size
   or a mempool runs out of items on its freelist.
 */
class OverflowFreeList
{

    struct OverflowList
    {
        size_t                          size;
        std::vector<std::vector<void*>> lists;

        OverflowList(size_t size, std::vector<void*>& list) : size(size)
        {
            lists.emplace_back();
            lists.back().swap(list);
        }
    };

    std::vector<OverflowList> store;

    ThreadSafe::Spinlock lock;

public:
    void insert(size_t size, std::vector<void*>& list)
    {
        std::lock_guard<ThreadSafe::Spinlock> slock(lock);
        bool                                  found = false;
        for ( auto& x : store ) {
            if ( x.size == size ) {
                found = true;
                x.lists.emplace_back();
                x.lists.back().swap(list);
            }
        }
        if ( !found ) { store.emplace_back(size, list); }
    }

    bool remove(size_t size, std::vector<void*>& list)
    {
        std::lock_guard<ThreadSafe::Spinlock> slock(lock);
        bool                                  found = false;
        for ( auto& x : store ) {
            if ( x.size == size ) {
                if ( !x.lists.empty() ) {
                    list.swap(x.lists.back());
                    x.lists.pop_back();
                    found = true;
                }
            }
        }
        return found;
    }
};


// Controls whether or not the mempools cache align their entries
static bool memPoolCacheAlign = false;


/**
 * Simple Memory Pool class.  The class instance is only ever accessed
 * by a single thread.  Only have to mutex when putting things in the
 * overflow store.
 */
class MemPoolNoMutex
{
    std::vector<void*> freelist;
    std::vector<void*> overflow;

    static OverflowFreeList shared_overflow;

public:
    /** Create a new Memory Pool.
     * @param elementSize - Size of each Element
     * @param initialSize - Size of the memory pool (in bytes)
     */
    MemPoolNoMutex(size_t elementSize, size_t initialSize = (2 << 20)) :
        numAlloc(0),
        numFree(0),
        elemSize(elementSize),
        arenaSize(initialSize),
        max_freelist_size(0)
    {
        if ( memPoolCacheAlign ) {
            // Round up to next multiple of 64 to ensure no events are
            // on the same cache line
            size_t remainder = (elemSize + 8) % 64;
            allocSize        = (remainder == 0) ? (elemSize + 8) : ((elemSize + 8) + 64 - remainder);
        }
        else {
            allocSize = elemSize + 8;
        }
        max_overflow_size = arenaSize / allocSize;

        // Won't alloc until we need to
        // allocPool();
    }

    ~MemPoolNoMutex()
    {
        for ( std::list<uint8_t*>::iterator i = arenas.begin(); i != arenas.end(); ++i ) {
            ::free(*i);
        }
    }

    /** Allocate a new element from the memory pool */
    inline void* malloc()
    {
        // We will service these out of the freelist first.  If
        // we don't have any in the freelist, then check the overflow.
        // If there is nothing there, we need to check with the shared
        // overflow to see if there is data there we can take.  If all
        // that fails, then alloc a new arena.

        numAlloc++;

        // Check freelist
        if ( !freelist.empty() ) {
            void* ret = freelist.back();
            freelist.pop_back();
            return ret;
        }

        // Check overflow.
        if ( !overflow.empty() ) {
            void* ret = overflow.back();
            overflow.pop_back();
            return ret;
        }

        // Need to check the shared_overflow
        shared_overflow.remove(elemSize, overflow);
        if ( !overflow.empty() ) {
            void* ret = overflow.back();
            overflow.pop_back();
            return ret;
        }

        // Need to allocate a new arena
        bool ok = allocPool();
        if ( !ok ) return nullptr;
        void* ret = freelist.back();
        freelist.pop_back();
        return ret;
    }

    /** Return an element to the memory pool */
    inline void free(void* ptr)
    {
        numFree++;
        // Goes in freelist if we aren't at max capacity.  Otherwise
        // goes in overflow.
        if ( freelist.size() >= max_freelist_size ) {
            overflow.push_back(ptr);
            if ( overflow.size() == max_overflow_size ) { shared_overflow.insert(elemSize, overflow); }
        }
        else {
            freelist.push_back(ptr);
        }
    }

    /**
       Approximates the current memory usage of the mempool. Some
       overheads are not taken into account.
     */
    uint64_t getBytesMemUsed()
    {
        uint64_t bytes_in_arenas    = arenas.size() * arenaSize;
        uint64_t bytes_in_free_list = freelist.capacity() * sizeof(void*);
        uint64_t bytes_in_overflow  = overflow.capacity() * sizeof(void*);
        return bytes_in_arenas + bytes_in_free_list + bytes_in_overflow;
    }

    // uint64_t getUndeletedEntries() { return numAlloc - numFree; }
    int64_t getNumAllocatedEntries() { return numAlloc; }
    int64_t getNumFreedEntries() { return numFree; }

    /** Counter:  Number of times elements have been allocated */
    int64_t numAlloc;
    /** Counter:  Number times elements have been freed */
    int64_t numFree;

    size_t getArenaSize() const { return arenaSize; }
    size_t getNumArenas() const { return arenas.size(); }
    size_t getElementSize() const { return elemSize; }
    size_t getAllocSize() const { return allocSize; }

    const std::list<uint8_t*>& getArenas() { return arenas; }

private:
    // allocPool will only ever be called by one thread, no need for locking
    // version that will cache align each memory chunk for an event
    bool allocPool()
    {
        uint8_t* newPool = (uint8_t*)mmap(nullptr, arenaSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if ( MAP_FAILED == newPool ) { return false; }
        std::memset(newPool, 0, arenaSize);
        arenas.push_back(newPool);
        size_t nelem = arenaSize / allocSize;
        for ( size_t i = 0; i < nelem; i++ ) {
            uint64_t* ptr = (uint64_t*)(newPool + (allocSize * i));
            freelist.push_back(ptr);
        }
        max_freelist_size += nelem;
        return true;
    }

    size_t elemSize;
    size_t arenaSize;
    size_t max_freelist_size;
    size_t max_overflow_size;
    size_t allocSize;

    std::list<uint8_t*> arenas;
};


OverflowFreeList MemPoolNoMutex::shared_overflow;


///////////////////////////////////////////////////////////////////////
// Classes to support MemPoolItem and MemPoolAccessor
///////////////////////////////////////////////////////////////////////

struct PoolInfo_t
{
    size_t          size;
    MemPoolNoMutex* pool;

    PoolInfo_t(size_t size, Core::MemPoolNoMutex* pool) : size(size), pool(pool) {}
};


// This is a vector where each thread has one entry.  Using a vector
// so that the memory will be cleaned up.  There won't be a chance to
// call delete[] if we use an array with new.
static std::vector<std::vector<PoolInfo_t>> memPoolThreadVector;

// My local thread number
thread_local int                      thread_num = -1;
thread_local std::vector<PoolInfo_t>* myPools;


inline MemPoolNoMutex*
getMemPool(std::size_t size) noexcept
{
    MemPoolNoMutex* pool = nullptr;

    for ( auto& x : *myPools ) {
        if ( x.size == size ) {
            pool = x.pool;
            break;
        }
    }

    if ( nullptr == pool ) {
        /* Still can't find it, alloc a new one */
        // pool = new Core::MemPoolNoMutex(size + sizeof(PoolData_t));
        pool = new Core::MemPoolNoMutex(size + sizeof(uint64_t*));
        myPools->emplace_back(size, pool);
    }
    return pool;
}


void
MemPoolAccessor::initializeGlobalData(int num_threads, bool cache_align)
{
    // Only resize once
    if ( memPoolThreadVector.size() == 0 ) { memPoolThreadVector.resize(num_threads); }
    memPoolCacheAlign = cache_align;
}

void
MemPoolAccessor::initializeLocalData(int thread)
{
    // Function should only be called once, but just in case, check to
    // see if thread_num has been set.
    if ( thread_num == -1 ) {
        thread_num = thread;
        myPools    = &memPoolThreadVector[thread_num];
    }
}


size_t
MemPoolAccessor::getArenaSize(size_t size)
{
    return getMemPool(size)->getArenaSize();
}


size_t
MemPoolAccessor::getNumArenas(size_t size)
{
    return getMemPool(size)->getNumArenas();
}


uint64_t
MemPoolAccessor::getBytesMemUsedBy(size_t size)
{
    return getMemPool(size)->getBytesMemUsed();
}


void
MemPoolAccessor::getMemPoolUsage(int64_t& bytes, int64_t& active_entries)
{
    bytes           = 0;
    active_entries  = 0;
    int64_t alloced = 0;
    int64_t freed   = 0;
    for ( auto&& pool_group : memPoolThreadVector ) {
        for ( auto&& entry : pool_group ) {
            bytes += entry.pool->getBytesMemUsed();
            alloced += entry.pool->getNumAllocatedEntries();
            freed += entry.pool->getNumFreedEntries();
        }
    }
    active_entries = alloced - freed;
}

void
MemPoolAccessor::printUndeletedMemPoolItems(const std::string& header, Output& out)
{
    for ( auto&& pool_group : memPoolThreadVector ) {
        for ( auto&& entry : pool_group ) {
            const std::list<uint8_t*>& arenas    = entry.pool->getArenas();
            size_t                     arenaSize = entry.pool->getArenaSize();
            size_t                     allocSize = entry.pool->getAllocSize();
            size_t                     nelem     = arenaSize / allocSize;
            for ( auto iter = arenas.begin(); iter != arenas.end(); ++iter ) {
                for ( size_t j = 0; j < nelem; j++ ) {
                    uint64_t* ptr = (uint64_t*)((*iter) + (allocSize * j));
                    if ( *ptr != 0 ) {
                        MemPoolItem* act = (MemPoolItem*)(ptr + 1);
                        act->print(header, out);
                    }
                }
            }
        }
    }
}


void*
MemPoolItem::operator new(std::size_t size) noexcept
{
    /* 1) Find memory pool
     * 2) Alloc item from pool
     * 3) Append PoolID to item, increment pointer
     */
    MemPoolNoMutex* pool = getMemPool(size);

    uint64_t* ptr = (uint64_t*)pool->malloc();
    if ( !ptr ) {
        fprintf(stderr, "Memory Pool failed to allocate a new object.  Error: %s\n", strerror(errno));
        return nullptr;
    }
    *ptr = size;
    return (void*)(ptr + 1);
}

/** Returns memory for this MemPoolItem to the appropriate memory pool */
void
MemPoolItem::operator delete(void* ptr)
{
    /* 1) Decrement pointer
     * 2) Determine Pool Pointer
     * 2b) Set Pool_id field to 0 to allow tracking
     * 3) Return to local pool
     */
    uint64_t* ptr8 = ((uint64_t*)ptr) - 1;
    uint64_t  size = *ptr8;
    if ( *ptr8 == 0 ) {
        // This item has already been deleted, error
        Output::getDefaultObject().fatal(
            CALL_INFO, 1, "ERROR: Double deletion of mempool item detected: %s",
            static_cast<MemPoolItem*>(ptr)->toString().c_str());
    }
    *ptr8 = 0;

    // find the pool
    MemPoolNoMutex* pool = getMemPool(size);
    pool->free(ptr8);
}


#else // #ifdef USE_MEMPOOLS


void
MemPoolAccessor::initializeGlobalData(int UNUSED(num_threads), bool UNUSED(cache_align))
{}

void
MemPoolAccessor::initializeLocalData(int UNUSED(thread))
{}


size_t
MemPoolAccessor::getArenaSize(size_t UNUSED(size))
{
    return 0;
}


size_t
MemPoolAccessor::getNumArenas(size_t UNUSED(size))
{
    return 0;
}


uint64_t
MemPoolAccessor::getBytesMemUsedBy(size_t UNUSED(size))
{
    return 0;
}


void
MemPoolAccessor::getMemPoolUsage(int64_t& bytes, int64_t& active_entries)
{
    bytes          = 0;
    active_entries = 0;
}

void
MemPoolAccessor::printUndeletedMemPoolItems(const std::string& UNUSED(header), Output& UNUSED(out))
{
    return;
}


void*
MemPoolItem::operator new(std::size_t size) noexcept
{
    return ::operator new(size);
}


/** Returns memory for this MemPoolItem to the appropriate memory pool */
void
MemPoolItem::operator delete(void* ptr)
{
    ::operator delete(ptr);
}


#endif // #ifdef USE_MEMPOOLS


std::string
MemPoolItem::toString() const
{
    std::stringstream buf;

    buf << "MemPoolItem of class: " << cls_name();
    return buf.str();
}


void
MemPoolItem::print(const std::string& header, Output& out) const
{
    out.output("%s%s\n", header.c_str(), toString().c_str());
}


} // namespace Core
} // namespace SST
