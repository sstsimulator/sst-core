

#ifndef _H_SST_CORE_INTERPROCESS_MUTEX
#define _H_SST_CORE_INTERPROCESS_MUTEX

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
			asm volatile ( "nop\n" : : : "memory" );
			asm volatile ( "nop\n" : : : "memory" );
			asm volatile ( "nop\n" : : : "memory" );
			asm volatile ( "nop\n" : : : "memory" );
			asm volatile ( "pause\n" : : : "memory" );
#else
			// Put some pause code in here
#endif
	}

	void lock() {
		while( ! __sync_bool_compare_and_swap( &lockVal, SST_CORE_INTERPROCESS_UNLOCKED, SST_CORE_INTERPROCESS_LOCKED) ) {
			processorPause();
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
