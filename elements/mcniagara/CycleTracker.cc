
#include <CycleTracker.h>

// These must match enum definitions exactly or else the
// names will be meaningless
const char* CycleTracker::cycleReasonNames[NUMCYCLEREASONS+1] = {
   "CPI-inh", "I Cache", "L1 Cache", "L2 Cache", "Memory", 
   "Int Dep", "Int-Use Dep", "Int-DSU Dep", 
   "FGU Dep", "Branch MisP", "Branch Stall", 
   "Pipe Flush", "STB Full", "Special Loads", 
   "Ld STB", "TLB Miss", "ITLB Miss", 0 };

CycleTracker::CycleTracker()
{
   int i;
   categoryCycles = new CycleCount[NUMCYCLEREASONS+1];
   categoryCount = new unsigned long long[NUMCYCLEREASONS+1];
   for (i = 0; i <= NUMCYCLEREASONS; i++) {
      categoryCycles[i] = 0;
      categoryCount[i] = 0;
   }
}

CycleTracker::~CycleTracker()
{
   delete[] categoryCycles;
   delete[] categoryCount;
}

void CycleTracker::accountForCycles(CycleCount cycles, CycleReason reason)
{
   categoryCycles[reason] += cycles;
   categoryCount[reason]++;
   totalCycles += cycles;
}

CycleCount CycleTracker::currentCycles()
{
   return totalCycles;
}

CycleCount CycleTracker::cyclesForCategory(CycleReason reason)
{
   return categoryCycles[reason];
}

double CycleTracker::cyclePercentForCategory(CycleReason reason)
{
   return (double) categoryCycles[reason] * 100.0 / totalCycles;
}

unsigned long long CycleTracker::eventCountForCategory(CycleReason reason)
{
   return categoryCount[reason];
}

const char* CycleTracker::categoryName(CycleReason reason)
{
   return cycleReasonNames[reason];
}
