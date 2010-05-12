
#include <stdio.h>
#include <InstructionQueue.h>
#include <FunctionalUnit.h>
#include <Token.h>
#include <stdlib.h>

/// @brief Constructor
///
InstructionQueue::InstructionQueue(QType type, char *name,
                                   unsigned int id, unsigned int size)
{
   unsigned int i;
   this->type = type;
   this->name = name;
   this->id = id;
   this->next = 0;
   this->size = size;
   queuedInstructions = new Token*[size];
   for (i=0; i < size; i++)
      queuedInstructions[i] = 0;
   for (i=0; i < MAXFUNITS; i++)
      myUnits[i] = 0;
   totalInstructions = finishedInstructions = 0;
   numInstructions = 0;
   fullStalls = assignedStalls = 0;
   occupancyXCycles = totalCycles = 0;
}


/// @brief Destructor
///
InstructionQueue::~InstructionQueue()
{
   fprintf(outputFP, "IQ%d %s: total   instructions: %llu\n", id, name,
           totalInstructions);
   fprintf(outputFP, "IQ%d %s: finished instructions: %llu\n", id, name,
           finishedInstructions);
   fprintf(outputFP, "IQ%d %s: average occupancy: %g\n", id, name,
           (double) occupancyXCycles / totalCycles);
   fprintf(outputFP, "IQ%d %s: full     stalls: %llu\n", id, name, fullStalls);
   fprintf(outputFP, "IQ%d %s: assigned stalls: %llu\n", id, name, assignedStalls);
   delete[] queuedInstructions;
}


/// @brief Create list link
///
void InstructionQueue::setNext(InstructionQueue *other)
{
   next = other;
}


/// @brief Retrieve list link
///
InstructionQueue* InstructionQueue::getNext()
{
   return next;
}


/// @brief Attach a functional unit to this queue
///
/// Functional units should only belong to one queue; this
/// attaches a unit to the queue it is called on.
///
int InstructionQueue::addFunctionalUnit(FunctionalUnit *fu)
{
   int i;
   for (i=0; i < MAXFUNITS; i++)
      if (myUnits[i] == 0)
         break;
   if (i >= MAXFUNITS) {
      fprintf(debugLogFP, "IQ%d %s: Error, can't handle any more FUs!\n", id, name);
      return -1;
   }
   myUnits[i] = fu;
   if (Debug) fprintf(debugLogFP, "IQ%d %s: added functional unit at %d\n", id, name, i);
   return 0;
}


