
#ifndef _H_SST_RAND
#define _H_SST_RAND

#include <iostream>
#include <fstream>

#include <stdint.h>

using namespace std;

namespace SST {
namespace RNG {

/*
	Implements basic random number generation for SST core or 
	components.
*/
class SSTRandom {

    public:
	virtual double   nextUniform() = 0;
	virtual uint32_t generateNextUInt32() = 0;
	virtual uint64_t generateNextUInt64() = 0;
	virtual int64_t	 generateNextInt64() = 0;
        virtual int32_t  generateNextInt32() = 0;

};

}
}

#endif
