

#ifndef _H_SST_CORE_INTERPROCESS_MUTEX
#define _H_SST_CORE_INTERPROCESS_MUTEX

#include <immintrin.h>
#include <sched.h>

namespace SST {
namespace Core {
namespace Interprocess {

#define SST_CORE_INTERPROCESS_LOCKED 1
#define SST_CORE_INTERPROCESS_UNLOCKED 0

class SSTMutex {

public:
	SSTMutex() {
		lockVal = SST_CORE_INTERPROCESS_UNLOCKED;
	}

	void processorPause() {
#if defined(__x86_64__)
#pragma message "Compiling into NOP and PAUSE.."
			_mm_pause();
#else
			// Put some pause code in here
#endif
	}

	void lock() {
		int loop_counter = 0;

		while( ! __sync_bool_compare_and_swap( &lockVal, SST_CORE_INTERPROCESS_UNLOCKED, SST_CORE_INTERPROCESS_LOCKED) ) {
			if(loop_counter < 64) {
				processorPause();
			} else {
				sched_yield();
			}

			loop_counter++;
		}
	}

	void unlock() {
		lockVal = SST_CORE_INTERPROCESS_UNLOCKED;
		__sync_synchronize();
	}

	bool try_lock() {
		return __sync_bool_compare_and_swap( &lockVal, SST_CORE_INTERPROCESS_UNLOCKED, SST_CORE_INTERPROCESS_LOCKED );
	}

private:
	volatile int lockVal __attribute__((aligned(64)));

};

}
}
}

#endif
