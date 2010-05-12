
#include <stdio.h>
#include <memory.h>
#include <Token.h>
#include <FunctionalUnit.h>

/// @brief Constructor
///
Token::Token(InstructionInfo *type, InstructionCount number, 
             CycleCount atCycle, bool isFake)
{
   this->number = number;
   issueCycle = atCycle;
   currentCycle = atCycle;
   retiredCycle = 0;
   this->type = type;
   fake = isFake;
   if (!fake)
      this->type->incSimulationCount();
   execStartCycle = atCycle;
   execEndCycle = 0;
   optionalProb = 0.0;
   hasAddressOperand = false; //( number % 2 == 0 );
   hasLoad = hasStore = false;
   loadSatisfied = false;
   addressGenerated = false;
   completed = false;
   retired = false;
   wasMispredicted = false;
   canceled = false;
   atUnit = 0;
   inDependency = outDependency = 0;
}

/// @brief: Destructor
///
Token::~Token()
{
   if (outDependency && completed) {
      fprintf(debugLogFP, "Token %llu being deleted but still has dependency to %llu\n",
              number, outDependency->consumer);
      outDependency->numReady++;
   }
   if (Debug>1) fprintf(debugLogFP, "Tk: destructing %llu at %p\n", number, this);
   memset(this, 0, sizeof(Token));
}

void Token::dumpDebugInfo()
{
   fprintf(debugLogFP, "Tk: number %llu issued %llu indeps %u outdep %llu addrOp %s %s "
           "load %s %s store %s compl/ret/canc %s%s%s\n",
           number, issueCycle, inDependency?inDependency->numProducers:99,
           outDependency?outDependency->consumer:0,
           hasAddressOperand?"T":"F", addressGenerated?"T":"F", 
           hasLoad?"T":"F", loadSatisfied?"T":"F", hasStore?"T":"F", 
           completed?"T":"F", retired?"T":"F", canceled?"T":"F");
}

/// @brief Set memory load operation info
///
void Token::setMemoryLoadInfo(Address address, unsigned int numBytes)
{
   // if stack op and loadProb is 1, then is a pop and doesn't need AGU,
   // otherwise it is a mem op and needs AGU
   if (type->needsLoadAddress())
      hasAddressOperand = true;
   hasLoad = true;
   loadSatisfied = false;
}


/// @brief Set memory store operation info
///
void Token::setMemoryStoreInfo(Address address, unsigned int numBytes)
{
   // if stack op and storeProb is 1, then is a push and doesn't need AGU,
   // otherwise it is a mem op and needs AGU
   if (type->needsStoreAddress())
      hasAddressOperand = true;
   hasStore = true;
}


/// @brief Adjust instruction info record if necessary.
///
/// Once the token has load/store's possibly generated, we
/// might need to point at a different instruction info record,
/// because multiple variants of an instruction are handled 
/// differently.
///
void Token::fixupInstructionInfo()
{
   InstructionInfo *ii = type;
   const char *origName = ii->getName();
   if (hasLoad && !ii->handlesLoad()) {
      ii = ii->getNext();
      while (ii && !strcmp(ii->getName(), origName)) {
         if (ii->handlesLoad()) break;
         ii = ii->getNext();
      }
   } else if (hasStore && !ii->handlesStore()) {
      ii = ii->getNext();
      while (ii && !strcmp(ii->getName(), origName)) {
         if (ii->handlesStore()) break;
         ii = ii->getNext();
      }
   }
   if (ii && ii != type && !strcmp(ii->getName(), origName)) {
      // we've found a different instruction record of the same 
      // instruction name and it supports the necessary data direction,
      // so change the token's record ptr.
      type = ii;
      if (Debug>1) fprintf(debugLogFP,"TOK %llu: switching Inst Infos\n", number);
   }
}


/// @brief Set link to input dependency record
///
void Token::setInDependency(Dependency *dep)
{
   inDependency = dep;
}

/// @brief Set link to output dependency record
///
void Token::setOutDependency(Dependency *dep)
{
   outDependency = dep;
}

/// @brief Check if instruction needs an address generated
///
bool Token::needsAddressGeneration()
{
   if (type->isFPUInstruction() && hasAddressOperand) {
      // rely on fake LEA to indicate address is generated
      // - it will increment the dependency ready count,
      //   this is not quite accurate but should be close
      if (inDependency && inDependency->numReady > 0)
         addressGenerated = true;
   }
   if (hasAddressOperand && !addressGenerated)
      return true;
   else
      return false;
}

/// @brief Check if address is ready for memory op
///
bool Token::addressIsReady()
{
   return !needsAddressGeneration();
}


/// @brief Check if instruction can use functional unit now
///
bool Token::needsFunctionalUnit(FunctionalUnit *fu)
{
   // TODO: compare instruction info and see if it can execute on
   // the FU. this must also consider sequencing, such as ALU after
   // AGU 
   if (hasAddressOperand && !addressGenerated) {
      if (fu->getType() == AGU) // || fu->getType() == FADD)
         return true;
      else
         return false;
   }
   if (type->needsFunctionalUnit(fu->getType()))
      return true;
   else
      return false;
}

