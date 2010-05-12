
#ifndef CYCLETRACKER_H
#define CYCLETRACKER_H

#include <McSimDefs.h>

/// @brief Keeps track of cycles accumulated and the reasons for them
//
//
class CycleTracker
{
 public:
   enum CycleReason { 
      CPII, I_CACHE, L1_CACHE, L2_CACHE, MEMORY, 
      INT_DEP, INT_USE_DEP, INT_DSU_DEP, 
      FGU_DEP, BRANCH_MP, BRANCH_ST,
      P_FLUSH, STB_FULL, SPCL_LOAD, LD_STB,
      TLB_MISS, ITLB_MISS, NUMCYCLEREASONS
   };
   CycleTracker();
   ~CycleTracker();
   void accountForCycles(CycleCount cycles, CycleReason reason);
   CycleCount currentCycles();
   CycleCount cyclesForCategory(CycleReason reason);
   double cyclePercentForCategory(CycleReason reason);
   unsigned long long eventCountForCategory(CycleReason reason);
   const char* categoryName(CycleReason reason);
 private:
   static const char* cycleReasonNames[NUMCYCLEREASONS+1];
   CycleCount totalCycles;
   CycleCount *categoryCycles;
   unsigned long long *categoryCount;
};

#endif
