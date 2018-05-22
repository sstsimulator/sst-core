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

#include "marsaglia.h"
#include <cstdlib>
#include <cstring>

using namespace SST;
using namespace SST::RNG;

/*
	Seed the Marsaglia method with two initializers that must be non-zero.
*/
MarsagliaRNG::MarsagliaRNG(unsigned int initial_z, unsigned int initial_w) {
	m_z = initial_z;
	m_w = initial_w;
}

/*
	Generate a new random number generator with a random selection for the
	seed.
*/
MarsagliaRNG::MarsagliaRNG() {
	struct timeval now;
        gettimeofday(&now, NULL);

        m_z = (uint32_t) now.tv_usec;
	m_w = (uint32_t) now.tv_sec;
}

/*
	Restart with a new set of seeds
*/
void MarsagliaRNG::restart(unsigned int new_z, unsigned int new_w) {
	m_z = new_z;
	m_w = new_w;
}

/*
	Generates a new unsigned integer using the Marsaglia multiple-with-carry method
*/
unsigned int MarsagliaRNG::generateNext() {
	m_z = 36969 * (m_z & 65535) + (m_z >> 16);
        m_w = 18000 * (m_w & 65535) + (m_w >> 16);

	return (m_z << 16) + m_w;
}

/*
	Transform an unsigned integer into a uniform double from which other
	distributed can be generated
*/
double MarsagliaRNG::nextUniform() {
	unsigned int next_uint = generateNext();
        return (next_uint + 1) * 2.328306435454494e-10;
}


uint64_t MarsagliaRNG::generateNextUInt64() {
	int64_t nextInt64 = generateNextInt64();
	uint64_t returnUInt64 = 0;

	char* returnUInt64Ptr = (char*) &returnUInt64;
	const char* nextInt64Ptr = (const char*) &nextInt64;

	for(size_t i = 0; i < sizeof(nextInt64); i++) {
		returnUInt64Ptr[i] = nextInt64Ptr[i];
	}

	return returnUInt64;
}

int64_t  MarsagliaRNG::generateNextInt64() {
	int64_t returnInt64 = 0;
	int32_t lowerHalf = generateNextInt32();
	int32_t upperHalf = generateNextInt32();

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

int32_t  MarsagliaRNG::generateNextInt32() {
	unsigned int nextRN = generateNext();
	int32_t returnInt32 = 0;

	char* returnInt32Ptr = (char*) &returnInt32;
	const char* nextRNPtr = (const char*) &nextRN;

	for(size_t i = 0; i < sizeof(returnInt32); i++) {
		returnInt32Ptr[i] = nextRNPtr[i];
	}

	return returnInt32;
}

void MarsagliaRNG::seed(uint64_t newSeed) {
	m_z = (unsigned int) newSeed;
	m_w = (unsigned int) (((~newSeed) << 1) + 1);
}

uint32_t MarsagliaRNG::generateNextUInt32() {
	int32_t nextInt32 = generateNextInt32();
	uint32_t returnUInt32 = 0;

	char* returnUInt32Ptr = (char*) &returnUInt32;
	const char* nextInt32Ptr = (const char*) &nextInt32;

	for(size_t i = 0; i < sizeof(nextInt32); i++) {
		returnUInt32Ptr[i] = nextInt32Ptr[i];
	}

	return returnUInt32;
}

