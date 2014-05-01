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

#ifndef _H_SST_CORE_RNG_GAUSSIAN
#define _H_SST_CORE_RNG_GAUSSIAN

#include "math.h"

#include "distrib.h"
#include "mersenne.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

/**
	Creates a Gaussian (normal) distribution for which to sample
*/
class SSTGaussianDistribution : public SSTRandomDistribution {

	public:
		/**
			Creates a new distribution with a predefined random number generator with a specified mean and standard deviation.
			\param mean The mean of the Gaussian distribution
			\param stddev The standard deviation of the Gaussian distribution
		*/
		SSTGaussianDistribution(double mean, double stddev);

		/**
			Creates a new distribution with a predefined random number generator with a specified mean and standard deviation.
			\param mean The mean of the Gaussian distribution
			\param stddev The standard deviation of the Gaussian distribution
			\param baseRNG The random number generator as the base of the distribution
		*/
		SSTGaussianDistribution(double mean, double stddev, SSTRandom* baseRNG);

		/**
			Gets the next double value in the distributon
			\return The next double value of the distribution (in this case a Gaussian distribution)
		*/
		virtual double getNextDouble();

		/**
			Gets the mean of the distribution
			\return The mean of the Guassian distribution
		*/
		double getMean();

		/**
			Gets the standard deviation of the distribution
			\return The standard deviation of the Gaussian distribution
		*/
		double getStandardDev();

	protected:
		/**
			The mean of the Gaussian distribution
		*/
		double mean;
		/**
			The standard deviation of the Gaussian distribution
		*/
		double stddev;
		/**
			The base random number generator for the distribution
		*/
		SSTRandom* baseDistrib;
		/**
			Random numbers for the distribution are read in pairs, this stores the second of the pair
		*/
		double unusedPair;
		/**
			Random numbers for the distribution are read in pairs, this tells the code to use the second of the pair
		*/
		bool usePair;
};

}
}

#endif
