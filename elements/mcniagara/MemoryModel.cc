
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <MemoryModel.h>

static bool Debug = false;

/// @brief Memory model constructor
///
/// This routine just zero's out fields. Use the init methods to 
/// populate the object with real data 
///
MemoryModel::MemoryModel()
{
   memQHead = memQTail = 0;
   numLoadsInQ = numStoresInQ = 0;
   pSTBHit = pL1Hit = pL2Hit = pTLBMiss = 0.0;
   pICHit = pIL2Hit = pITLBMiss = 0.0;
   numL1Hits = numL2Hits = numMemoryHits = numTLBMisses = 0;
   numICHits = numIL2Hits = numIMemoryHits = numITLBMisses = 0;
   numSTBHits = numStores = numLoads = 0;
}


/// @brief Memory model destructor
///
/// This deletes any outstanding memory ops in the queue
///
MemoryModel::~MemoryModel()
{
   MemoryOp *p;
   while (memQHead) {
      p = memQHead->next;
      delete memQHead;
      memQHead = p;
   }
}

/// @brief Initialize memory hierarchy latencies
///
/// @param latTLB is the TLB latency in cycles
/// @param latL1 is the L1 cache latency in cycles
/// @param latL2 is the L2 cache latency in cycles
/// @param latMem is the memory access latency in cycles
///
void MemoryModel::initLatencies(unsigned int latTLB, unsigned int latL1, 
                                unsigned int latL2, unsigned int latMem)
{
   latencyTLB = latTLB;
   latencyL1 = latL1;
   latencyL2 = latL2;
   latencyMem = latMem;
   if (Debug) fprintf(stderr, "Latencies: TLB %u L1 %u L2 %u Mem %u\n",
                      latencyTLB, latencyL1, latencyL2, latencyMem);
}

/// @brief Initialize memory probabilities
///
/// Right now, these are assumed to be independent, but really should be a 
/// CDF.
/// @param pSTBHit is the probability a load will be satisfied from the
///        store buffer
/// @param pL1Hit is the probability a load is satisfied in the L1 cache
///        given that it missed the store buffer
/// @param pL2Hit is the probability a load is satisfied in the L2 cache
///        given that it missed the store buffer and the L1 cache
/// @param pTLBMiss is the probability a load or store misses the TLB
/// @param pICHit is the probability an instruction fetch is satisfied in
///        the I-cache
/// @param pIL2Hit is the probability an instruction fetch is satisfied in L2
///        cache, given that it missed the I cache
/// @param pITLBMiss is the probability an instruction fetch misses the TLB
///
void MemoryModel::initProbabilities(double pSTBHit, double pL1Hit, double pL2Hit,
                                    double pTLBMiss, double pICHit, double pIL2Hit,
                                    double pITLBMiss)
{
   this->pSTBHit = pSTBHit;
   this->pL1Hit = pL1Hit;
   this->pL2Hit = pL2Hit;
   this->pTLBMiss = pTLBMiss;
   this->pICHit = pICHit;
   this->pIL2Hit = pIL2Hit;
   this->pITLBMiss = pITLBMiss;
   if (Debug) fprintf(stderr, "Data hit %%: STB %g L1 %g L2 %g\n",
                      pSTBHit, pL1Hit, pL2Hit);
   if (Debug) fprintf(stderr, "Inst hit %%: IC %g L2 %g\n", pICHit, pIL2Hit);
   if (Debug) fprintf(stderr, "TLB Miss %%: DTLB %g ITLB %g\n", pTLBMiss,
                      pITLBMiss);
}

