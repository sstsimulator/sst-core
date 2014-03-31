#ifndef _H_SST_CORE_RNG_EMPTY
#define _H_SST_CORE_RNG_EMPTY

#include "math.h"

#include "distrib.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

class SSTConstantDistribution : public SSTRandomDistribution {

	public:
		SSTConstantDistribution(double mean);

		double getNextDouble();
		double getMean();

	protected:
		double mean;

};

}
}

#endif
