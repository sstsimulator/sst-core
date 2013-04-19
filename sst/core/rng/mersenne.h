
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
	Implements basic random number generation for SST core or 
	components.
*/
class MersenneRNG : public SSTRandom {

    public:
        MersenneRNG(unsigned int seed);
        MersenneRNG();

	double   nextUniform();
	uint32_t generateNextUInt32();
	uint64_t generateNextUInt64();
	int64_t  generateNextInt64();
        int32_t  generateNextInt32();

    private:
        void  generateNextBatch();
        uint32_t* numbers;
        int index;

};

}
}

#endif
