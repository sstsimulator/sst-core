
#include <sst_config.h>
#include "gaussian.h"

SSTGaussianDistribution::SSTGaussianDistribution(double mn, double sd) {
	mean = mn;
	stddev = sd;

	baseDistrib = new MersenneRNG();
	unusedPair = 0;
	usePair = false;
}

SSTGaussianDistribution::SSTGaussianDistribution(double mn, double sd, SSTRandom* baseRNG) {
	mean = mn;
	stddev = sd;

	baseDistrib = baseRNG;
	unusedPair = 0;
	usePair = false;
}

/*
	Use the Marsaglia Polar Method (Marsaglia and Bray) to generate a pair of
	Gaussian distributed values, using the second generated value for a repeat
	to this call.
*/
double SSTGaussianDistribution::getNextDouble() {
	if(usePair) {
		usePair = false;
		return unusedPair;
	} else {
		double gauss_u, gauss_v, sq_sum;

		do {
			gauss_u = baseDistrib->nextUniform();
			gauss_v = baseDistrib->nextUniform();
			sq_sum = (gauss_u * gauss_u) + (gauss_v * gauss_v);
		} while(sq_sum >= 1 || sq_sum == 0);

		if(baseDistrib->nextUniform() < 0.5) {
			gauss_u *= -1.0;
		}

		if(baseDistrib->nextUniform() < 0.5) {
			gauss_v *= -1.0;
		}

		double multipler = sqrt(-2.0 * log(sq_sum) / sq_sum);
		unusedPair = mean + stddev * gauss_v * multipler;
		usePair = true;

		return mean + stddev * gauss_u * multipler;
	}
}

double SSTGaussianDistribution::getMean() {
	return mean;
}

double SSTGaussianDistribution::getStandardDev() {
	return stddev;
}
