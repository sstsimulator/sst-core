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

#ifndef SST_CORE_CORE_THREADSAFE_H
#define SST_CORE_CORE_THREADSAFE_H

#if ( defined( __amd64 ) || defined( __amd64__ ) || \
        defined( __x86_64 ) || defined( __x86_64__ ) )
#include <x86intrin.h>
#endif

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <vector>
//#include <stdalign.h>

#include <time.h>

#include <sst/core/profile.h>

namespace SST {
namespace Core {
namespace ThreadSafe {

#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ < 8 ))
#    define CACHE_ALIGNED(type, name) type name __attribute__((aligned(64)))
#    define CACHE_ALIGNED_T
#else
#    define CACHE_ALIGNED(type, name) alignas(64) type name
#    define CACHE_ALIGNED_T alignas(64)
#endif


class CACHE_ALIGNED_T Barrier {
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

            size_t gen = generation.load(std::memory_order_acquire);
            asm("":::"memory");
            size_t c = count.fetch_sub(1) -1;
            if ( 0 == c ) {
                /* We should release */
                count.store(origCount);
                asm("":::"memory");
                /* Incrementing generation causes release */
                generation.fetch_add(1, std::memory_order_release);
                __sync_synchronize();
            } else {
                /* Try spinning first */
                uint32_t count = 0;
                do {
                    count++;
                    if ( count < 1024 ) {
#if ( defined( __amd64 ) || defined( __amd64__ ) || \
        defined( __x86_64 ) || defined( __x86_64__ ) )
                        _mm_pause();
#elif defined(__PPC64__)
       	asm volatile( "or 27, 27, 27" ::: "memory" );
#endif
		    } else if ( count < (1024*1024) ) {
                        std::this_thread::yield();
                    } else {
                        struct timespec ts;
                        ts.tv_sec = 0;
                        ts.tv_nsec = 1000;
                        nanosleep(&ts, NULL);
                    }
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


#if 0
typedef std::mutex Spinlock;
#else
class Spinlock {
    std::atomic_flag latch = ATOMIC_FLAG_INIT;
public:
    Spinlock() { }

    inline void lock() {
        while ( latch.test_and_set(std::memory_order_acquire) ) {
#if ( defined( __amd64 ) || defined( __amd64__ ) || \
        defined( __x86_64 ) || defined( __x86_64__ ) )
                _mm_pause();
#elif defined(__PPC64__)
                asm volatile( "or 27, 27, 27" ::: "memory" );
                __sync_synchronize();
#endif
        }
    }

    inline void unlock() {
        latch.clear(std::memory_order_release);
    }
};
#endif



template<typename T>
class BoundedQueue {

    struct cell_t {
        std::atomic<size_t> sequence;
        T data;
    };

    bool initialized;
    size_t dsize;
    cell_t *data;
    CACHE_ALIGNED(std::atomic<size_t>, rPtr);
    CACHE_ALIGNED(std::atomic<size_t>, wPtr);

public:
    // BoundedQueue(size_t maxSize) : dsize(maxSize)
    BoundedQueue(size_t maxSize) : initialized(false)
    {
        initialize(maxSize);
    }

    BoundedQueue() : initialized(false) {}
    
    void initialize(size_t maxSize) {
        if ( initialized ) return;
        dsize = maxSize;
        data = new cell_t[dsize];
        for ( size_t i = 0 ; i < maxSize ; i++ )
            data[i].sequence.store(i);
        rPtr.store(0);
        wPtr.store(0);
        //fprintf(stderr, "%p  %p:  %ld\n", &rPtr, &wPtr, ((intptr_t)&wPtr - (intptr_t)&rPtr));
        initialized = true;
    }

    ~BoundedQueue()
    {
        if ( initialized ) delete [] data;
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
                // fprintf(stderr, "diff = %ld: %zu - %zu\n", diff, seq, pos);
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

    T remove() {
        while(1) {
            T res;
            if ( try_remove(res) ) {
                return res;
            }
#if ( defined( __amd64 ) || defined( __amd64__ ) || \
        defined( __x86_64 ) || defined( __x86_64__ ) )
            _mm_pause();
#elif defined(__PPC64__)
       	asm volatile( "or 27, 27, 27" ::: "memory" );
#endif
        }
    }
};

template<typename T>
class UnboundedQueue {
    struct CACHE_ALIGNED_T Node {
        std::atomic<Node*> next;
        T data;

        Node() : next(nullptr) { }
    };

    CACHE_ALIGNED(Node*, first);
    CACHE_ALIGNED(Node*, last);
    CACHE_ALIGNED(Spinlock, consumerLock);
    CACHE_ALIGNED(Spinlock, producerLock);

public:
    UnboundedQueue() {
        /* 'first' is a dummy value */
        first = last = new Node();
    }

    ~UnboundedQueue() {
        while( first != nullptr ) {      // release the list
            Node* tmp = first;
            first = tmp->next;
            delete tmp;
        }
    }

    void insert(const T& t) {
        Node* tmp = new Node();
        tmp->data = t;
        std::lock_guard<Spinlock> lock(producerLock);
        last->next = tmp;         // publish to consumers
        last = tmp;               // swing last forward
    }

    bool try_remove( T& result) {
        std::lock_guard<Spinlock> lock(consumerLock);
        Node* theFirst = first;
        Node* theNext = first->next;
        if( theNext != nullptr ) {     // if queue is nonempty
            result = theNext->data;    // take it out
            first = theNext;           // swing first forward
            delete theFirst;           // delete the old dummy
            return true;
        }
        return false;
    }

    T remove() {
        while(1) {
            T res;
            if ( try_remove(res) ) {
                return res;
            }
#if ( defined( __amd64 ) || defined( __amd64__ ) || \
        defined( __x86_64 ) || defined( __x86_64__ ) )
            _mm_pause();
#elif defined(__PPC64__)
       	asm volatile( "or 27, 27, 27" ::: "memory" );
#endif
        }
    }

};

}
}
}

#endif
