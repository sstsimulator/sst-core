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

#ifndef _H_SST_CORE_INTERPROCESS_MUTEX
#define _H_SST_CORE_INTERPROCESS_MUTEX

#include <sched.h>
#include <time.h>

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

	void processorPause(int currentCount) {
		if( currentCount < 64 ) {
#if defined(__x86_64__)
			asm volatile ("pause" : : : "memory");
#else
			// Put some pause code in here
#endif
		} else if( currentCount < 256 ) {
			sched_yield();
		} else {
			struct timespec sleepPeriod;
			sleepPeriod.tv_sec = 0;
			sleepPeriod.tv_nsec = 100;

			struct timespec interPeriod;
			nanosleep(&sleepPeriod, &interPeriod);
		}
	}

	void lock() {
		int loop_counter = 0;

		while( ! __sync_bool_compare_and_swap( &lockVal, SST_CORE_INTERPROCESS_UNLOCKED, SST_CORE_INTERPROCESS_LOCKED) ) {
			processorPause(loop_counter);
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
	volatile int lockVal;

};

}
}
}

#endif
