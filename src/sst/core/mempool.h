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

#ifndef SST_CORE_MEMPOOL_H
#define SST_CORE_MEMPOOL_H

#include <list>
#include <deque>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <cstdint>
#include <sys/mman.h>

#include "sst/core/threadsafe.h"

namespace SST {
namespace Core {

/**
 * Simple Memory Pool class
 */
class MemPool
{
    template <typename LOCK_t>
    class FreeList {
        LOCK_t mtx;
        std::vector<void*> list;
    public:
        inline void insert(void *ptr) {
            std::lock_guard<LOCK_t> lock(mtx);
            list.push_back(ptr);
        }

        inline void* try_remove() {
            std::lock_guard<LOCK_t> lock(mtx);
            if ( list.empty() ) return NULL;
            void *p = list.back();
            list.pop_back();
            return p;
        }

        size_t size() const { return list.size(); }
    };


public:
    /** Create a new Memory Pool.
     * @param elementSize - Size of each Element
     * @param initialSize - Size of the memory pool (in bytes)
     */
	MemPool(size_t elementSize, size_t initialSize=(2<<20)) :
        numAlloc(0), numFree(0),
        elemSize(elementSize), arenaSize(initialSize),
        allocating(false)
    {
        allocPool();
    }

	~MemPool()
    {
        for ( std::list<uint8_t*>::iterator i = arenas.begin() ; i != arenas.end() ; ++i ) {
            ::free(*i);
        }
    }

    /** Allocate a new element from the memory pool */
	inline void* malloc()
    {
        void *ret = freeList.try_remove();
        while ( !ret ) {
            bool ok = allocPool();
            if ( !ok ) return NULL;
#if ( defined( __amd64 ) || defined( __amd64__ ) || \
        defined( __x86_64 ) || defined( __x86_64__ ) )
            _mm_pause();
#elif defined(__PPC64__)
       	    asm volatile( "or 27, 27, 27" ::: "memory" );
#endif
            ret = freeList.try_remove();
        }
        ++numAlloc;
        return ret;
    }

    /** Return an element to the memory pool */
	inline void free(void *ptr)
    {
        // TODO:  Make sure this is in one of our arenas
        freeList.insert(ptr);
// #ifdef __SST_DEBUG_EVENT_TRACKING__
//         *((uint64_t*)ptr) = 0xFFFFFFFFFFFFFFFF;
// #endif
        ++numFree;
    }

    /**
       Approximates the current memory usage of the mempool. Some
       overheads are not taken into account.
     */
    uint64_t getBytesMemUsed() {
        uint64_t bytes_in_arenas = arenas.size() * arenaSize;
        uint64_t bytes_in_free_list = freeList.size() * sizeof(void*);
        return bytes_in_arenas + bytes_in_free_list;
    }

    uint64_t getUndeletedEntries() {
        return numAlloc - numFree;
    }
    
    /** Counter:  Number of times elements have been allocated */
    std::atomic<uint64_t> numAlloc;
    /** Counter:  Number times elements have been freed */
    std::atomic<uint64_t> numFree;

    size_t getArenaSize() const { return arenaSize; }
    size_t getElementSize() const { return elemSize; }

    const std::list<uint8_t*>& getArenas() { return arenas; }
    
private:

	bool allocPool()
    {
        /* If already in progress, return */
        if ( allocating.exchange(1, std::memory_order_acquire) ) {
            return true;
        }

        uint8_t *newPool = (uint8_t*)mmap(0, arenaSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
        if ( MAP_FAILED == newPool ) {
            allocating.store(0, std::memory_order_release);
            return false;
        }
        std::memset(newPool, 0xFF, arenaSize); 
        arenas.push_back(newPool);
        size_t nelem = arenaSize / elemSize;
        for ( size_t i = 0 ; i < nelem ; i++ ) {
            uint64_t* ptr = (uint64_t*)(newPool + (elemSize*i));
// #ifdef __SST_DEBUG_EVENT_TRACKING__
//             *ptr = 0xFFFFFFFFFFFFFFFF;
// #endif
            freeList.insert(ptr);
            // freeList.push_back(newPool + (elemSize*i));
        }
        allocating.store(0, std::memory_order_release);
        return true;
    }

	size_t elemSize;
	size_t arenaSize;

    std::atomic<unsigned int> allocating;
	FreeList<ThreadSafe::Spinlock> freeList;
	std::list<uint8_t*> arenas;

};

}
}

#endif
