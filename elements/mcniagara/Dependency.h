
#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include <McSimDefs.h>
#include <CycleTracker.h>

//-------------------------------------------------------------------
/// @brief Dependency tracker class
///
//-------------------------------------------------------------------
class DependencyTracker
{
   /// Dependency list node type
   struct Dependency
   {
      InstructionNumber producer;
      InstructionNumber consumer;
      CycleCount availableCycle;
      CycleTracker::CycleReason reason;
      struct Dependency *next;
   };

 public:
   DependencyTracker();
   ~DependencyTracker();
   int addDependency(InstructionNumber instructionNum, CycleCount whenSatisfied,
                     CycleTracker::CycleReason reason);
   int adjustDependenceChain(CycleCount numCycles);
   CycleCount isDependent(InstructionNumber instructionNum, CycleTracker::CycleReason *reason);
 private:
   Dependency *DepQHead, ///< Dependency list head pointer
              *DepQTail; ///< Dependency list tail pointer
};

#endif
