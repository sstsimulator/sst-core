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

#ifndef SST_CORE_RNG_MARSAGLIA_H
#define SST_CORE_RNG_MARSAGLIA_H
#include <sst/core/sst_types.h>

#include <stdint.h>
#include <sys/time.h>

#include "sstrng.h"

#define MARSAGLIA_UINT32_MAX 4294967295U
#define MARSAGLIA_UINT64_MAX 18446744073709551615ULL
#define MARSAGLIA_INT32_MAX  2147483647L
#define MARSAGLIA_INT64_MAX  9223372036854775807LL

namespace SST {
namespace RNG {
/**
	\class MarsagliaRNG marsaglia.h "sst/core/rng/marsaglia.h"

	Implements a random number generator using the Marsaglia method. This method is
	computationally cheap and provides a reasonable distribution of random numbers. If
	you need additional strength in the random numbers you may want to consider the
	Mersenne RNG.

        For more information see the Multiply-with-carry Random Number Generator article
	at Wikipedia (http://en.wikipedia.org/wiki/Multiply-with-carry).
*/
class MarsagliaRNG : public SSTRandom {

    public:
	/**
		Creates a new Marsaglia RNG using the initial seeds.
		@param[in] initial_z One of the random seed pairs
		@param[in] initial_w One of the random seed pairs.
	*/
        MarsagliaRNG(unsigned int initial_z,
                unsigned int initial_w);

	/**
		Creates a new Marsaglia RNG using random initial seeds (which are
		read from the system clock). Note that these will create variation
		between runs and between platforms.
	*/
        MarsagliaRNG();

	/**
		Restart the random number generator with new seeds
		@param[in] new_z A new Z-seed
		@param[in] new_w A new W-seed
	*/
	void	restart(unsigned int new_z, unsigned int new_w);

	/**
		Generates the next random number as a double in the range 0 to 1.
	*/
	double   nextUniform() override;

	/**
		Generates the next random number as an unsigned 32-bit integer.
	*/
	uint32_t generateNextUInt32() override;

	/**
		Generates the next random number as an unsigned 64-bit integer.
	*/
	uint64_t generateNextUInt64() override;

	/**
		Generates the next number as a signed 64-bit integer.
	*/
	int64_t  generateNextInt64() override;

	/**
		Generates the next number as a signed 32-bit integer.
	*/
    int32_t   generateNextInt32() override;
    
    /**
		Seed the XOR RNG
	*/
	void seed(uint64_t newSeed);

    private:
	/**
		Generates the next random number
	*/
        unsigned int generateNext();

	/**
		The Z seed of the Marsaglia generator
	*/
        unsigned int m_z;

	/**
		The W seed of the Marsaglia generator
	*/
        unsigned int m_w;

};

} //namespace RNG
} //namespace SST

#endif //SST_CORE_RNG_MARSAGLIA_H
