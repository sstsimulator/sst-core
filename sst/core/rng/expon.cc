
#include "expon.h"

ExponentialDistribution::ExponentialDistribution(double mn) {
	mean = mn;
	baseDistrib = new MersenneRNG();
}

double ExponentialDistribution::getNextDouble() {
	const double next = baseDistrib->nextUniform();
	return log(1 - next) / ( -1 * mean );
}

double ExponentialDistribution::getMean() {
	return mean;
}

