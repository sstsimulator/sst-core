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

#ifndef _H_SST_CORE_RNG_UNIFORM
#define _H_SST_CORE_RNG_UNIFORM

#include "math.h"

#include "distrib.h"
#include "mersenne.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

/**
	\class SSTUniformDistribution uniform.h "sst/core/rng/uniform.h"

	Creates a Uniform distribution for use within SST. This distribution is the same across
	platforms and compilers.
*/
class SSTUniformDistribution : public SSTRandomDistribution {

	public:
		/**
			Creates an uniform distribution with a specific number of bins
			\param probsCount Number of probability bins in this distribution
		*/
        SSTUniformDistribution(const uint32_t probsCount) :
        SSTRandomDistribution(),
	probCount(probsCount) {

        baseDistrib = new MersenneRNG();
        deleteDistrib = true;
    }

    /**
        Creates a Uniform distribution with a specific number of bins and user supplied
	random number generator
        \param probsCount Number of probability bins in the distribution
        \param baseDist The base random number generator to take the distribution from.
    */
    SSTUniformDistribution(const uint32_t probsCount, SSTRandom* baseDist) :
	SSTRandomDistribution(),
	probCount(probsCount) {

        baseDistrib = baseDist;
        deleteDistrib = false;
    }

		/**
			Destroys the distribution and will delete locally allocated RNGs
		*/
    ~SSTUniformDistribution() {
        if(deleteDistrib) {
            delete baseDistrib;
        }
    }

    /**
	Gets the next (random) double value in the distribution
	\return The next random double from the distribution, this is the double converted of the index where the probability is located
    */
    double getNextDouble() {
        const double nextD = baseDistrib->nextUniform();
	const double probPerBin = 1.0 / (double)(probCount);

	for(uint32_t i = 0; i < probCount; ++i) {
		if(nextD < ( ((double) i) * probPerBin )) {
			return i;
		}
	}

	return probCount;
    }

	protected:
		/**
			Sets the base random number generator for the distribution.
		*/
		SSTRandom* baseDistrib;

		/**
			Controls whether the base distribution should be deleted when this class is destructed.
		*/
		bool deleteDistrib;

		/**
			Count of discrete probabilities
		*/
		uint32_t probCount;

};

}
}

#endif
