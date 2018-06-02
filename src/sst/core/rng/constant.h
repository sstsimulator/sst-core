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

#ifndef _H_SST_CORE_RNG_EMPTY
#define _H_SST_CORE_RNG_EMPTY

#include "math.h"

#include "distrib.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

/**
	\class SSTConstantDistribution constant.h "sst/core/rng/constant.h"

	Implements a distribution which always returns a constant value (provided by the user). This
	can be used in situations where the user may not want to apply a distribution.
*/
class SSTConstantDistribution : public SSTRandomDistribution {

	public:
		/**
			Creates a constant distribution which returns a constant value.
			\param v Is the constant value the user wants returned by the distribution
		*/
    SSTConstantDistribution(double v) :
    SSTRandomDistribution() {
        mean = v;
    }

		/**
			Destroys the constant distribution
		*/
    ~SSTConstantDistribution() {
        
    }

		/**
			Gets the next double for the distribution, in this case it will return the constant
			value specified by the user
			\return Constant value specified by the user when creating the class
		*/
    double getNextDouble() {
        return mean;
    }

		/**
			Gets the constant value for the distribution
			\return Constant value specified by the user when creating the class
		*/
    double getMean() {
        return mean;
    }

	protected:
		/**
			Describes the constant value to return from the distribution.
		*/
		double mean;

};

}
}

#endif
