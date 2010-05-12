
#ifndef MCSIMDEFS_H
#define MCSIMDEFS_H

typedef unsigned long long InstructionNumber;
//typedef unsigned long long CycleCount;
typedef double CycleCount;
typedef unsigned long long Address;


// Stall reasons
/***
enum StallReason {
   CPII, I_CACHE, L1_CACHE, L2_CACHE, MEMORY, 
   INT_DEP, INT_USE_DEP, INT_DSU_DEP, 
   FGU_DEP, BRANCH_MP, BRANCH_ST,
   P_FLUSH, STB_FULL, SPCL_LOAD, LD_STB,
   TLB_MISS, ITLB_MISS, NUMSTALLREASONS
};
***/

double my_rand(void);

#endif
