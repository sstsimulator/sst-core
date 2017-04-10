
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
		while( true ) {
			bufferMutex.lock();

			if( readIndex != writeIndex ) {
				const T result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;

				bufferMutex.unlock();
				return result;
			}

			bufferMutex.unlock();
			bufferMutex.processorPause();
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
			bufferMutex.processorPause();
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
