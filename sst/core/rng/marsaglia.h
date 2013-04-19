
#ifndef _H_MARSAGLIA_H
#define _H_MARSAGLIA_H

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <sys/time.h>
#include "sstrand.h"

#define MARSAGLIA_UINT32_MAX 4294967295U
#define MARSAGLIA_UINT64_MAX 18446744073709551615ULL
#define MARSAGLIA_INT32_MAX  2147483647
#define MARSAGLIA_INT64_MAX  9223372036854775807LL

using namespace std;
using namespace SST;
using namespace SST::RNG;

namespace SST {
namespace RNG {
/*
	Implements basic random number generation for SST core or 
	components.
*/
class MarsagliaRNG : public SSTRandom {

    public:
        MarsagliaRNG(unsigned int initial_z,
                unsigned int initial_w);
        MarsagliaRNG();
	double   nextUniform();
	uint32_t generateNextUInt32();
	uint64_t generateNextUInt64();
	int64_t  generateNextInt64();
        int32_t   generateNextInt32();

    private:
        unsigned int generateNext();
        unsigned int m_z;
        unsigned int m_w;

};

}
}

#endif
