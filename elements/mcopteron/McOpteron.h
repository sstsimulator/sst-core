
#ifndef MCOPTERON_H
#define MCOPTERON_H

#include <Token.h>
#include <OffCpuIF.h>

class FunctionalUnit;
class InstructionQueue;
class LoadStoreUnit;
class MemoryModel;
class ReorderBuffer;
class ConfigVars;

///-------------------------------------------------------------------
/// @brief Main Monte Carlo Opteron Simulation Class
///
/// This class drives the entire simulation. It instantiates the CPU model,
/// reads instruction definition and mix information from files, and then 
/// runs the simulation cycle by cycle. The simulation is generally composed
/// of working backwards up the architectural pipeline each cycle, from
/// retiring instructions out of the reorder buffer and load-store queue,
/// to updating the progress of the functional units, to allowing the 
/// reservation queues to send new instructions to the functional units,
/// to fetching and dispatching new instructions to the reservation queues.
/// Instruction fetch can also be done from a trace file. 
///
/// Each instruction is represented by a token object. This object has a 
/// pointer to an InstructionInfo record that holds the data about the type
/// of instruction it is. The token moves to the reservation queues, and
/// the reorder buffer and possibly the load-store queue also hold a reference
/// to it. Once the token recognizes it is completed, the reorder buffer will
/// tell it it is retired, and then the reservation queue can delete it. Token
/// objects are always deleted by the reservation queue that holds it.
///-------------------------------------------------------------------
class McOpteron
{
 public:
   McOpteron();
   ~McOpteron();
   void setOutputFiles(const char* outFilename, const char* debugFilename);
   int init(const char* definitionFilename, const char* mixFilename, 
            const char* cpuIniFilename, const char* appIniFilename, 
            OffCpuIF *extif, const char *traceFilename=0);
   int finish(bool printInstMix);
   int simCycle();
   CycleCount currentCycles();
   double currentCPI();

 private:
   Token* generateToken();
   Token* getNextTraceToken();
   bool allQueuesEmpty();
   int updateFunctionalUnits();
   int scheduleNewInstructions();
   int refillInstructionQueues();
   Dependency* checkForDependencies(InstructionCount insn);
   Dependency* addNewDependency(InstructionCount insn);
   int readConfigFile(const char* filename);
   int readIMixFile(const char *filename);
   int readIDefFile(const char *filename);
   void createInstructionMixCDF();

   FunctionalUnit *functionalUnitsHead;      ///< Functional units list
   InstructionQueue *instructionQueuesHead;  ///< Instruction queues list
   CycleCount currentCycle;                  ///< Current simulation cycle
   InstructionCount totalInstructions;       ///< Total instructions so far
   double* instructionClassProbabilities;    ///< Instruction type CDF
   InstructionInfo **instructionClasses;     ///< Instruction type ptrs
   InstructionInfo *instructionClassesHead,  ///< Instruction type list
                   *instructionClassesTail;  ///< Instruction type list tail
   unsigned int numInstructionClasses;       ///< Number of instruction types
   InstructionInfo *infoLEA;  ///< direct ptr to LEA insn for fast access
   Dependency *dependenciesHead; ///< inter-insn operand dependency list
   Dependency *dependenciesTail;
   LoadStoreUnit *loadStoreUnit;
   MemoryModel *memoryModel;
   ReorderBuffer *reorderBuffer;
   ReorderBuffer *fakeIBuffer;
   double lastCPI;
   Token *lastToken; ///< Must remember last token generated but not used
   double probBranchMispredict;
   CycleCount nextAvailableFetch;
   unsigned long long fetchStallCycles;
   OffCpuIF *externalIF; ///< external interface
   FILE *traceF;
   Address fakeAddress;
   ConfigVars *config;
};

#define READIntConfigVar(name, variable) \
      if ((v = config->findVariable(name))) \
         variable = atoi(v); \
      else { \
         fprintf(stderr, "Error: must specify var (%s)...quitting\n", name); \
         exit(1); \
      }
#define READStringConfigVar(name, variable) \
      if ((v = config->findVariable(name))) \
         variable = strdup(v); \
      else { \
         fprintf(stderr, "Error: must specify var (%s)...quitting\n", name); \
         exit(1); \
      }
#define READRealConfigVar(name, variable) \
      if ((v = config->findVariable(name))) \
         variable = atof(v); \
      else { \
         fprintf(stderr, "Error: must specify var (%s)...quitting\n", name); \
         exit(1); \
      }


#endif
