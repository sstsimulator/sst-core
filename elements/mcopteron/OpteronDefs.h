
#ifndef OPTERONDEFS_H
#define OPTERONDEFS_H

#include <stdio.h>
#ifdef MEMDEBUG
#include <duma.h>
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

/// High level instruction categories
enum Category { UNKNOWN, GENERICINT, SPECIALINT, MULTINT, FLOAT };

/// Functional Unit Designators
enum FunctionalUnitTypes { AGU = 1, ALU0 = 2, ALU1 = 4, ALU2 = 8,
                           FADD = 16, FMUL = 32, FSTORE = 64 };
// Access byte bitmask for up to four instruction steps
#define STEP1(v) ((v)&0xff)
#define STEP2(v) (((v)>>8)&0xff)
#define STEP3(v) (((v)>>16)&0xff)
#define STEP4(v) (((v)>>24)&0xff)

#define HISTOGRAMSIZE 64
#define MAXCANASSIGN 3  ///< Opteron decode path allows 3 insns/cycle
#define AGU_LATENCY 1   ///< # of cycles to generate address in AGU

typedef unsigned long long InstructionCount;
typedef unsigned long long CycleCount;
typedef unsigned long long Address;

/// Records of inter-instruction data dependencies
struct Dependency
{
   InstructionCount consumer;
   unsigned int numProducers;
   unsigned int numReady;
   bool consumed;
   struct Dependency *next;
};

EXTERN unsigned int Debug;
EXTERN FILE *debugLogFP;
EXTERN FILE *outputFP;

double genRandomProbability();
void seedRandom(unsigned long seed);

#endif
