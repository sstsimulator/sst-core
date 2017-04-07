

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

	void lock() {
		while( __sync_val_compare_and_swap( &lockVal, SST_CORE_INTERPROCESS_UNLOCKED, SST_CORE_INTERPROCESS_LOCKED) != SST_CORE_INTERPROCESS_UNLOCKED ) {
#if defined(__x86_64__)
			asm volatile ( "nop\n" : : : "memory" );
			asm volatile ( "nop\n" : : : "memory" );
			asm volatile ( "nop\n" : : : "memory" );
			asm volatile ( "nop\n" : : : "memory" );
#else
			// Put some pause code in here
#endif
		}
	}

	void unlock() {
		lockVal = SST_CORE_INTERPROCESS_UNLOCKED;
		__sync_synchronize();
	}

	bool try_lock() {
		__sync_val_compare_and_swap( &lockVal, SST_CORE_INTERPROCESS_UNLOCKED, SST_CORE_INTERPROCESS_LOCKED) != SST_CORE_INTERPROCESS_UNLOCKED;
	}

private:
	volatile int lockVal __attribute__((aligned(64)));

};

}
}
}

#endif
