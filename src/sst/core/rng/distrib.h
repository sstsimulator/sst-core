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

#ifndef SST_CORE_RNG_DISTRIB_H
#define SST_CORE_RNG_DISTRIB_H

namespace SST {
namespace RNG {

/**
 * \class RandomDistribution
 * Base class of statistical distributions in SST.
 */
class RandomDistribution
{

public:
    /**
        Obtains the next double from the distribution
        \return The next double in the distribution being sampled
    */
    virtual double getNextDouble() = 0;

    /**
        Destroys the distribution
    */
    virtual ~RandomDistribution() {};

    /**
        Creates the base (abstract) class of a distribution
    */
    RandomDistribution() {};
};

using SSTRandomDistribution = SST::RNG::RandomDistribution;

} // namespace RNG
} // namespace SST

#endif // SST_CORE_RNG_DISTRIB_H
