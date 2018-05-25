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

#include "mersenne.h"

#include <cstdlib>
#include <cstring>

using namespace SST;
using namespace SST::RNG;
/*
	Generate a new random number generator with a random selection for the
	seed.
*/
MersenneRNG::MersenneRNG() {
	numbers = (uint32_t*) malloc(sizeof(uint32_t) * 624);

	struct timeval now;
	gettimeofday(&now, NULL);

	numbers[0] = (uint32_t) now.tv_usec;
	index = 0;

	for(int i = 1 ; i < 624; i++) {
		const uint32_t temp = ((uint32_t) 1812433253UL) * (numbers[i-1] ^ (numbers[i-1] >> 30)) + i;
		numbers[i] = temp;
	}
}

/*
	Seed the Mersenne and then make a group of numbers
*/
MersenneRNG::MersenneRNG(unsigned int startSeed) {
	seed(startSeed);
}

void MersenneRNG::generateNextBatch() {
	index = 0;
	for(int i = 0; i < 624; ++i) {
		uint32_t temp = (numbers[i] & 0x80000000) +
			(numbers[(i+1) % 624] & 0x7fffffff);

		numbers[i] = numbers[(i + 397) % 624] ^ (temp >> 1);

		if(temp % 2 != 0) {
			numbers[i] = numbers[i] ^ 2567483615UL;
		}
	}
}

/*
	Transform an unsigned integer into a uniform double from which other
	distributed can be generated
*/
double MersenneRNG::nextUniform() {
	uint32_t temp = generateNextUInt32();
	return ( (double) temp ) / (double) MERSENNE_UINT32_MAX;
}

uint32_t MersenneRNG::generateNextUInt32() {
	if(index == 0)
                generateNextBatch();

        uint32_t temp = numbers[index];
        temp = temp ^ (temp >> 11);
        temp = temp ^ ((temp << 7) & 2636928640UL);
        temp = temp ^ ((temp << 15) & 4022730752UL);
        temp = temp ^ (temp >> 18);

        index = (index + 1) % 624;
	return (uint32_t) temp;
}

uint64_t MersenneRNG::generateNextUInt64() {
	int64_t nextInt64 = generateNextInt64();
	uint64_t returnUInt64 = 0;

	std::memcpy(&returnUInt64, &nextInt64, sizeof(nextInt64));

	return returnUInt64;
}

int64_t  MersenneRNG::generateNextInt64() {
	uint32_t lowerHalf = generateNextUInt32();
	uint32_t upperHalf = generateNextUInt32();
	int64_t returnNumber = 0;

	char* returnNumberPtr = (char*) &returnNumber;
	const char* lowerHalfPtr = (const char*) &lowerHalf;
	const char* upperHalfPtr = (const char*) &upperHalf;

	for(size_t i = 0; i < sizeof(lowerHalf); i++) {
		returnNumberPtr[i] = lowerHalfPtr[i];
	}

	for(size_t i = 0; i < sizeof(upperHalf); i++) {
		returnNumberPtr[i + 4] = upperHalfPtr[i];
	}

	return returnNumber;
}

int32_t  MersenneRNG::generateNextInt32() {
	uint32_t next_uint32 = generateNextUInt32();
	int32_t castReturn = 0;

	std::memcpy(&castReturn, &next_uint32, sizeof(next_uint32));

	return castReturn;
}

void MersenneRNG::seed(uint64_t seed) {
	numbers = (uint32_t*) malloc(sizeof(uint32_t) * 624);
	numbers[0] = (uint32_t) seed;
	index = 0;

	for(int i = 1 ; i < 624; i++) {
		const uint32_t temp = ((uint32_t) 1812433253UL) * (numbers[i-1] ^ (numbers[i-1] >> 30)) + i;
		numbers[i] = temp;
	}
}

MersenneRNG::~MersenneRNG() {
	free(numbers);
}
