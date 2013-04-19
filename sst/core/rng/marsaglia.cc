
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
#if defined(__i386__)
  		unsigned long long int x;

     		__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
		m_z = (unsigned int) x;

     		__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
		m_w = (unsigned int) x;

#elif defined(__x86_64__)
	 	 unsigned hi, lo;
  		__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  		m_z = (unsigned int) ( ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 ) );

  		__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  		m_w = (unsigned int) ( ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 ) );
#else
		// Create seeds from two primes
		m_z = 102197;
		m_w = 47657;
#endif
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

