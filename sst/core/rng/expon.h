#ifndef _H_SST_CORE_RNG_EXP
#define _H_SST_CORE_RNG_EXP

#include <sst_config.h>
#include "math.h"

#include "distrib.h"
#include "mersenne.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

class SSTExponentialDistribution : public SSTRandomDistribution {

	public:
		SSTExponentialDistribution(double lambda);
		SSTExponentialDistribution(double lambda, SSTRandom* baseDist);
		virtual double getNextDouble();
		double getLambda();

	protected:
		double lambda;
		SSTRandom* baseDistrib;

};

}
}

#endif
