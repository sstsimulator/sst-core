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

#ifndef SST_CORE_MEMPOOL_H
#define SST_CORE_MEMPOOL_H

#include <list>
#include <deque>

#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include <sys/mman.h>

namespace SST {
namespace Core {

/**
 * Simple Memory Pool class
 */
class MemPool
{
public:
    /** Create a new Memory Pool.
     * @param elementSize - Size of each Element
     * @param initialSize - Size of the memory pool (in bytes)
     */
	MemPool(size_t elementSize, size_t initialSize=(2<<20)) :
        numAlloc(0), numFree(0),
        elemSize(elementSize), arenaSize(initialSize)
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
	void* malloc()
    {
        if ( freeList.empty() ) {
            bool ok = allocPool();
            if ( !ok ) return NULL;
        }
        void *ret = freeList.back();
        freeList.pop_back();
        ++numAlloc;
        return ret;
    }

    /** Return an element to the memory pool */
	void free(void *ptr)
    {
        // TODO:  Make sure this is in one of our arenas
        freeList.push_back(ptr);
#ifdef __SST_DEBUG_EVENT_TRACKING__
        *((uint32_t*)ptr) = 0xFFFFFFFF;
        ++numFree;
#endif
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
    uint64_t numAlloc;
    /** Counter:  Number times elements have been freed */
    uint64_t numFree;

    size_t getArenaSize() const { return arenaSize; }
    size_t getElementSize() const { return elemSize; }

    const std::list<uint8_t*>& getArenas() { return arenas; }
    
private:

	bool allocPool()
    {
#if 0
        uint8_t *newPool = (uint8_t*)::malloc(arenaSize);
#else
        uint8_t *newPool = (uint8_t*)mmap(0, arenaSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
        if ( MAP_FAILED == newPool ) {
            return false;
        }
#endif
        arenas.push_back(newPool);
        size_t nelem = arenaSize / elemSize;
        for ( size_t i = 0 ; i < nelem ; i++ ) {
            uint32_t* ptr = (uint32_t*)(newPool + (elemSize*i));
#ifdef __SST_DEBUG_EVENT_TRACKING__
            *ptr = 0xFFFFFFFF;
#endif
            freeList.push_back(ptr);
            // freeList.push_back(newPool + (elemSize*i));
        }
        return true;
    }

	size_t elemSize;
	size_t arenaSize;

	std::deque<void*> freeList;
	std::list<uint8_t*> arenas;

};

}
}

#endif
