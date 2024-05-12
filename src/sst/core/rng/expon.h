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

#ifndef SST_CORE_RNG_EXPON_H
#define SST_CORE_RNG_EXPON_H

#include "distrib.h"
#include "math.h"
#include "mersenne.h"
#include "rng.h"

using namespace SST::RNG;

namespace SST {
namespace RNG {

/**
    \class ExponentialDistribution expon.h "sst/core/rng/expon.h"

    Creates an exponential distribution for use within SST. This distribution is the same across
    platforms and compilers.
*/
class ExponentialDistribution : public RandomDistribution
{

public:
    /**
        Creates an exponential distribution with a specific lambda
        \param mn The lambda of the exponential distribution
    */
    ExponentialDistribution(const double mn) : RandomDistribution()
    {

        lambda        = mn;
        baseDistrib   = new MersenneRNG();
        deleteDistrib = true;
    }

    /**
        Creates an exponential distribution with a specific lambda and a base random number generator
        \param mn The lambda of the exponential distribution
        \param baseDist The base random number generator to take the distribution from.
    */
    ExponentialDistribution(const double mn, SST::RNG::Random* baseDist) : RandomDistribution()
    {

        lambda        = mn;
        baseDistrib   = baseDist;
        deleteDistrib = false;
    }

    /**
        Destroys the exponential distribution
    */
    ~ExponentialDistribution()
    {
        if ( deleteDistrib ) { delete baseDistrib; }
    }

    /**
        Gets the next (random) double value in the distribution
        \return The next random double from the distribution
    */
    double getNextDouble() override
    {
        const double next = baseDistrib->nextUniform();
        return log(1 - next) / (-1 * lambda);
    }

    /**
        Gets the lambda with which the distribution was created
        \return The lambda which the user created the distribution with
    */
    double getLambda() { return lambda; }

    /**
        Default constructor. FOR SERIALIZATION ONLY.
     */
    ExponentialDistribution() : RandomDistribution() {}

    /**
        Serialization function for checkpoint
    */
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& lambda;
        ser& baseDistrib;
        ser& deleteDistrib;
    }

    /**
           Serialization macro
       */
    ImplementSerializable(SST::RNG::ExponentialDistribution)

protected:
    /**
        Sets the lambda of the exponential distribution.
    */
    double            lambda;
    /**
        Sets the base random number generator for the distribution.
    */
    SST::RNG::Random* baseDistrib;

    /**
        Controls whether the base distribution should be deleted when this class is destructed.
    */
    bool deleteDistrib;
};

using SSTExponentialDistribution = SST::RNG::ExponentialDistribution;

} // namespace RNG
} // namespace SST

#endif // SST_CORE_RNG_EXPON_H
