// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "poisson.h"

#include "distrib.h"

using namespace SST::RNG;

PoissonDistribution::PoissonDistribution(const double mn) : RandomDistribution(), lambda(mn)
{

    baseDistrib   = new MersenneRNG();
    deleteDistrib = true;
}

PoissonDistribution::PoissonDistribution(const double mn, SST::RNG::Random* baseDist) : RandomDistribution(), lambda(mn)
{

    baseDistrib   = baseDist;
    deleteDistrib = false;
}

PoissonDistribution::~PoissonDistribution()
{
    if ( deleteDistrib ) { delete baseDistrib; }
}

double
PoissonDistribution::getNextDouble()
{
    const double L = exp(-lambda);
    double       p = 1.0;
    int          k = 0;

    do {
        k++;
        p *= baseDistrib->nextUniform();
    } while ( p > L );

    return k - 1;
}

double
PoissonDistribution::getLambda()
{
    return lambda;
}
