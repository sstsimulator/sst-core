
#include <sst_config.h>
#include "expon.h"

SSTExponentialDistribution::SSTExponentialDistribution(double mn) {
	lambda = mn;
	baseDistrib = new MersenneRNG();
}

SSTExponentialDistribution::SSTExponentialDistribution(double mn, SSTRandom* baseDist) {
	lambda = mn;
	baseDistrib = baseDist;
}

double SSTExponentialDistribution::getNextDouble() {
	const double next = baseDistrib->nextUniform();
	return log(1 - next) / ( -1 * lambda );
}

double SSTExponentialDistribution::getLambda() {
	return lambda;
}
