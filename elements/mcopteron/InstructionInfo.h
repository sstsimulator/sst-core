
#ifndef INSTRUCTIONINFO_H
#define INSTRUCTIONINFO_H

#include <OpteronDefs.h>

//-------------------------------------------------------------------
/// @brief Holds the static information about an instruction type
///
/// This holds the information about how to execute an instruction
/// along with how to do the bookkeeping for it. Objects of this
/// class will be initialized at simulator initialization from a file
/// of instruction info.
//-------------------------------------------------------------------
class InstructionInfo
{
 public:
   enum OperandSize { OPSIZE8 = 1, OPSIZE16 = 2, OPSIZE32 = 4, OPSIZE64 = 8, OPSIZE128 = 16 };
   enum DataDirection {
      IREG2IREG = 1, IREG2MEM = 2, IREG2FREG = 4,
      FREG2FREG = 8, FREG2MEM = 16, FREG2IREG = 32,
      MEM2IREG = 64, MEM2MEM = 128, MEM2FREG = 256 };
   
   InstructionInfo();
   ~InstructionInfo();
   void dumpDebugInfo();
   void initStaticInfo(char *name, char *operands, char *operation, char *decodeUnit, 
                       char *execUnits, char *category);
   void initTimings(unsigned int baseLatency, unsigned int memLatency,
                    unsigned int throughputNum, unsigned int throughputDem);
   void initProbabilities(double occurProb, double loadProb, double storeProb,
                          double useHistogram[]);
   void accumProbabilities(double occurProb, unsigned long long occurs,
                           unsigned long long loads, unsigned long long stores,
                           double useHistogram[]);
   InstructionInfo* findInstructionRecord(const char *mnemonic, unsigned int iOpSize);
   unsigned int getUseDistance(double prob);
   bool isConditionalJump() {return conditionalJump;}
   char* getName() {return name;}
   InstructionInfo* getNext() {return next;}
   void setNext(InstructionInfo *other) {next = other;}
   Category getCategory() {return category;}
   unsigned int getLatency() {return latency;}
   double getOccurProb() {return occurProbability;}
   double getLoadProb() {return loadProbability;}
   double getStoreProb() {return storeProbability;}
   bool handlesLoad() {return (allowedDataDirs & (MEM2IREG|MEM2MEM|MEM2FREG)) != 0;}
   bool handlesStore() {return (allowedDataDirs & (IREG2MEM|FREG2MEM|MEM2MEM)) != 0;}
   bool needsLoadAddress() {return (!stackOp || loadProbability < 0.99);}
   bool needsStoreAddress() {return (!stackOp || storeProbability < 0.99);}
   bool needsFunctionalUnit(FunctionalUnitTypes fut) { return (execUnitMask & fut);}
   bool isFPUInstruction() {return execUnitMask & (FADD|FMUL|FSTORE);}
   unsigned long long getSimulationCount() {return actualOccurs;}
   void incSimulationCount() {actualOccurs++;}
   unsigned int throughput() {return throughputDem;}
   static InstructionInfo* createFromString(char* infoString);
 private:
   char* operands;   ///< Number of operands needed
   char* operation;  ///< Operation??
   char* decodeUnit; ///< Which decode unit needed?
   char* execUnits;  ///< Execution units that this insn type can use
                     // optional paths? how do we encode and/or/seq?
   Category category;       ///< High level instruction type category
   double occurProbability; ///< Probability of occurrence 
   double loadProbability;  ///< Probability this insn type does a load
   double storeProbability; ///< Probability this insn type does a store
   char *name;              ///< Instruction type name
   unsigned long long actualOccurs; ///< actual # of occurs in simulation
   unsigned long execUnitMask; ///< Bit mask that encodes FU usage
   unsigned int latency;    ///< Instruction type latency
   unsigned int throughputDem; ///< Throughput (HOW TO USE THIS???)
   bool stackOp;
   bool conditionalJump;

   unsigned int opSize; ///< OR'd bits of OperandSize (8/16/32/64)
   unsigned int allowedDataDirs; ///< OR'd bits of DataDirection types
   unsigned long long totalOccurs; ///< accumulated total occurs from i-mix file
   unsigned int memLatency; ///< Memory latency (not used??)
   unsigned int throughputNum; ///< Throughput (HOW TO USE THIS???)
   double toUseHistogram[HISTOGRAMSIZE]; ///< Dependence histogram
   class InstructionInfo *next;  ///< List ptr
};

#endif
