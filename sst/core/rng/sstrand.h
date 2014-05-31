// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_RNG_SSTRAND_H
#define SST_CORE_RNG_SSTRAND_H

//#include <iostream>
//#include <fstream>

#include <stdint.h>

//using namespace std;

namespace SST {
namespace RNG {

/**
	\class SSTRandom sstrand.h "sst/core/rng/sstrand.h"

	Implements the base class for random number generators for the SST core. This does not
	implement an actual RNG itself only the base class which describes the methods each
	class will implement.
*/
class SSTRandom {

    public:
	/**
		Generates the next random number in the range 0 to 1.
	*/
	virtual double   nextUniform() = 0;

	/**
		Generates the next random number as an unsigned 32-bit integer.
	*/
	virtual uint32_t generateNextUInt32() = 0;

	/**
		Generates the next random number as an unsigned 64-bit integer.
	*/
	virtual uint64_t generateNextUInt64() = 0;

	/**
		Generates the next random number as a signed 64-bit integer.
	*/
	virtual int64_t	 generateNextInt64() = 0;

	/**
		Generates the next random number as a signed 32-bit integer
	*/
        virtual int32_t  generateNextInt32() = 0;

	/**
		Destroys the random number generator
	*/
	virtual ~SSTRandom() { }

};

} //namespace RNG
} //namespace SST

#endif //SST_CORE_RNG_SSTRAND_H