/// @brief Compute cost of serving a data load
///
/// This computes the cycle at which a load will be satisfied.
/// @param currentCycle is the current cycle count when load issued
/// @param address is the address this load is accessing (not used yet)
/// @param numbytes is the number of bytes being read (not used yet)
/// @param reason is an out-parameter giving the reason for the load delay
/// @return the cycle count at which the load will be satisfied
///
CycleCount MemoryModel::serveLoad(CycleCount currentCycle, Address address,
                                  unsigned int numBytes, CycleTracker::CycleReason *reason)
{
   CycleCount satisfiedCycle = currentCycle;

   numLoads++;
   purgeMemoryQ(currentCycle); // update mem Q (removes old loads/stores)

   // check if an existing load is not yet satisfied
   if (lastLoad && lastLoad->satisfiedCycle > satisfiedCycle) {
      // stall until current load is done
      satisfiedCycle = lastLoad->satisfiedCycle + Cost::LoadAfterLoad;
   }

   // All memops might suffer a TLB miss, so adjust if this happens
   // -- JEC: this should have a separate cpi accounting category
   if (my_rand() <= pTLBMiss) {
      numTLBMisses++;
      satisfiedCycle += latencyTLB;
   }

   // Now step through memory hierarchy (including store buffer)
   if (my_rand() <= pSTBHit) { 
      // Load is satisfied from store buffer
      *reason = CycleTracker::LD_STB;
      numSTBHits++;
      satisfiedCycle += Cost::LoadFromSTB; 
   } else if (my_rand() <= pL1Hit) { 
      // Load is satisfied in L1 cache
      *reason = CycleTracker::L1_CACHE;
      numL1Hits++;
      satisfiedCycle += latencyL1;
   } else if (my_rand() <= pL2Hit) {
      // load satisfied in L2
      *reason = CycleTracker::L2_CACHE;
      numL2Hits++;
      satisfiedCycle += latencyL2;
   } else { 
      // load satisfied in Memory
      *reason = CycleTracker::MEMORY;
      numMemoryHits++;
      satisfiedCycle += latencyMem;
   }
   // add this load to the mem Q
   addToMemoryQ(satisfiedCycle, MEMLOAD);
   return satisfiedCycle;
}

/// @brief Compute cost of serving an instruction fetch load
///
/// This computes the cycle at which an instruction fetch will be satisfied
/// @param currentCycle is the current cycle count when fetch issued
/// @param address is the address this fetch is accessing (not used yet)
/// @param numbytes is the number of bytes being read (not used yet)
/// @param reason is an out-parameter giving the reason for the load delay
/// @return the cycle count at which the load will be satisfied
///
CycleCount MemoryModel::serveILoad(CycleCount currentCycle, Address address,
                                   unsigned int numBytes, CycleTracker::CycleReason *reason)
{
   CycleCount satisfiedCycle = currentCycle;

   numILoads++;
   
   // JEC: Instruction loads should check for conflicting loads
   //      from L2 on up, since they share resources
   purgeMemoryQ(currentCycle); // update mem Q (removes old loads/stores)

   // All memops might suffer a TLB miss, so adjust if this happens
   if (my_rand() <= pITLBMiss) {
      numITLBMisses++;
      satisfiedCycle += latencyTLB;
   }

   // Now step through memory hierarchy
   if (my_rand() <= pICHit) { 
      // Load is satisfied in I cache
      *reason = CycleTracker::I_CACHE;
      numICHits++;
      //satisfiedCycle += 0;  // no cost to hit i-cache
   } else {
      // Will hit L2 or memory, so handle conflicts
      // check if an existing load is not yet satisfied
      if (lastLoad && lastLoad->satisfiedCycle > satisfiedCycle) {
         // stall until current load is done
         satisfiedCycle = lastLoad->satisfiedCycle + Cost::LoadAfterLoad;
      }
      if (my_rand() <= pIL2Hit) {
         // load satisfied in L2
         *reason = CycleTracker::I_CACHE;
         numIL2Hits++;
         satisfiedCycle += latencyL2;
      } else {
         // load satisfied in Memory
         *reason = CycleTracker::I_CACHE;
         numIMemoryHits++;
         satisfiedCycle += latencyMem;
      }
      addToMemoryQ(satisfiedCycle, MEMLOAD);
   }

   return satisfiedCycle;
}

/// @brief Compute cost of serving a data store
///
/// This computes the cycle at which a store will be satisfied and
/// how much the store instruction might need to stall.
/// @param currentCycle is the current cycle count when store issued
/// @param address is the address this store is accessing (not used yet)
/// @param numbytes is the number of bytes being written (not used yet)
/// @param reason is an out-parameter giving the reason for the store delay
/// @return the cycle count which the store needs to stall until (different
///         than when the store will be satisfied!)
///
CycleCount MemoryModel::serveStore(CycleCount currentCycle, Address address,
                         unsigned int numBytes, CycleTracker::CycleReason *reason)
{
   // how to compute a store's satisfied cycle? Maybe something better
   // would be to not compute it at all and have sim call a doStore()
   // method when a long instruction is going to give it time. Or if we
   // can come up with a probabilistic distribution, sample that.
   CycleCount satisfiedCycle = currentCycle + Cost::AverageStoreLatency;
   CycleCount stallUntilCycle = currentCycle; // assume can finish now

   if (lastStore && lastStore->satisfiedCycle > satisfiedCycle) 
      satisfiedCycle = lastStore->satisfiedCycle + Cost::StoreAfterStore;

   if (numStoresInQ >= Config::StoreBufferSize) {
      // store buffer is full, must stall until an open slot
      // find first store in Q
      MemoryOp *firstStore = memQHead;
      while (firstStore && firstStore->op != MEMSTORE)
         firstStore = firstStore->next;
      if (!firstStore) assert(0);
      stallUntilCycle = firstStore->satisfiedCycle+1;
      purgeMemoryQ(stallUntilCycle);
   }
   *reason = CycleTracker::STB_FULL;
   addToMemoryQ(satisfiedCycle, MEMSTORE);
   return stallUntilCycle;
}

