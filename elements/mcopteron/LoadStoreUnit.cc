
#include <LoadStoreUnit.h>
#include <MemoryModel.h>
#include <Token.h>
#include <stdio.h>

/// @brief Constructor: create slots and initialize as empty
/// @param numSlots is the total number of instruction slots
/// @param maxOpsPerCycle is the total number of memory ops allowed per cycle
/// @param memModel is a ptr to the memory model object, used to perform memops
///
LoadStoreUnit::LoadStoreUnit(unsigned int numSlots, unsigned int maxOpsPerCycle,
                             MemoryModel *memModel, OffCpuIF *extIF)
{
   unsigned int i;
   slots = new LSSlot[numSlots];
   this->numSlots = numSlots;
   numFilled = 0;
   fullStalls = 0;
   for (i=0; i < numSlots; i++) {
      slots[i].type = EMPTY;
      slots[i].token = 0;
      slots[i].satisfiedCycle = 0;
   }
   memoryModel = memModel;
   externalIF = extIF;
   maxMemOpsPerCycle = maxOpsPerCycle;
}

/// @brief Destructor: delete slot array
///
LoadStoreUnit::~LoadStoreUnit()
{
   fprintf(outputFP, "LSQ: stalls from full: %llu\n", fullStalls);
   delete[] slots;
}

/// @brief Add a load or store token to the LSQ
/// @param token is the instruction token to add
/// @param atCycle is the cycle count at which it is being added
/// @return 0 if instruction could not be added, 1 on success
///
unsigned int LoadStoreUnit::add(Token *token, CycleCount atCycle)
{
   if (!token->isLoad() && !token->isStore()) {
      // shouldn't be here since this token doesn't do a memop, 
      // but fake success anyways
      fprintf(debugLogFP, "LSQ: trying to add a non memop!\n");
      return 1;
   }
   if (token->isLoad() && token->isStore() && numFilled > numSlots-2) {
      // need at least two slots for a load and store, so fail
      fullStalls++;
      return 0;
   }
   if (numFilled > numSlots-1) {
      // need at least one slot for a load or store, so fail
      fullStalls++;
      return 0;
   }
   if (token->isLoad())
      addLoad(token, atCycle);
   if (token->isStore())
      addStore(token, atCycle);
   return 1;
}


/// @brief Internal add-load token to LSQ
/// @param token is the instruction token to add
/// @param atCycle is the cycle count at which it is being added
/// @return the slot ID (starting at 1), not used anywhere
///
unsigned int LoadStoreUnit::addLoad(Token *token, CycleCount atCycle)
{
   unsigned int i;
   for (i=0; i < numSlots; i++)
      if (slots[i].token == 0) break;
   if (i >= numSlots) {
      fprintf(debugLogFP, "LSQ: Error trying to add load: queue full!\n");
      return 0;
   }
   slots[i].token = token;
   slots[i].type = LOAD;
   slots[i].startCycle = atCycle;
   slots[i].satisfiedCycle = 0;
   numFilled++;
   if (Debug>1)
      fprintf(debugLogFP, "LSQ: load token %llu added at %llu\n",
                    slots[i].token->instructionNumber(), slots[i].startCycle);
   // TODO: check if a load is satisfied by a store in LSQ
   return i+1; // avoid ID of 0
}

/// @brief Internal add-store token to LSQ
/// @param token is the instruction token to add
/// @param atCycle is the cycle count at which it is being added
/// @return the slot ID (starting at 1), not used anywhere
///
unsigned int LoadStoreUnit::addStore(Token *token, CycleCount atCycle)
{
   unsigned int i;
   for (i=0; i < numSlots; i++)
      if (slots[i].token == 0) break;
   if (i >= numSlots) {
      fprintf(debugLogFP, "LSQ: Error trying to add store: queue full!\n");
      return 0;
   }
   slots[i].token = token;
   slots[i].type = STORE;
   slots[i].startCycle = atCycle;
   slots[i].satisfiedCycle = 0;
   numFilled++;
   if (Debug>1)
      fprintf(debugLogFP, "LSQ: store token %llu added at %llu\n",
                    slots[i].token->instructionNumber(), slots[i].startCycle);
   return i+1; // avoid ID of 0
}


/// @brief Update LSQ status (called each cycle)
///
/// This does two things: 1) cycles through the LSQ and
/// for any memop that has an address newly ready it
/// asks the memory model to serve that memop (just
/// calculate cycles to serve); and 2) it purges memops
/// that have finished; stores just go away quietly but
/// loads have a token callback that indicates the load
/// is satisfied (so that the instruction can continue).
///
/// @param currentCycle is the active cycle 
///
void LoadStoreUnit::updateStatus(CycleCount currentCycle)
{
   unsigned int i;
   unsigned int numOps = 0;
   for (i=0; i < numSlots; i++) {
      if (slots[i].type == EMPTY)
         continue;
      if (slots[i].token && slots[i].token->wasCanceled()) {
         // throw instruction away
         slots[i].type   = EMPTY;
         slots[i].token  = 0;
         numFilled--;
         continue;
      }
      if (slots[i].token)
          // allow token to update status
         slots[i].token->isExecuting(currentCycle);
      if (Debug>2) 
         fprintf(debugLogFP,"LSQ slot %d: token %llu satCyc %llu addrRdy %u\n",
                 i, slots[i].token->instructionNumber(), slots[i].satisfiedCycle,
                 slots[i].token->addressIsReady());
      // following condition is: if this token hasn't yet generated a satisfy
      // cycle (it is still 0) and this token's address is now read and we 
      // haven't already performed the maximum memory ops this cycle, then serve it
      if (slots[i].token && slots[i].satisfiedCycle == 0 && 
          slots[i].token->addressIsReady() && numOps < maxMemOpsPerCycle) {
         if (slots[i].type == LOAD) {
            slots[i].satisfiedCycle = memoryModel->serveLoad(currentCycle,0,0);
            externalIF->memoryAccess(OffCpuIF::AM_READ, 0x1000, 9);
         } else {
            slots[i].satisfiedCycle = memoryModel->serveStore(currentCycle,0,0);
            externalIF->memoryAccess(OffCpuIF::AM_WRITE, 0x4000, 9);
            if (slots[i].satisfiedCycle == 0)
               slots[i].satisfiedCycle = 1; // should never happen
            slots[i].token = 0; // make sure we don't access store token again
         }
         if (Debug>1)
            fprintf(debugLogFP, "LSQ: token %llu will be satisfied at %llu\n",
                    slots[i].token?slots[i].token->instructionNumber():0, 
                    slots[i].satisfiedCycle);
         numOps++;
         //continue;
      }
      // following condition is: this memop has just been satisfied (it set its
      // satisfied cycle previously and the current cycle is now >= it's sat-cycle)
      if (slots[i].satisfiedCycle > 0 && slots[i].satisfiedCycle <= currentCycle) {
         // if token is a load, then notify it with the callback
         if (slots[i].type == LOAD)
            slots[i].token->loadSatisfiedAt(currentCycle);
         // we don't report stores since they might be long gone and token deleted
         slots[i].type   = EMPTY; // clear this record
         slots[i].token  = 0; // clear this record
         numFilled--;
      }
   }
}

