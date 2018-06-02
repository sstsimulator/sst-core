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

#ifndef _H_SST_CORE_RNG_DISCRETE
#define _H_SST_CORE_RNG_DISCRETE

#include "math.h"

#include <cstdlib>  // for malloc/free

#include "distrib.h"
#include "mersenne.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

/**
	\class SSTDiscreteDistribution discrete.h "sst/core/rng/discrete.h"

	Creates a discrete distribution for use within SST. This distribution is the same across
	platforms and compilers.
*/
class SSTDiscreteDistribution : public SSTRandomDistribution {

	public:
		/**
			Creates an exponential distribution with a specific lambda
			\param lambda The lambda of the exponential distribution
		*/
    SSTDiscreteDistribution(const double* probs, const uint32_t probsCount) :
        SSTRandomDistribution(),
        probCount(probsCount) {

        probabilities = (double*) malloc(sizeof(double) * probsCount);
	double prob_sum = 0;

	for(uint32_t i = 0; i < probsCount; i++) {
		probabilities[i] = prob_sum;
		prob_sum += probs[i];
	}

            baseDistrib = new MersenneRNG();
            deleteDistrib = true;
    }

    /**
        Creates an exponential distribution with a specific lambda and a base random number generator
        \param lambda The lambda of the exponential distribution
        \param baseDist The base random number generator to take the distribution from.
    */
    SSTDiscreteDistribution(const double* probs, const uint32_t probsCount, SSTRandom* baseDist) :
        probCount(probsCount) {
        
        probabilities = (double*) malloc(sizeof(double) * probsCount);
	double prob_sum = 0;

	for(uint32_t i = 0; i < probsCount; i++) {
		probabilities[i] = prob_sum;
		prob_sum += probs[i];
	}

        baseDistrib = baseDist;
            deleteDistrib = false;
    }

		/**
			Destroys the exponential distribution
		*/
    ~SSTDiscreteDistribution() {
        free(probabilities);
        
        if(deleteDistrib) {
            delete baseDistrib;
        }
    }

		/**
			Gets the next (random) double value in the distribution
			\return The next random double from the discrete distribution, this is the double converted of the index where the probability is located
		*/
    double getNextDouble() {
        const double nextD = baseDistrib->nextUniform();

        uint32_t index = 0;

        for(; index < probCount; index++) {
            if(probabilities[index] >= nextD) {
                break;
            }
        }

        return (double) index;
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
			The discrete probability list
		*/
		double* probabilities;

		/**
			Count of discrete probabilities
		*/
		uint32_t probCount;

};

}
}

#endif
