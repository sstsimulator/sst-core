
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
		buffer = new T[mSize];

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
		buffer = new T[buffSize];
    	}

	T read() {
		T result;
		bool readDone = false;

		while( (!readDone) ) {
			bufferMutex.lock();

			if( readIndex != writeIndex ) {
				result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;
				readDone = true;
			}

			bufferMutex.unlock();
		}

		return result;
	}

	bool readNB(T* result) {
		if( bufferMutex.try_lock() ) {
			if( readIndex != writeIndex ) {
				*result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;

				bufferMutex.unlock();
				return true;
			} else {
				bufferMutex.unlock();
			}
		}

		return false;
	}

	void write(const T& v) {
		bool writeDone = false;

		while( (!writeDone) ) {
			bufferMutex.lock();

			if( ((writeIndex + 1) % buffSize) != readIndex ) {
				buffer[writeIndex] = v;
				writeIndex = (writeIndex + 1) % buffSize;

				writeDone = true;
				__sync_synchronize();
			}

			bufferMutex.unlock();
		}
	}

	~CircularBuffer() {
		delete[] buffer;
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
	T* buffer;
	size_t readIndex;
	size_t writeIndex;

};

}
}
}

#endif
