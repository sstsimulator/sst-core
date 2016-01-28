// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
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
	return nextUniform() * (uint64_t) XORSHIFT_UINT64_MAX;
}

int64_t  XORShiftRNG::generateNextInt64() {
	double next = nextUniform();
	if(next > 0.5) 
		next = next * -0.5;
	next = next * 2;

	return (int64_t) (next * ((int64_t) XORSHIFT_INT64_MAX));
}

int32_t  XORShiftRNG::generateNextInt32() {
	double next = nextUniform();
	if(next > 0.5) 
		next = next * -0.5;
	next = next * 2;

	return (int32_t) (next * ((int32_t) XORSHIFT_INT32_MAX));
}

void XORShiftRNG::seed(uint64_t seed) {
	x = (uint32_t) seed;
	y = 0;
	w = 0;
	z = 0;
}

XORShiftRNG::~XORShiftRNG() {

}
