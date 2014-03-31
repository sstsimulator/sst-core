
#include "constant.h"

SSTConstantDistribution::SSTConstantDistribution(double v) {
	mean = v;
}

double SSTConstantDistribution::getNextDouble() {
	return mean;
}

double SSTConstantDistribution::getMean() {
	return mean;
}
