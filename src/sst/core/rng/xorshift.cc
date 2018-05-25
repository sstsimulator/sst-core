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

#include <sst_config.h>

#include "xorshift.h"
#include <cstdlib>
#include <cassert>

using namespace SST;
using namespace SST::RNG;
/*
	Generate a new random number generator with a random selection for the
	seed.
*/
XORShiftRNG::XORShiftRNG() {
	struct timeval now;
	gettimeofday(&now, NULL);

	x = (uint32_t) now.tv_usec;
	y = (uint32_t) now.tv_sec;
	w = 0;
	z = 11;
}

/*
	Seed the Mersenne and then make a group of numbers
*/
XORShiftRNG::XORShiftRNG(unsigned int startSeed) {
	assert(startSeed != 0);

	seed(startSeed);
}

/*
	Transform an unsigned integer into a uniform double from which other
	distributed can be generated
*/
double XORShiftRNG::nextUniform() {
	uint32_t temp = generateNextUInt32();
	return ( (double) temp ) / (double) XORSHIFT_UINT32_MAX;
}

uint32_t XORShiftRNG::generateNextUInt32() {
	uint32_t t = x ^ (x << 11);
    	x = y; y = z; z = w;
    	return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

uint64_t XORShiftRNG::generateNextUInt64() {
	uint64_t returnUInt64 = 0;
	int64_t nextInt64 = generateNextInt64();

	char* returnUInt64Ptr = (char*) &returnUInt64;
	const char* nextInt64Ptr = (const char*) &nextInt64;

	for(size_t i = 0; i < sizeof(nextInt64); i++) {
		returnUInt64Ptr[i] = nextInt64Ptr[i];
	}

	return returnUInt64;
}

int64_t  XORShiftRNG::generateNextInt64() {
	int64_t returnInt64 = 0;
	uint32_t lowerHalf = generateNextUInt32();
	uint32_t upperHalf = generateNextUInt32();

	char* returnInt64Ptr = (char*) &returnInt64;
	const char* lowerHalfPtr = (const char*) &lowerHalf;
	const char* upperHalfPtr = (const char*) &upperHalf;

	for(size_t i = 0; i < sizeof(lowerHalf); i++) {
		returnInt64Ptr[i] = lowerHalfPtr[i];
	}

	for(size_t i = 0; i < sizeof(lowerHalf); i++) {
		returnInt64Ptr[i+4] = upperHalfPtr[i];
	}

	return returnInt64;
}

int32_t  XORShiftRNG::generateNextInt32() {
	int32_t returnInt32 = 0;
	uint32_t nextUInt32 = generateNextUInt32();

	char* returnInt32Ptr = (char*) &returnInt32;
	const char* nextUInt32Ptr = (const char*) &nextUInt32;

	for(size_t i = 0; i < sizeof(returnInt32); i++) {
		returnInt32Ptr[i] = nextUInt32Ptr[i];
	}

	return returnInt32;
}

void XORShiftRNG::seed(uint64_t seed) {
	x = (uint32_t) seed;
	y = 0;
	w = 0;
	z = 0;
}

XORShiftRNG::~XORShiftRNG() {

}