/// @brief Check if AGU operands are ready
///
/// TODO: We may need separate use-distance tables for AGU operands and
/// ALU operands, since they can execute independently and are quite 
/// different. For now we assume AGU operands are ready always
///
bool Token::aguOperandsReady(CycleCount atCycle)
{
   return true;
   // dead code below here
/**   
   // check dependencies but not a load
   if (inDependency && inDependency->numProducers != inDependency->numReady) {
      if (Debug>1) 
         fprintf(debugLogFP, "Token %llu still waiting for dependencies %u %u\n",
                 number, inDependency->numProducers, inDependency->numReady);
      return false;
   } else {
      return true;
   }
**/
}

/// @brief Check if all operands are available for instruction
///
bool Token::allOperandsReady(CycleCount atCycle)
{
   if (inDependency && inDependency->numProducers > inDependency->numReady) {
      if (Debug>1) 
         fprintf(debugLogFP, "Token %llu still waiting for dependencies %u %u\n",
                 number, inDependency->numProducers, inDependency->numReady);
      return false;
   } else if (hasLoad && !loadSatisfied) {
      if (Debug>1) 
         fprintf(debugLogFP, "Token %llu still waiting for memory load\n", number);
      return false;
   } else {
      return true;
   }
}


/// @brief Mark the beginning of execution on a functional unit
///
void Token::executionStart(CycleCount currentCycle)
{
   execStartCycle = currentCycle;
   if (hasAddressOperand && !addressGenerated) {
      // assume we are generating an address, finishes in one cycle
      execEndCycle = currentCycle + AGU_LATENCY - 1;
   } else {
      execEndCycle = currentCycle + type->getLatency() - 1;
      if (inDependency) {
         // report that we've consumed our operands
         inDependency->consumed = true;
         inDependency = 0;
      }
   }
   if (Debug>1) fprintf(debugLogFP, "Token %llu is starting at %llu till %llu\n",
              number, currentCycle, execEndCycle);
}

void Token::loadSatisfiedAt(CycleCount atCycle)
{
   loadSatisfied = true;
}

void Token::storeSatisfiedAt(CycleCount atCycle)
{
}


/// @brief True if instruction is executing on functional unit now
///
bool Token::isExecuting(CycleCount currentCycle)
{
   bool needsAddress = needsAddressGeneration(); // side effect for FP insns
   if (execEndCycle == 0)
      return false;
   if (execEndCycle >= currentCycle)
      // still executing
      return true;
   else {
      // has finished some exec step
      if (needsAddress) //hasAddressOperand && !addressGenerated) 
         // we assume the first step must have been to generate an address
         // - maybe we should check the unit (AGU or FADD??)
         addressGenerated = true;
      else {
         completed = true;
         if (Debug>1)
           fprintf(debugLogFP,"Tk %llu: completed dep = %u %u to %llu\n", number,
                   outDependency?outDependency->numProducers:99,
                   outDependency?outDependency->numReady:99,
                   outDependency?outDependency->consumer:0);
         if (outDependency) {
            outDependency->numReady++; // we've produced our value
            outDependency = 0; // make sure we cannot access anymore
         }
      }
      execEndCycle = 0; // clear exec step
      //if (fake) {
      //   retired = true; // reorder buffer doesn't have it, so we do it here
      // }
      return false;
   }
}


/// @brief True if instruction has been completed
///
bool Token::isCompleted(CycleCount currentCycle)
{
   isExecuting(currentCycle); // allow token to check itself for this cycle
   return completed;
}

bool Token::isMispredictedJump()
{
   return wasMispredicted;
}

/// @brief Mark instruction as retired
///
void Token::retireInstruction(CycleCount atCycle)
{
   completed = true; // should already be set, but...
   retired = true;
   if (Debug>1)
      fprintf(debugLogFP,"Tk %llu: retired dep = %u %u to %llu\n", number,
              outDependency?outDependency->numProducers:99,
              outDependency?outDependency->numReady:99,
              outDependency?outDependency->consumer:0);
   if (outDependency) { // should already be done, but...
      outDependency->numReady++; 
      outDependency = 0;
   }
   if (inDependency) {
      // report that we've consumed our operands
      // should be done already, but...
      inDependency->consumed = true;
      inDependency = 0;
   }
}

/// @brief Mark instruction as canceled
///
void Token::cancelInstruction(CycleCount atCycle)
{
   completed = true;
   canceled = true;
   // if future tokens think they need an operand from
   // us, make sure that it is counted
   if (outDependency) {
      outDependency->numReady++; // we've produced our value
      outDependency = 0; // make sure we cannot access anymore
   }
   if (inDependency) {
      // report that we've consumed our operands
      inDependency->consumed = true;
      inDependency = 0;
   }
}

