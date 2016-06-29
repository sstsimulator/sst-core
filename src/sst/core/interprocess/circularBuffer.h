// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_INTERPROCESS_CIRCULARBUFFER_H
#define SST_CORE_INTERPROCESS_CIRCULARBUFFER_H 1

#include <cstddef>
#include <pthread.h>

#include <mutex>
#include <condition_variable>

namespace SST {
namespace Core {
namespace Interprocess {

/**
 * Multi-process safe, Circular Buffer class
 *
 * @tparam T  Type of data item to store in the buffer
 * @tparam A  Memory Allocator type to use
 */
template <typename T>
class CircularBuffer {

    pthread_mutex_t mtx;
    pthread_cond_t cond_full, cond_empty;
    pthread_condattr_t attrcond;
    pthread_mutexattr_t attrmutex;

    size_t rPtr, wPtr;
    size_t buffSize;
    T buffer[0]; // Actual size: buffSize


public:
    /**
     * Construct a new circular buffer
     * @param bufferSize Number of elements in the buffer
     * @param allocator Memory allocator to use for constructing the buffer
     */
    CircularBuffer(size_t bufferSize = 0) :
        rPtr(0), wPtr(0), buffSize(bufferSize)
    {
        pthread_mutexattr_init(&attrmutex);
        pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mtx, &attrmutex);

        pthread_condattr_init(&attrcond);
        pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&cond_full, &attrcond);
        pthread_cond_init(&cond_empty, &attrcond);
    }

    ~CircularBuffer() {
        pthread_mutex_destroy(&mtx);
        pthread_mutexattr_destroy(&attrmutex);

        pthread_cond_destroy(&cond_full);
        pthread_cond_destroy(&cond_empty);
        pthread_condattr_destroy(&attrcond);
    }

    void setBufferSize(size_t bufferSize)
    {
        if ( buffSize != 0 ) {
            fprintf(stderr, "Already specified size for buffer\n");
            exit(1);
        }
        buffSize = bufferSize;
    }

    /**
     * Write a value to the circular buffer
     * @param value New Value to write
     */
    void write(const T &value)
    {
        pthread_mutex_lock(&mtx);
		while ( (wPtr+1) % buffSize == rPtr ) pthread_cond_wait(&cond_full, &mtx);

        buffer[wPtr] = value;
        wPtr = (wPtr +1 ) % buffSize;

        pthread_cond_signal(&cond_empty);
        pthread_mutex_unlock(&mtx);
    }

    /**
     * Blocking Read a value from the circular buffer
     * @return The next item in the queue to be read
     */
    T read(void)
    {
        pthread_mutex_lock(&mtx);
		while ( rPtr == wPtr ) pthread_cond_wait(&cond_empty, &mtx);

        T ans = buffer[rPtr];
        rPtr = (rPtr +1 ) % buffSize;

		pthread_cond_signal(&cond_full);

        pthread_mutex_unlock(&mtx);
        return ans;
    }

    /**
     * Non-Blocking Read a value from the circular buffer
     * @param result Pointer to an item which will be filled in if possible
     * @return True if an item was available, False otherwisw
     */
    bool readNB(T *result)
    {
        int lock = pthread_mutex_trylock(&mtx);
        if ( !lock ) return false;
		if ( rPtr == wPtr ) {
            pthread_mutex_unlock(&mtx);
            return false;
        }

        *result = buffer[rPtr];
        rPtr = (rPtr +1 ) % buffSize;

		pthread_cond_signal(&cond_full);

        pthread_mutex_unlock(&mtx);
        return true;
    }

};

}
}
}



#endif
