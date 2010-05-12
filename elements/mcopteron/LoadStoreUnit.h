
#ifndef LOADSTOREUNIT_H
#define LOADSTOREUNIT_H

#include <OpteronDefs.h>
#include <OffCpuIF.h>

class Token;
class MemoryModel;

/// @brief Load Store Unit model
///
/// This class is responsible for simulating the load-store queue.
/// It keeps track of instructions in the LSQ, calls to the memory
/// module to get the cycles needed to serve the instruction's memop
/// once an address is available, and notify's instructions with a load
/// that the load is satisfied. We don't do any actual address tracking
/// so it does not do an store-load forwarding, but relies on the 
/// probabilities given to the memory model to take care of that.
///
class LoadStoreUnit
{
 public:
   LoadStoreUnit(unsigned int numSlots, unsigned int maxOpsPerCycle,
                 MemoryModel *memModel, OffCpuIF *extIF);
   ~LoadStoreUnit();
   unsigned int add(Token *token, CycleCount atCycle);
   void updateStatus(CycleCount currentCycle);
 private:
   unsigned int addLoad(Token *token, CycleCount atCycle);
   unsigned int addStore(Token *token, CycleCount atCycle);
   enum LSType { EMPTY, LOAD, STORE }; // TODO: handle load and store?
   /// Slot record
   struct LSSlot {
      Token *token;
      LSType type;
      CycleCount startCycle;
      CycleCount satisfiedCycle;
   };
   LSSlot *slots;                   ///< Instantiated array of LS slots
   unsigned int numSlots;           ///< Total number of slots
   unsigned int numFilled;          ///< Number of slots currently occupied
   unsigned int maxMemOpsPerCycle;  ///< Max number of memory ops per cycle
   unsigned long long fullStalls;   ///< Statistic: number of stalls due to full buffer
   MemoryModel *memoryModel;        ///< Ptr to memory model object
   OffCpuIF *externalIF;
};

#endif
