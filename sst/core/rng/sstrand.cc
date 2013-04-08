// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


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
