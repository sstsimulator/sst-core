// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include "poisson.h"

using namespace SST::RNG;

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
	const double L = exp(-lambda);
	      double p = 1.0;
	      int k = 0;

	do {
		k++;
		p *= baseDistrib->nextUniform();
	} while(p > L);

	return k - 1;
}

double SSTPoissonDistribution::getLambda() {
	return lambda;
}
