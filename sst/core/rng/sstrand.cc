
#include "sstrand.h"

/*
	Seed the Marsaglia method with two initializers that must be non-zero.
*/
SSTRandom::SSTRandom(unsigned int initial_z, unsigned int initial_w) {
	m_z = initial_z;
	m_w = initial_w;
}

/*
	Generate a new random number generator with a random selection for the
	seed.
*/
SSTRandom::SSTRandom() {
	fstream rand_file("/dev/random", fstream::in);
	rand_file >> m_z;
	rand_file >> m_w;
	rand_file.close();
}

/*
	Generates a new unsigned integer using the Marsaglia multiple-with-carry method
*/
unsigned int SSTRandom::generateNext() {
	m_z = 36969 * (m_z & 65535) + (m_z >> 16);
        m_w = 18000 * (m_w & 65535) + (m_w >> 16);

	return (m_z << 16) + m_w;
}

/*
	Transform an unsigned integer into a uniform double from which other
	distributed can be generated
*/
double SSTRandom::nextUniform() {
	unsigned int next_uint = generateNext();
        return (next_uint + 1) * 2.328306435454494e-10;
}
