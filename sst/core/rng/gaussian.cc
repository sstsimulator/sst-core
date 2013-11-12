
#include "gaussian.h"

GaussianDistribution::GaussianDistribution(double mn, double sd) {
	mean = mn;
	stddev = sd;

	baseDistrib = new MersenneRNG();
	unusedPair = 0;
	usePair = false;
}

/*
	Use the Marsaglia Polar Method (Marsaglia and Bray) to generate a pair of
	Gaussian distributed values, using the second generated value for a repeat
	to this call.
*/
double GaussianDistribution::getNextDouble() {
	if(usePair) {
		return unusedPair;
	} else {
		double gauss_u, gauss_v, sq_sum;

		do {
			gauss_u = baseDistrib->nextUniform();
			gauss_v = baseDistrib->nextUniform();
			sq_sum = (gauss_u * gauss_u) + (gauss_v * gauss_v);
		} while(sq_sum >= 1 || sq_sum == 0);

		double multipler = sqrt(-2.0 * log(sq_sum) / sq_sum);
		unusedPair = gauss_v * multipler;
		usePair = true;

		return mean + stddev * gauss_u * multipler;
	}
}

double GaussianDistribution::getMean() {
	return mean;
}

double GaussianDistribution::getStandardDev() {
	return stddev;
}
