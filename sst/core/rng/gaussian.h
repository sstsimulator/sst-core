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
		double mean;
		double stddev;
		SSTRandom* baseDistrib;
		double unusedPair;
		bool usePair;
};

}
}

#endif
