// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
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
        	SSTRandomDistribution(), probCount(probsCount), deleteDistrib(true) {

		if( probCount > 0 ) {
			probPerBin = 1.0 / static_cast<double>( probCount );
		}

        	baseDistrib = new MersenneRNG();
    	}

    	/**
	        Creates a Uniform distribution with a specific number of bins and user supplied
	    	random number generator
	        \param probsCount Number of probability bins in the distribution
        	\param baseDist The base random number generator to take the distribution from.
    	*/
    	SSTUniformDistribution(const uint32_t probsCount, SSTRandom* baseDist) :
   		SSTRandomDistribution(), probCount(probsCount), deleteDistrib(false) {

		if( probCount > 0 ) {
			probPerBin = 1.0 / static_cast<double>( probCount );
		}

        	baseDistrib = baseDist;
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
		uint32_t current_bin = 1;

		while( nextD > ( static_cast<double>( current_bin ) * probPerBin ) ) {
			current_bin++;
		}

		return static_cast<double>( current_bin - 1 );
    	}

    protected:
        /**
            Sets the base random number generator for the distribution.
        */
        SSTRandom* baseDistrib;

        /**
            Controls whether the base distribution should be deleted when this class is destructed.
        */
        const bool deleteDistrib;

        /**
            Count of discrete probabilities
        */
        const uint32_t probCount;

	/**
	    Range 0..1 split into discrete bins
	*/
	double probPerBin;

};

}
}

#endif