/// @brief Add a load or store to the current memory op queue
///
/// @param whenSatisfied is the cycle count when this op will finish
/// @param type is either MEMLOAD or MEMSTORE
/// @return always 0
///
int MemoryModel::addToMemoryQ(CycleCount whenSatisfied, MemOpType type)
{
   MemoryOp *m;
   m = new MemoryOp();
   m->insnNum = 0;
   m->address = 0;
   m->numBytes = 0;
   m->issueCycle = 0;
   m->satisfiedCycle = whenSatisfied;
   m->op = type;
   m->next = 0;
   if (type == MEMSTORE) {
      lastStore = m;
      numStoresInQ++;
   } else {
      lastLoad = m;
      numLoadsInQ++;
   }
   if (!memQTail) {
      memQHead = memQTail = m;
   } else {
      memQTail->next = m;
      memQTail = m;
   }
   return 0;
}


/// @brief Purge the memory queue up to some certain cycle.
///
/// Remove "old" memory ops from the queue that are satisfied
/// up to the given cycle count.
/// @param upToCycle is the cycle count to purge up to
/// @return always 0.0
///
double MemoryModel::purgeMemoryQ(CycleCount upToCycle)
{
   MemoryOp *p;
   while (memQHead && memQHead->satisfiedCycle <= upToCycle) {
      p = memQHead;
      memQHead = memQHead->next;
      if (p->op == MEMLOAD)
         numLoadsInQ--;
      else
         numStoresInQ--;
      if (lastLoad == p) lastLoad = 0;
      if (lastStore == p) lastStore = 0;
      delete p;
   }
   if (!memQHead) memQTail = 0; // list is empty, clear tail ptr too
   
   if (numLoadsInQ + numStoresInQ > 10000) {
      fprintf(stderr, "Panic: too many ops in memq: %u %u\n",
              numLoadsInQ, numStoresInQ);
      exit(0);
   }
   return 0.0;
}

/// @brief return number of outstand ops in queue
///
/// @param memOp is either MEMLOAD or MEMSTORE
///
unsigned int MemoryModel::numberInMemoryQ(MemOpType memOp)
{
   if (memOp == MEMLOAD)
      return numLoadsInQ;
   else if (memOp == MEMSTORE)
      return numStoresInQ;
   return 0;
}

/// @brief Get data load operation statistics
///
void MemoryModel::getDataLoadStats(unsigned long long *numLoads,
                         unsigned long long *numSTBHits,
                         unsigned long long *numL1Hits,
                         unsigned long long *numL2Hits,
                         unsigned long long *numMemoryHits,
                         unsigned long long *numTLBMisses)
{
   *numLoads = this->numLoads;
   *numSTBHits = this->numSTBHits;
   *numL1Hits = this->numL1Hits;
   *numL2Hits = this->numL2Hits;
   *numMemoryHits = this->numMemoryHits;
   *numTLBMisses = this->numTLBMisses;
}

/// @brief Get instruction load statistics
///
void MemoryModel::getInstLoadStats(unsigned long long *numILoads,
                         unsigned long long *numICHits,
                         unsigned long long *numIL2Hits,
                         unsigned long long *numIMemoryHits,
                         unsigned long long *numITLBMisses)
{
   *numILoads = this->numILoads;
   *numICHits = this->numICHits;
   *numIL2Hits = this->numIL2Hits;
   *numIMemoryHits = this->numIMemoryHits;
   *numITLBMisses = this->numITLBMisses;
}

/// @brief Get store operation statistics
///
void MemoryModel::getStoreStats(unsigned long long *numStores )
{
   *numStores = this->numStores;
}