/// @brief Schedule instructions onto functional units
///
/// This is the main functionality of a queue; it iterates 
/// through the instructions in the queue and assigns them to
/// functional units if they are available and can be used.
///
int InstructionQueue::scheduleInstructions(CycleCount currentCycle)
{
   unsigned int f, i;
   FunctionalUnit *fu;
   Token *t; unsigned int stuckCount = 0;
   //
   occupancyXCycles += numInstructions;
   totalCycles++;
   // LOTS of stuff here!!
   if (Debug>1) fprintf(debugLogFP, "IQ%d %s: scheduling instructions\n", id, name);
   for (f=0; f < MAXFUNITS; f++) {
      if (myUnits[f] == 0)
         continue; // no unit here, so skip index
      if (!myUnits[f]->isAvailable(currentCycle))
         continue; // unit is still busy, so skip
      // now we have an available unit, lets try to 
      // find an instruction that can use it
      fu = myUnits[f];
      for (i=0; i < size; i++) {
         t = queuedInstructions[i];
         if (!t) // empty slot
            continue;
         // debugging check to see if we are leaving tokens stranded
         if (currentCycle > 3000 && t->issuedAt() < currentCycle - 3000) {
            fprintf(debugLogFP, "IQ%d %s: Token likely stuck!\n", id, name);
            t->dumpDebugInfo();
            t->getType()->dumpDebugInfo();
            stuckCount++;
            if (stuckCount > 2) exit(0);
         }
         if (t->isExecuting(currentCycle))
            // skip already executing token
            continue;
         if (t->wasRetired() || t->wasCanceled()) {
            if (Debug>1) fprintf(debugLogFP, "IQ%d %s: retiring %llu %p\n", id, name,
                       t->instructionNumber(), t);
            queuedInstructions[i] = 0;  // forget pointer
            numInstructions--;          // remove from queue count
            finishedInstructions++;     // add to retired count
            delete t;
            continue;
            // TODO: only allow retire of one insn per cycle???
         }
         if (t->isCompleted(currentCycle))
            // insn is just waiting to be retired
            continue;
         // first check to see if insn needs to use an AGU for address
         // generation (what about FP insns that need addressing?)
         if (t->aguOperandsReady(currentCycle) && t->needsAddressGeneration() && 
             ((fu->getType() == AGU && fu->isAvailable(currentCycle)))) {
            //  || type == FLOAT)) {
            // assign AGU to token
            // token must keep place in queue since it will do another
            // op too (probably)
            if (Debug>1) fprintf(debugLogFP, "IQ%d %s: schedule %llu (%s) to AGU\n", id, name,
                       t->instructionNumber(), t->getType()->getName());
            // JEC: try just not occupying anything for FLOAT queue address gen
            // TODO: Of course we need to occupy an AGU, but we were occupying the
            // ADDER and this is bad; 
            // if (fu->getType() == AGU)
               fu->occupy(currentCycle, AGU_LATENCY);
            t->executionStart(currentCycle);
            continue;
         } 
         // now check to see if insn can use current FU and if all is ready
         else if (t->allOperandsReady(currentCycle) && 
                  t->needsFunctionalUnit(fu) && fu->isAvailable(currentCycle)) {
            // assign other FU to token
            if (Debug>1) fprintf(debugLogFP, "IQ%d %s: schedule %llu (%s) to other\n", id, name,
                       t->instructionNumber(), t->getType()->getName());
            // occupy functional unit; we use the throughput denominator as the amount
            // of time to occupy the unit. Even though the instruction will take longer
            // to finish, the functional units are pipelined and the throughput determines
            // how fast instructions can issue, which our unit occuptation is simulating
            fu->occupy(currentCycle, t->getType()->throughput());
            t->executionStart(currentCycle);
            continue;
         }
         // TODO: check to see if insn needs to fire off a memory load
         // and then let it wait for load
      }
   }
   // repack queue so older insns stay at top
   for (f=0; f < 4; f++) {
      for (i=1; i < size; i++) {
         if (queuedInstructions[i-1] == 0) {
            queuedInstructions[i-1] = queuedInstructions[i];
            queuedInstructions[i] = 0;
         }
      }
   }
   return 0;
}


/// @brief Check if instruction can be assigned to this queue
///
bool InstructionQueue::canHandleInstruction(Token *token)
{
   if (numInstructions >= size) // full queue
      return false; 
   // easy int instructions can use any int queue
   if ((type == INT || type == INTMUL || type == INTSP) &&
       token->getType()->getCategory() == GENERICINT)
       return true;
   // int multiplies need the intmul queue
   else if (type == INTMUL && token->getType()->getCategory() == MULTINT)
       return true;
   // int multiplies need the intmul queue
   else if (type == INTSP && token->getType()->getCategory() == SPECIALINT)
       return true;
   // float instructions need the float queue
   else if (type == FLOAT && token->getType()->getCategory() == ::FLOAT)
       return true;
   else
      return false;
}


/// @brief Check if instruction was already assigned this cycle
///
/// Only one instruction can be placed on a queue per cycle, so
/// it it already happened on this queue we need to block others.
///
bool InstructionQueue::alreadyAssigned(CycleCount currentCycle)
{
   if (lastAssignedCycle == currentCycle)
      return true;
   else
      return false;
}


/// @brief Assign an instruction to this queue
///
/// This assumes canHandleInstruction() has already been called and
/// it has been verified that the instruction can be placed on this queue
///
int InstructionQueue::assignInstruction(Token *token, CycleCount atCycle)
{
   unsigned int i;
   for (i=0; i < size; i++)
      if (!queuedInstructions[i])
         break;
   if (i >= size) {
      fprintf(debugLogFP, "IQ%d %s: Error assigning, queue full!\n", id, name);
      return -1;
   }
   queuedInstructions[i] = token;
   numInstructions++;
   totalInstructions++;
   lastAssignedCycle = atCycle;
   if (Debug>1) fprintf(debugLogFP, "IQ%d %s: assign insn at %u total %u\n",
                      id, name, i, numInstructions);
   return 0;
}


/// @brief Return average occupancy of this queue
///
double InstructionQueue::averageOccupancy(CycleCount cycles)
{
   return (double) totalInstructions / cycles;
}

