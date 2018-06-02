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

#ifndef SST_CORE_INTERPROCESS_CIRCULARBUFFER_H
#define SST_CORE_INTERPROCESS_CIRCULARBUFFER_H

#include "sstmutex.h"

namespace SST {
namespace Core {
namespace Interprocess {

template <typename T>
class CircularBuffer {

public:
	CircularBuffer(size_t mSize = 0) {
		buffSize = mSize;
		readIndex = 0;
		writeIndex = 0;
	}

	void setBufferSize(const size_t bufferSize)
    	{
        	if ( buffSize != 0 ) {
	            fprintf(stderr, "Already specified size for buffer\n");
        	    exit(1);
        	}

	        buffSize = bufferSize;
		__sync_synchronize();
    	}

	T read() {
		int loop_counter = 0;

		while( true ) {
			bufferMutex.lock();

			if( readIndex != writeIndex ) {
				const T result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;

				bufferMutex.unlock();
				return result;
			}

			bufferMutex.unlock();
			bufferMutex.processorPause(loop_counter++);
		}
	}

	bool readNB(T* result) {
		if( bufferMutex.try_lock() ) {
			if( readIndex != writeIndex ) {
				*result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;

				bufferMutex.unlock();
				return true;
			} 

			bufferMutex.unlock();
		}

		return false;
	}

	void write(const T& v) {
		int loop_counter = 0;
	
		while( true ) {
			bufferMutex.lock();

			if( ((writeIndex + 1) % buffSize) != readIndex ) {
				buffer[writeIndex] = v;
				writeIndex = (writeIndex + 1) % buffSize;

				__sync_synchronize();
				bufferMutex.unlock();
				return;
			}

			bufferMutex.unlock();
			bufferMutex.processorPause(loop_counter++);
		}
	}

	~CircularBuffer() {

	}

	void clearBuffer() {
		bufferMutex.lock();
		readIndex = writeIndex;
		__sync_synchronize();
		bufferMutex.unlock();
	}

private:
	SSTMutex bufferMutex;
	size_t buffSize;
	size_t readIndex;
	size_t writeIndex;
	T buffer[0];

};

}
}
}

#endif
