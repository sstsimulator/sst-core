#ifndef _H_SST_CORE_RNG_EMPTY
#define _H_SST_CORE_RNG_EMPTY

#include "math.h"

#include "distrib.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

/**
	Implements a distribution which always returns a constant value (provided by the user). This
	can be used in situations where the user may not want to apply a distribution.
*/
class SSTConstantDistribution : public SSTRandomDistribution {

	public:
		/**
			Creates a constant distribution which returns a constant value.
			\param mean Is the constant value the user wants returned by the distribution
		*/
		SSTConstantDistribution(double mean);

		/**
			Gets the next double for the distribution, in this case it will return the constant
			value specified by the user
			\return Constant value specified by the user when creating the class
		*/
		double getNextDouble();

		/**
			Gets the constant value for the distribution
			\return Constant value specified by the user when creating the class
		*/
		double getMean();

	protected:
		double mean;

};

}
}

#endif
