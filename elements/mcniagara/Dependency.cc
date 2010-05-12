
#include <Dependency.h>

/// @brief Dependency tracker constructor
///
DependencyTracker::DependencyTracker()
{
   DepQHead = DepQTail = 0;
}

/// @brief Dependency tracker destructor
///
DependencyTracker::~DependencyTracker()
{
   Dependency *d;
   while (DepQHead) {
      d = DepQHead->next;
      delete DepQHead;
      DepQHead = d;
   }
}

/// @brief Add a data dependency
///
/// @param consumerNum is the instruction number who consumes this data dep
/// @param whenSatisfied is the cycle count when this dependency is satisfied
/// @param reason is the accounting reason for this delay (if any)
///
int DependencyTracker::addDependency(InstructionNumber consumerNum, 
                                     CycleCount whenSatisfied, CycleTracker::CycleReason reason)
{
   Dependency *d;
   d = DepQHead;
   while (d) {
      if (d->consumer == consumerNum)
         break;
      d = d->next;
   }
   if (d) {
      // existing record for this insn, just update (only if longer)
      if (d->availableCycle < whenSatisfied)
         d->availableCycle = whenSatisfied;
   } else {
      // new record needed
      d = new Dependency();
      d->producer = 0; // not provided for now
      d->consumer = consumerNum;
      d->availableCycle = whenSatisfied;
      d->reason = reason;
      d->next = 0;
      if (!DepQTail) {
         DepQHead = DepQTail = d;
      } else {
         DepQTail->next = d;
         DepQTail = d;
      }
   }
   return 0;
}

/// @brief Adjust dependency chain (not used)
///
/// This should never be used and should not exist. There is
/// never any reason to adjust existing dependencies.
/// @param numCycles is the number of cycle to adjust by
/// @return always 0
int DependencyTracker::adjustDependenceChain(CycleCount numCycles)
{
   // don't do anything here, this should not exist
   return 0;
}

/// @brief Check if an instruction is dependent on some data
/// 
/// This checks the existing dependencies to see if the given 
/// instruction is dependent. It has a side effect in deleting
/// the dependency record of the given instruction, if found.
///
/// @param instructionNum is the instruction number to check on
/// @param reason is an out-parameter giving the reason for any delay
/// @return the cycle count at which the dependency is satisfied
///
CycleCount DependencyTracker::isDependent(InstructionNumber instructionNum,
                                          CycleTracker::CycleReason *reason)
{
   Dependency *d = DepQHead, *prev = 0;
   CycleCount avail;
   while (d) {
      if (d->consumer == instructionNum)
         break;
      prev = d;
      d = d->next;
   }
   if (!d)
      return 0;
   if (prev)
      prev->next = d->next;
   else
      DepQHead = d->next;
   *reason = d->reason;
   avail = d->availableCycle;
   delete d;
   return avail;
}
