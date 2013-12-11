#ifndef _H_SST_CORE_RNG_GAUSSIAN
#define _H_SST_CORE_RNG_GAUSSIAN

#include "math.h"

#include "distrib.h"
#include "mersenne.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

class SSTGaussianDistribution : public SSTRandomDistribution {

	public:
		SSTGaussianDistribution(double mean, double stddev);
		SSTGaussianDistribution(double mean, double stddev, SSTRandom* baseRNG);

		virtual double getNextDouble();
		double getMean();
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
