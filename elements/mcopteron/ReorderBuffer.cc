
#include <ReorderBuffer.h>
#include <Token.h>
#include <stdio.h>

/// @brief Constructor: create and clear data
/// @param numSlots is the total number of instruction slots in the buffer
/// @param numRetireablePerCycle is the maximum # of instructions retireable per cycle
///
ReorderBuffer::ReorderBuffer(unsigned int numSlots, unsigned int numRetireablePerCycle,
                             bool fake)
{
   this->numSlots = numSlots;
   numPerCycle = numRetireablePerCycle;
   tokenBuffer = new Token*[numSlots];
   numTokens = 0;
   availSlot = 0;
   retireSlot = 0;
   totalRetired = totalAnulled = 0;
   fullStalls = 0;
   this->fake = fake;
   for (unsigned int i=0; i < numSlots; i++)
      tokenBuffer[i] = 0;
}

/// @brief Destructor: report stats
///
ReorderBuffer::~ReorderBuffer()
{
   delete[] tokenBuffer;
   const char *qual = "";
   if (fake) qual = "Fake";
   fprintf(outputFP, "%sROB: Total instructions retired: %llu\n", qual, totalRetired);
   fprintf(outputFP, "%sROB: Total instructions anulled: %llu\n", qual, totalAnulled);
   fprintf(outputFP, "%sROB: full RO buffer stalls: %llu\n", qual, fullStalls);
}

/// @brief Dispatch an instruction to the reorder buffer
///
/// This assumes that isFull() was already checked; it will
/// silently fail to add the instruction if the buffer is
/// full.
/// @param token is the instruction to add to the buffer
/// @param atCycle is the cycle count at which it is being added
///
bool ReorderBuffer::dispatch(Token *token, CycleCount atCycle)
{
   if (numTokens >= numSlots) {
      fprintf(debugLogFP, "ROB: Error dispatching token %llu on cycle %llu, buffer full!\n",
              token->instructionNumber(), atCycle);
      return false;
   }
   if (Debug>1) fprintf(debugLogFP, "ROB: dispatching token %llu (%p) into slot %u\n", 
               token->instructionNumber(), token, availSlot);
   tokenBuffer[availSlot] = token;
   availSlot = (availSlot + 1) % numSlots;
   numTokens++;
   return true;
}

/// @brief True if buffer is currently full
///
bool ReorderBuffer::isFull()
{
   if (numTokens >= numSlots)
      return true;
   else
      return false;
}

/// @brief Update: retire instructions in order, or cancel instructions
/// 
/// This retires instructions in order, only up to numPerCycle per cycle,
/// and if it hits a mispredicted branch instruction, it cancels all
/// instructions behind it. The counter 'retireSlot' is an index that is
/// always pointing to the next instruction in-order that should retire.
/// @param currentCycle is the current cycle count
///
int ReorderBuffer::updateStatus(CycleCount currentCycle)
{
   unsigned int i;
   for (i=0; i < numPerCycle; i++) {
      if (tokenBuffer[retireSlot] && tokenBuffer[retireSlot]->isCompleted(currentCycle)) {
         if (Debug>1) fprintf(debugLogFP, "ROB: retiring instruction %llu (%p) in slot %u\n",
                              tokenBuffer[retireSlot]->instructionNumber(), 
                              tokenBuffer[retireSlot], retireSlot);
         tokenBuffer[retireSlot]->retireInstruction(currentCycle);
         tokenBuffer[retireSlot] = 0;
         totalRetired++;
         numTokens--;
         retireSlot = (retireSlot+1) % numSlots;
      } else {
         // as soon as we can't retire an insn, we stop the loop
         break;
      }
   }
   // if insn we stopped at is a mispredicted branch, then flush the buffer
   // TODO: WAIT, shouldn't this happen when the instruction is completed??
   if (tokenBuffer[retireSlot] && tokenBuffer[retireSlot]->isMispredictedJump()) {
      unsigned int nt = numTokens;
      for (i=0; i < nt; i++) {
         if (Debug>1) fprintf(debugLogFP, "ROB: canceling instruction %llu in slot %u\n",
                              tokenBuffer[retireSlot]->instructionNumber(), retireSlot);
         tokenBuffer[retireSlot]->cancelInstruction(currentCycle);
         tokenBuffer[retireSlot] = 0;
         totalAnulled++;
         numTokens--;
         retireSlot = (retireSlot+1) % numSlots;
      }
      return 1;
   } else {
      return 0;
   }
}
