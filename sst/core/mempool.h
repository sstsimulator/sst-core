// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2013, Sandia Corporation
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

class MemPool
{
public:
	MemPool(size_t elementSize, size_t initialSize=(2<<20))
        : numAlloc(0), numFree(0),
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

	void free(void *ptr)
    {
        // TODO:  Make sure this is in one of our arenas
        freeList.push_back(ptr);
        ++numFree;
    }

    uint64_t numAlloc;
    uint64_t numFree;

private:

	bool allocPool()
    {
#if 0
        uint8_t *newPool = (uint8_t*)::malloc(arenaSize);
#else
        uint8_t *newPool = (uint8_t*)mmap(0, arenaSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        if ( MAP_FAILED == newPool ) {
            return false;
        }
#endif
        arenas.push_back(newPool);
        size_t nelem = arenaSize / elemSize;
        for ( size_t i = 0 ; i < nelem ; i++ ) {
            freeList.push_back(newPool + (elemSize*i));
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
