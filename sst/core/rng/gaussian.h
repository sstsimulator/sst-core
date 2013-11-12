#ifndef _H_SST_CORE_RNG_GAUSSIAN
#define _H_SST_CORE_RNG_GAUSSIAN

#include <sst_config.h>
#include "math.h"

#include "distrib.h"
#include "mersenne.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

class GaussianDistribution : public RandomDistribution {

	public:
		GaussianDistribution(double mean, double stddev);
		virtual double getNextDouble();		
		double getMean();
		double getStandardDev();

	protected:
		double mean;
		double stddev;
		MersenneRNG* baseDistrib;
		double unusedPair;
		bool usePair;
};

}
}

#endif
