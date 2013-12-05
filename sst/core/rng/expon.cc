
#include "expon.h"

SSTExponentialDistribution::SSTExponentialDistribution(double mn) {
	mean = mn;
	baseDistrib = new MersenneRNG();
}

double SSTExponentialDistribution::getNextDouble() {
	const double next = baseDistrib->nextUniform();
	return log(1 - next) / ( -1 * mean );
}

double SSTExponentialDistribution::getMean() {
	return mean;
}

