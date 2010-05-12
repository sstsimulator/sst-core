
#include <mersenne.h>

double genRandomProbability()
{
   double r = (double) genrand_real2(); // this function is defined in mersene.h
   return r;
}

void seedRandom(unsigned long seed)
{
   init_genrand(seed);
}
