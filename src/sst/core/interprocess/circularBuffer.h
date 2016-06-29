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
#include <sst/core/output.h>

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

    std::mutex mtx;
    std::condition_variable cond_full, cond_empty;

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
    }

    void setBufferSize(size_t bufferSize)
    {
        if ( buffSize != 0 )
            Output::getDefaultObject().fatal(CALL_INFO, -1, "Already specified size for buffer\n");
        buffSize = bufferSize;
    }

    /**
     * Write a value to the circular buffer
     * @param value New Value to write
     */
    void write(const T &value)
    {
        std::unique_lock<std::mutex> lock(mtx);
		while ( (wPtr+1) % buffSize == rPtr ) cond_full.wait(lock);

        buffer[wPtr] = value;
        wPtr = (wPtr +1 ) % buffSize;

		cond_empty.notify_one();
    }

    /**
     * Blocking Read a value from the circular buffer
     * @return The next item in the queue to be read
     */
    T read(void)
    {
        std::unique_lock<std::mutex> lock(mtx);
		while ( rPtr == wPtr ) cond_empty.wait(lock);

        T ans = buffer[rPtr];
        rPtr = (rPtr +1 ) % buffSize;

		cond_full.notify_one();

        return ans;
    }

    /**
     * Non-Blocking Read a value from the circular buffer
     * @param result Pointer to an item which will be filled in if possible
     * @return True if an item was available, False otherwisw
     */
    bool readNB(T *result)
    {
        std::unique_lock<std::mutex> lock(mtx);
        if ( !lock ) return false;
		if ( rPtr == wPtr ) return false;

        *result = buffer[rPtr];
        rPtr = (rPtr +1 ) % buffSize;

		cond_full.notify_one();

        return true;
    }

};

}
}
}



#endif
