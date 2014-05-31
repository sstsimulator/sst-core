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
#include "expon.h"

SSTExponentialDistribution::SSTExponentialDistribution(double mn) :
	SSTRandomDistribution() {

	lambda = mn;
	baseDistrib = new MersenneRNG();
	deleteDistrib = true;
}

SSTExponentialDistribution::SSTExponentialDistribution(double mn, SSTRandom* baseDist) :
	SSTRandomDistribution() {

	lambda = mn;
	baseDistrib = baseDist;
	deleteDistrib = false;
}

SSTExponentialDistribution::~SSTExponentialDistribution() {
	if(deleteDistrib) {
		delete baseDistrib;
	}
}

double SSTExponentialDistribution::getNextDouble() {
	const double next = baseDistrib->nextUniform();
	return log(1 - next) / ( -1 * lambda );
}

double SSTExponentialDistribution::getLambda() {
	return lambda;
}
