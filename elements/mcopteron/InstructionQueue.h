
#ifndef INSTRUCTIONQUEUE_H
#define INSTRUCTIONQUEUE_H

#include <OpteronDefs.h>

class FunctionalUnit;
class Token;

#define MAXFUNITS 5

//-------------------------------------------------------------------
/// @brief Represents an instruction queue in the CPU
///
/// This class implements the instruction queue concept. One
/// object represents one queue. Each queue has some functional
/// units that it supervises. This class really is the heart of
/// simulating the execution of an instruction, since it assigns
/// instructions to functional units and retires them when they 
/// are done.
//-------------------------------------------------------------------
class InstructionQueue
{
 public:
   enum QType {INT, INTMUL, INTSP, FLOAT};
   InstructionQueue(QType type, char *name, unsigned int id, unsigned int size);
   ~InstructionQueue();
   void setNext(InstructionQueue *other);
   InstructionQueue* getNext();
   int addFunctionalUnit(FunctionalUnit *fu);
   int scheduleInstructions(CycleCount currentCycle);
   bool canHandleInstruction(Token *token);
   int assignInstruction(Token *token, CycleCount atCycle);
   bool isFull() {return (numInstructions >= size);};
   bool isEmpty() {return (numInstructions == 0);};
   void incFullStall() {fullStalls++;}
   void incAlreadyAssignedStall() {assignedStalls++;}
   bool alreadyAssigned(CycleCount currentCycle);
   double averageOccupancy(CycleCount cycles);
 private:
   char *name;         ///< Queue name
   QType type;         ///< Queue type
   unsigned int id;    ///< Unique queue ID
   unsigned int size;  ///< Number of entries in queue
   FunctionalUnit* myUnits[MAXFUNITS];  ///< Functional units this queue manages
   Token **queuedInstructions;    ///< Instructions in queue (of size 'size')
   InstructionCount totalInstructions;   ///< Total instructions this queue processed
   InstructionCount finishedInstructions; ///< Total instructions completed from queue so far
   unsigned long long fullStalls, assignedStalls;
   unsigned long long occupancyXCycles, totalCycles;
   unsigned int numInstructions;  ///< Number of instructions ???
   CycleCount lastAssignedCycle;  ///< Cycle that last instruction assignment occurred
   class InstructionQueue *next;  ///< Linked list ptr
};

#endif
