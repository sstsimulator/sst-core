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

#ifndef _H_SST_CORE_RNG_POISSON
#define _H_SST_CORE_RNG_POISSON

#include "math.h"

#include "distrib.h"
#include "mersenne.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

/**
	\class SSTPoissonDistribution poisson.h "sst/core/rng/poisson.h"

	Creates an Poisson distribution for use within SST. This distribution is the same across
	platforms and compilers.
*/
class SSTPoissonDistribution : public SSTRandomDistribution {

	public:
		/**
			Creates an Poisson distribution with a specific lambda
			\param lambda The lambda of the Poisson distribution
		*/
		SSTPoissonDistribution(const double lambda);

		/**
			Creates an Poisson distribution with a specific lambda and a base random number generator
			\param lambda The lambda of the Poisson distribution
			\param baseDist The base random number generator to take the distribution from.
		*/
		SSTPoissonDistribution(const double lambda, SSTRandom* baseDist);

		/**
			Destroys the Poisson distribution
		*/
		~SSTPoissonDistribution();

		/**
			Gets the next (random) double value in the distribution
			\return The next random double from the distribution
		*/
		virtual double getNextDouble();

		/**
			Gets the lambda with which the distribution was created
			\return The lambda which the user created the distribution with
		*/
		double getLambda();

	protected:
		/**
			Sets the lambda of the Poisson distribution.
		*/
		const double lambda;
		/**
			Sets the base random number generator for the distribution.
		*/
		SSTRandom* baseDistrib;

		/**
			Controls whether the base distribution should be deleted when this class is destructed.
		*/
		bool deleteDistrib;

};

}
}

#endif
