
#ifndef SST_CORE_INTERPROCESS_CIRCULARBUFFER_H
#define SST_CORE_INTERPROCESS_CIRCULARBUFFER_H

#include <mutex>
#include <condition_variable>

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
		std::unique_lock<std::mutex> lock(bufferMutex);
		bufferCondVar.wait(lock, [this]{ return (readIndex != writeIndex); });

		const T result = buffer[readIndex];
		readIndex = (readIndex + 1) % buffSize;

		__sync_synchronize();
		bufferCondVar.notify_all();

		return result;
	}

	bool readNB(T* result) {
		if( bufferMutex.try_lock() ) {
			if( writeIndex == readIndex ) {
				bufferMutex.unlock();
				return false;
			} else {
				*result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;

				__sync_synchronize();
				bufferCondVar.notify_all();
				bufferMutex.unlock();

				return true;
			}
		} else {
			return false;
		}
	}

	void write(const T& v) {
		std::unique_lock<std::mutex> lock(bufferMutex);

		bufferCondVar.wait(lock, [this]{ return ((writeIndex + 1) % buffSize) != readIndex; });

		buffer[writeIndex] = v;
		writeIndex = (writeIndex + 1) % buffSize;

		__sync_synchronize();
		bufferCondVar.notify_all();
	}

	~CircularBuffer() {
		delete[] buffer;
	}

	void clearBuffer() {
		std::unique_lock<std::mutex> lock(bufferMutex);
		readIndex = writeIndex;
		__sync_synchronize();
	}

private:
	std::mutex bufferMutex;
	std::condition_variable bufferCondVar;
	size_t buffSize;
	T* buffer;
	size_t readIndex;
	size_t writeIndex;

};

}
}
}

#endif
