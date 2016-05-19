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

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/containers/vector.hpp>


namespace SST {
namespace Core {
namespace Interprocess {

/**
 * Multi-process safe, Circular Buffer class
 *
 * @tparam T  Type of data item to store in the buffer
 * @tparam A  Memory Allocator type to use
 */
template <typename T, typename A>
class CircularBuffer {
    typedef boost::interprocess::vector<T, A> RawBuffer_t;

    boost::interprocess::interprocess_mutex mutex;
    boost::interprocess::interprocess_condition cond_full, cond_empty;

    size_t rPtr, wPtr;
    RawBuffer_t buffer;
    size_t buffSize;


public:
    /**
     * Construct a new circular buffer
     * @param bufferSize Number of elements in the buffer
     * @param allocator Memory allocator to use for constructing the buffer
     */
    CircularBuffer(size_t bufferSize, const A & allocator) :
        rPtr(0), wPtr(0), buffer(allocator), buffSize(bufferSize)
    {
        buffer.resize(buffSize);
    }

    /**
     * Write a value to the circular buffer
     * @param value New Value to write
     */
    void write(const T &value)
    {
		boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(mutex);
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
		boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(mutex);
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
		boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(mutex, boost::interprocess::try_to_lock);
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
