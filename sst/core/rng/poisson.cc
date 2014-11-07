// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "poisson.h"

static const double SST_POISSON_PI = 3.14159265358979323846;

SSTPoissonDistribution::SSTPoissonDistribution(const double mn) :
	SSTRandomDistribution(), lambda(mn) {

	baseDistrib = new MersenneRNG();
	deleteDistrib = true;
}

SSTPoissonDistribution::SSTPoissonDistribution(const double mn, SSTRandom* baseDist) :
	SSTRandomDistribution(), lambda(mn) {

	baseDistrib = baseDist;
	deleteDistrib = false;
}

SSTPoissonDistribution::~SSTPoissonDistribution() {
	if(deleteDistrib) {
		delete baseDistrib;
	}
}

double SSTPoissonDistribution::getNextDouble() {
	const double c      = 0.767 - 3.36 / lambda;
	const double beta   = SST_POISSON_PI / sqrt(3.0 * lambda);
	const double alpha  = beta * lambda;
	const double k      = log(c) - lambda - log(beta);

	while(true) {
		const double u = baseDistrib->nextUniform();
		const double x = (alpha - log( ( 1.0 - u ) / u )) / beta;
		const int    n = (int) floor(x + 0.5);
		if(n < 0) continue;

		const double v     = baseDistrib->nextUniform();
		const double y     = alpha - beta * x;
		const double temp  = 1.0 + exp(y);
		const double lhs   = y + log(v / (temp * temp));
		const double rhs   = k + n * log(lambda) - logFac(n);

		if(lhs <= rhs) {
			return n;
		}
	}
}

double SSTPoissonDistribution::logFac(const int n) {
	const double x = n + 1;
	return (x - 0.5)*log(x) - x + 0.5 * log(2 * SST_POISSON_PI + 1.0 / (12.0 * x));
}

double SSTPoissonDistribution::getLambda() {
	return lambda;
}
