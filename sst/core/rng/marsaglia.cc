
#include "sstrand.h"
#include "marsaglia.h"

/*
	Seed the Marsaglia method with two initializers that must be non-zero.
*/
MarsagliaRNG::MarsagliaRNG(unsigned int initial_z, unsigned int initial_w) {
	m_z = initial_z;
	m_w = initial_w;
}

/*
	Generate a new random number generator with a random selection for the
	seed.
*/
MarsagliaRNG::MarsagliaRNG() {
	struct timeval now;
        gettimeofday(&now, NULL);

        m_z = (uint32_t) now.tv_usec;
	m_w = (uint32_t) now.tv_sec;
}

/*
	Generates a new unsigned integer using the Marsaglia multiple-with-carry method
*/
unsigned int MarsagliaRNG::generateNext() {
	m_z = 36969 * (m_z & 65535) + (m_z >> 16);
        m_w = 18000 * (m_w & 65535) + (m_w >> 16);

	return (m_z << 16) + m_w;
}

/*
	Transform an unsigned integer into a uniform double from which other
	distributed can be generated
*/
double MarsagliaRNG::nextUniform() {
	unsigned int next_uint = generateNext();
        return (next_uint + 1) * 2.328306435454494e-10;
}


uint64_t MarsagliaRNG::generateNextUInt64() {
	return nextUniform() * MARSAGLIA_UINT64_MAX;
}

int64_t  MarsagliaRNG::generateNextInt64() {
	double next = nextUniform();
	if(next > 0.5)
		next = next * -0.5;
	next = next * 2;

	return (int64_t) (next * MARSAGLIA_INT64_MAX);
}

int32_t  MarsagliaRNG::generateNextInt32() {
	double next = nextUniform();
	if(next > 0.5)
		next = next * -0.5;
	next = next * 2;

	return (int32_t) (next * MARSAGLIA_INT32_MAX);
}


uint32_t MarsagliaRNG::generateNextUInt32() {
	return nextUniform() * MARSAGLIA_UINT32_MAX;
}

