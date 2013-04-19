
#include "sstrand.h"
#include "mersenne.h"

/*
	Generate a new random number generator with a random selection for the
	seed.
*/
MersenneRNG::MersenneRNG() {
	numbers = (uint32_t*) malloc(sizeof(uint32_t) * 624);

	struct timeval now;
	gettimeofday(&now, NULL);

	numbers[0] = (uint32_t) now.tv_usec;
	index = 0;

	for(int i = 1 ; i < 624; i++) {
		const uint32_t temp = 1812433253UL * (numbers[i-1] ^ (numbers[i-1] >> 30)) + i;
		numbers[i] = temp;
	}
}

/*
	Seed the Mersenne and then make a group of numbers
*/
MersenneRNG::MersenneRNG(unsigned int seed) {
	numbers = (uint32_t*) malloc(sizeof(uint32_t) * 624);
	numbers[0] = (uint32_t) seed;
	index = 0;

	for(int i = 1 ; i < 624; i++) {
		const uint32_t temp = 1812433253UL * (numbers[i-1] ^ (numbers[i-1] >> 30)) + i;
		numbers[i] = temp;
	}
}

void MersenneRNG::generateNextBatch() {
	index = 0;
	for(int i = 0; i < 624; ++i) {
		uint32_t temp = (numbers[i] & 0x80000000) +
			(numbers[(i+1) % 624] & 0x7fffffff);

		numbers[i] = numbers[(i + 397) % 624] ^ (temp >> 1);

		if(temp % 2 != 0) {
			numbers[i] = numbers[i] ^ 2567483615UL;
		}
	}
}

/*
	Transform an unsigned integer into a uniform double from which other
	distributed can be generated
*/
double MersenneRNG::nextUniform() {
	uint32_t temp = generateNextUInt32();
	return ( (double) temp ) / (double) MERSENNE_UINT32_MAX;
}

uint32_t MersenneRNG::generateNextUInt32() {
	if(index == 0)
                generateNextBatch();

        uint32_t temp = numbers[index];
        temp = temp ^ (temp >> 11);
        temp = temp ^ ((temp << 7) & 2636928640UL);
        temp = temp ^ ((temp << 15) & 4022730752UL);
        temp = temp ^ (temp >> 18);

        index = (index + 1) % 624;
	return (uint32_t) temp;
}

uint64_t MersenneRNG::generateNextUInt64() {
	return nextUniform() * MERSENNE_UINT64_MAX;
}

int64_t  MersenneRNG::generateNextInt64() {
	double next = nextUniform();
	if(next > 0.5) 
		next = next * -0.5;
	next = next * 2;

	return (int64_t) (next * MERSENNE_INT64_MAX);
}

int32_t  MersenneRNG::generateNextInt32() {
	double next = nextUniform();
	if(next > 0.5) 
		next = next * -0.5;
	next = next * 2;

	return (int32_t) (next * MERSENNE_INT32_MAX);
}
