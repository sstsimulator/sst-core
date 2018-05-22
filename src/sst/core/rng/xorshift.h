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

#ifndef SST_CORE_RNG_XORSHIFT_H
#define SST_CORE_RNG_XORSHIFT_H

#include <stdint.h>
#include <sys/time.h>

#include "sstrng.h"

#define XORSHIFT_UINT32_MAX 4294967295U
#define XORSHIFT_UINT64_MAX 18446744073709551615ULL
#define XORSHIFT_INT32_MAX  2147483647L
#define XORSHIFT_INT64_MAX  9223372036854775807LL

namespace SST {
namespace RNG {
/**
	\class XORShiftRNG xorshift.h "sst/core/rng/xorshift.h"

	Implements a lightweight RNG based on XOR-shift operations. We utilize the
	XORSHIFT algorithm from: http://en.wikipedia.org/wiki/Xorshift. This is a very
	lightweight and inexpensive RNG.

*/
class XORShiftRNG : public SSTRandom {

    public:
	/**
		Create a new Mersenne RNG with a specified seed
		@param[in] seed The seed for this RNG
	*/
        XORShiftRNG(unsigned int seed);

	/**
		Creates a new Mersenne using a random seed which is obtained from the system
		clock. Note this will give different results on different platforms and between
		runs.
	*/
        XORShiftRNG();

	/**
		Generates the next random number as a double value between 0 and 1.
	*/
	double   nextUniform() override;

	/**
		Generates the next random number as an unsigned 32-bit integer
	*/
	uint32_t generateNextUInt32() override;

	/**
		Generates the next random number as an unsigned 64-bit integer
	*/
	uint64_t generateNextUInt64() override;

	/**
		Generates the next random number as a signed 64-bit integer
	*/
	int64_t  generateNextInt64() override;

	/**
		Generates the next random number as a signed 32-bit integer
	*/
    int32_t  generateNextInt32() override;

	/**
		Seed the XOR RNG
	*/
	void seed(uint64_t newSeed);

	/**
		Destructor for Mersenne
	*/
	~XORShiftRNG();

protected:
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t w;

};

} //namespace RNG
} //namespace SST

#endif //SST_CORE_RNG_XORSHIFT_H
