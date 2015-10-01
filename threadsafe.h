// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_THREADSAFE_H
#define SST_CORE_CORE_THREADSAFE_H

#include <x86intrin.h>

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <vector>
//#include <stdalign.h>

#include <sched.h>

#include <sst/core/profile.h>

namespace SST {
namespace Core {
namespace ThreadSafe {

#if defined(__clang__)
#  define CACHE_ALIGNED(type, name) alignas(64) type name
#elif defined(__GNUC__)
#  if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8 ))
#    define CACHE_ALIGNED(type, name) alignas(64) type name
#  else
#    define CACHE_ALIGNED(type, name) type name __attribute__((aligned(64)))
#  endif
#endif

class Barrier {
    std::mutex mtx;
    std::condition_variable cv;
    size_t origCount;
    std::atomic<bool> enabled;
    std::atomic<size_t> count, generation;
public:
    Barrier(size_t count) : origCount(count), enabled(true),
            count(count), generation(0)
    { }

    // Come g++ 4.7, this can become a delegating constructor
    Barrier() : origCount(0), enabled(false), count(0), generation(0)
    { }


    /** ONLY call this while nobody is in wait() */
    void resize(size_t newCount)
    {
        count = origCount = newCount;
        generation.store(0);
        enabled.store(true);
    }


    /**
     * Wait for all threads to reach this point.
     * @return 0.0, or elapsed time spent waiting, if configured with --enable-profile
     */
    double wait()
    {
        double elapsed = 0.0;
        if ( enabled ) {
            auto startTime = SST::Core::Profile::now();

            size_t gen = generation.load();
            size_t c = --count;
            if ( 0 == c ) {
                /* We should release */
                count = origCount;
                ++generation; /* Incrementing generation causes release */
            } else {
                /* Try spinning first */
                do {
                    _mm_pause();
                } while ( gen == generation.load(std::memory_order_acquire) );
            }
            elapsed = SST::Core::Profile::getElapsed(startTime);
        }
        return elapsed;
    }

    void disable()
    {
        enabled.store(false);
        count.store(0);
        ++generation;
    }
};


class Spinlock {
    std::atomic<int> latch;
public:
    Spinlock() : latch(0)
    { }

    inline void lock() {
        //while ( latch.exchange(1, std::memory_order_acquire) ) {
        int zero = 0;
        while ( !latch.compare_exchange_weak(zero, 1,
                    std::memory_order_acquire,
                    std::memory_order_relaxed) && zero) {
            do {
                zero = 0;
                _mm_pause();
            } while ( latch.load(std::memory_order_acquire) );
        }
    }

    inline void unlock() {
        /* TODO:  Understand why latch.store(0) breaks */
        --latch;
    }
};



template<typename T>
class BoundedQueue {

    struct cell_t {
        std::atomic<size_t> sequence;
        T data;
    };

    const size_t dsize;
    cell_t *data;
    CACHE_ALIGNED(std::atomic<size_t>, rPtr);
    CACHE_ALIGNED(std::atomic<size_t>, wPtr);

public:
    BoundedQueue(size_t maxSize) : dsize(maxSize)
    {
        data = new cell_t[dsize];
        for ( size_t i = 0 ; i < maxSize ; i++ )
            data[i].sequence.store(i);
        rPtr.store(0);
        wPtr.store(0);
        fprintf(stderr, "%p  %p:  %ld\n", &rPtr, &wPtr, ((intptr_t)&wPtr - (intptr_t)&rPtr));
    }

    ~BoundedQueue()
    {
        delete [] data;
    }

    size_t size() const
    {
        return (wPtr.load() - rPtr.load());
    }

    bool empty() const
    {
        return (rPtr.load() == wPtr.load());
    }

    bool try_insert(const T& arg)
    {
        cell_t *cell = NULL;
        size_t pos = wPtr.load(std::memory_order_relaxed);
        for (;;) {
            cell = &data[pos % dsize];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;
            if ( 0 == diff ) {
                if ( wPtr.compare_exchange_weak(pos, pos+1, std::memory_order_relaxed) )
                    break;
            } else if ( 0 > diff ) {
                fprintf(stderr, "diff = %ld: %zu - %zu\n", diff, seq, pos);
                return false;
            } else {
                pos = wPtr.load(std::memory_order_relaxed);
            }
        }
        cell->data = arg;
        cell->sequence.store(pos+1, std::memory_order_release);
        return true;
    }

    bool try_remove(T &res)
    {
        cell_t *cell = NULL;
        size_t pos = rPtr.load(std::memory_order_relaxed);
        for (;;) {
            cell = &data[pos % dsize];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);
            if ( 0 == diff ) {
                if ( rPtr.compare_exchange_weak(pos, pos+1, std::memory_order_relaxed) )
                    break;
            } else if ( 0 > diff ) {
                return false;
            } else {
                pos = rPtr.load(std::memory_order_relaxed);
            }
        }
        res = cell->data;
        cell->sequence.store(pos + dsize, std::memory_order_release);
        return true;
    }

};

template<typename T>
class UnboundedQueue {
    Spinlock latch;
    std::vector<T> data;
    size_t dsize;
    size_t page;
    size_t rPtr;
    size_t wPtr;

    void resize(size_t nobj) {
        size_t oldSize = dsize;
        if ( oldSize < nobj ) {
            size_t newSize = nobj;
            size_t remain = nobj % page;
            if ( remain ) {
                newSize += (page - remain);
            }
            data.resize( newSize );
            if ( oldSize ) {
                rPtr = rPtr % oldSize;
                wPtr = wPtr % oldSize;
            }
            dsize = newSize;
        }
    }

public:
    UnboundedQueue() :
        dsize(0), page(0), rPtr(0), wPtr(0)
    {
        page = 4096 / sizeof(T);
        resize(page);
    }

    size_t size() const
    {
        return (wPtr - rPtr);
    }

    bool empty() const
    {
        return (rPtr == wPtr);
    }

    void insert(T arg)
    {
        latch.lock();
        size_t t = wPtr++;
        size_t space = 1 + (t - rPtr);
        t %= dsize;
        if ( space >= dsize ) {
            resize(space);
        }
        data[t] = arg;
        latch.unlock();
    }

    T remove()
    {
        latch.lock();
        size_t t = rPtr++;
        size_t p = t % dsize;
        latch.unlock();
        while ( t >= wPtr ) _mm_pause();
        return data[p];
    }

};


}
}
}

#endif
