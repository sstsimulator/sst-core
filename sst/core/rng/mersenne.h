
#ifndef _H_MERSENNE_H
#define _H_MERSENNE_H

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <sys/time.h>
#include "sstrand.h"

#define MERSENNE_UINT32_MAX 4294967295U
#define MERSENNE_UINT64_MAX 18446744073709551615ULL
#define MERSENNE_INT32_MAX  2147483647
#define MERSENNE_INT64_MAX  9223372036854775807LL

using namespace std;
using namespace SST;
using namespace SST::RNG;

namespace SST {
namespace RNG {
/*
	Implements a Mersenne-based RNG for use in the SST core or components. The Mersenne
	RNG provides a better "randomness" to the distribution of outputs but is computationally
	more expensive than the Marsaglia RNG.
*/
class MersenneRNG : public SSTRandom {

    public:
	/**
		Create a new Mersenne RNG with a specified seed
		@param[seed] The seed for this RNG
	*/
        MersenneRNG(unsigned int seed);

	/**
		Creates a new Mersenne using a random seed which is obtained from the system
		clock. Note this will give different results on different platforms and between
		runs.
	*/
        MersenneRNG();

	/**
		Generates the next random number as a double value between 0 and 1.
	*/
	double   nextUniform();

	/**
		Generates the next random number as an unsigned 32-bit integer
	*/
	uint32_t generateNextUInt32();

	/**
		Generates the next random number as an unsigned 64-bit integer
	*/
	uint64_t generateNextUInt64();

	/**
		Generates the next random number as a signed 64-bit integer
	*/
	int64_t  generateNextInt64();

	/**
		Generates the next random number as a signed 32-bit integer
	*/
        int32_t  generateNextInt32();

    private:
        void  generateNextBatch();
        uint32_t* numbers;
        int index;

};

}
}

#endif
