
#ifndef TOKEN_H
#define TOKEN_H

#include <InstructionInfo.h>

class FunctionalUnit;

/// @brief Simulated instruction class
///
class Token
{
 public:
   Token(InstructionInfo *type, InstructionCount number,
         CycleCount atCycle, bool isFake);
   ~Token();
   void dumpDebugInfo();
   void setMemoryLoadInfo(Address address, unsigned int numBytes);
   void setMemoryStoreInfo(Address address, unsigned int numBytes);
   void setInDependency(Dependency *dep);
   void setOutDependency(Dependency *dep);
   void setOptionalProb(double p) {optionalProb = p;}
   InstructionInfo* getType() {return type;}
   InstructionCount instructionNumber() {return number;}
   void fixupInstructionInfo();
   bool needsAddressGeneration();
   bool addressIsReady();
   bool needsFunctionalUnit(FunctionalUnit *fu);
   bool aguOperandsReady(CycleCount atCycle);
   bool allOperandsReady(CycleCount atCycle);
   bool isExecuting(CycleCount currentCycle);
   bool isCompleted(CycleCount currentCycle);
   bool isLoad() {return hasLoad;}
   bool isStore() {return hasStore;}
   bool isFake() {return fake;}
   bool wasRetired() {return retired;}
   bool wasCanceled() {return canceled;}
   CycleCount issuedAt() {return issueCycle;}
   void setBranchMispredict() {wasMispredicted = true;}
   void executionStart(CycleCount currentCycle);
   void executionContinue(CycleCount currentCycle);
   void loadSatisfiedAt(CycleCount atCycle);
   void storeSatisfiedAt(CycleCount atCycle);
   bool isMispredictedJump();
   void retireInstruction(CycleCount atCycle);
   void cancelInstruction(CycleCount atCycle);
 private:
   InstructionInfo *type;    ///< pointer to instruction info
   double optionalProb;      ///< option probability for sim to use
   InstructionCount number;  ///< issue number of this instruction
   CycleCount issueCycle;    ///< cycle at which issued
   CycleCount retiredCycle;  ///< cycle at which retired (will be computed)
   CycleCount currentCycle;  ///< current cycle in instruction's progress
   FunctionalUnit *atUnit;   ///< Ptr to unit this instruction is at
   CycleCount execStartCycle; ///< cycle of start of func unit use
   CycleCount execEndCycle;  ///< cycle of end of func unit use
   bool fake;
   bool canceled;         ///< True if was canceled
   bool retired;             ///< True if was retired
   bool loadSatisfied;
   bool hasAddressOperand;   ///< True if insn needs address generated
   bool addressGenerated;    ///< True if address has already been generated
   bool hasLoad;             ///< True if insn does a memory load
   bool hasStore;            ///< True if insn does a memory store
   bool completed;           ///< True if instruction has finished
   Dependency *inDependency;  ///< record for input dependencies
   Dependency *outDependency; ///< record for output dependency
   bool wasMispredicted;     ///< True if this is a branch and it was mispredicted
};

#endif
