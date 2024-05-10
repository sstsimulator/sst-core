// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_RNG_CONSTANT_H
#define SST_CORE_RNG_CONSTANT_H

#include "distrib.h"
#include "math.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

/**
    \class ConstantDistribution constant.h "sst/core/rng/constant.h"

    Implements a distribution which always returns a constant value (provided by the user). This
    can be used in situations where the user may not want to apply a distribution.
*/
class ConstantDistribution : public RandomDistribution
{

public:
    /**
        Creates a constant distribution which returns a constant value.
        \param v Is the constant value the user wants returned by the distribution
    */
    ConstantDistribution(double v) : RandomDistribution() { mean = v; }

    /**
        Destroys the constant distribution
    */
    ~ConstantDistribution() {}

    /**
        Gets the next double for the distribution, in this case it will return the constant
        value specified by the user
        \return Constant value specified by the user when creating the class
    */
    double getNextDouble() override { return mean; }

    /**
        Gets the constant value for the distribution
        \return Constant value specified by the user when creating the class
    */
    double getMean() { return mean; }

    /**
        Default constructor. FOR SERIALIZATION ONLY.
     */
    ConstantDistribution() : RandomDistribution() {}

    /**
        Serialization function for checkpoint
    */
    void serialize_order(SST::Core::Serialization::serializer& ser) override { ser& mean; }

    /**
        Serialization macro
    */
    ImplementSerializable(SST::RNG::ConstantDistribution)

protected:
    /**
        Describes the constant value to return from the distribution.
    */
    double mean;
};

using SSTConstantDistribution = SST::RNG::ConstantDistribution;

} // namespace RNG
} // namespace SST

#endif // SST_CORE_RNG_CONSTANT_H
