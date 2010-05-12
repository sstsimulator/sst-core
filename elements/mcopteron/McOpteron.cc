
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define EXTERN 
#include <McOpteron.h>
#undef EXTERN
#include <FunctionalUnit.h>
#include <InstructionQueue.h>
#include <InstructionInfo.h>
#include <LoadStoreUnit.h>
#include <MemoryModel.h>
#include <ReorderBuffer.h>
#include <ConfigVars.h>

/// @brief Constructor
///
McOpteron::McOpteron()
{
   totalInstructions = 0;
   currentCycle = 0;
   functionalUnitsHead = 0;
   instructionQueuesHead = 0;
   instructionClassProbabilities = 0;
   instructionClassesHead = instructionClassesTail = 0;
   numInstructionClasses = 0;
   infoLEA = 0;
   instructionClasses = 0;
   dependenciesHead = dependenciesTail = 0;
   loadStoreUnit = 0;
   memoryModel = 0;
   lastToken = 0;
   lastCPI = 0.0;
   probBranchMispredict = 0.0;   // TODO: Read this in somewhere!
   traceF = 0;
   fakeAddress = 0x10000;
   fetchStallCycles = 0;
   nextAvailableFetch = 0;
   config = 0;
   outputFP = stdout;
   debugLogFP = stderr;
   Debug = 0;
}


/// @brief Destructor
///
McOpteron::~McOpteron()
{
   fprintf(outputFP, "CPU: stalls due to fetching: %llu\n", fetchStallCycles);
   delete loadStoreUnit;
   delete memoryModel;
   delete reorderBuffer;
   delete fakeIBuffer;
   delete config;
}

/// @brief Check for operand dependencies between instructions
///
/// This walks the current dependency list and checks for a record
/// for the given instruction, and returns it if found. It also
/// prunes any records that have already been consumed.
///
Dependency* McOpteron::checkForDependencies(InstructionCount insn)
{
   Dependency *d, *n, *prev = 0;
   d = dependenciesHead;
   while (d) {
      if (d->consumer == insn)
         return d;
      // prune as we go
      if (d->consumed && d->numProducers == d->numReady) {
         n = d->next;
         if (prev)
            prev->next = d->next;
         else 
            dependenciesHead = d->next;
         if (dependenciesTail == d) {
            if (d->next)
               dependenciesTail = d->next;
            else if (prev)
               dependenciesTail = prev;
            else
               dependenciesTail = dependenciesHead;
         }
         memset(d,0,sizeof(Dependency));
         delete d;
      } else {
        n = d->next;
        prev = d;
      }
      d = n;
   }
   return 0;
}


/// @brief Add a new operand dependency to the dep list
///
Dependency* McOpteron::addNewDependency(InstructionCount insn)
{
   Dependency *d;
   d = checkForDependencies(insn);
   // if there's an existing record for this insn, just increment
   // the number of producers on it and return it
   if (d) {
      if (d->consumed) return 0; // impossible??
      d->numProducers++;
      return d;
   }
   // else, no dependency yet so create a new one
   d = new Dependency();
   d->consumer = insn;
   d->numProducers = 1;
   d->numReady = 0;
   d->consumed = false;
   d->next = 0;
   if (dependenciesHead) {
      dependenciesTail->next = d;
      dependenciesTail = d;
   } else {
      dependenciesHead = dependenciesTail = d;
   }
   return d;
}


/// @brief Create instruction mix probability CDF
///
/// This assumes the main linked list of instruction types
/// has been created, and it works off that list.
/// - TODO: sort the instructions so that more probable 
///   entries are at the beginning of the array
///
void McOpteron::createInstructionMixCDF()
{
   InstructionInfo *ii = instructionClassesHead;
   numInstructionClasses = 0;
   // first count the number of instruction types
   while (ii) {
      numInstructionClasses++;
      ii = ii->getNext();
   }
   if (numInstructionClasses == 0)
      return;
   if (instructionClassProbabilities)
      delete[] instructionClassProbabilities;
   if (instructionClasses)
      delete[] instructionClasses;
   // now create CDF and info ptr arrays
   instructionClassProbabilities = new double[numInstructionClasses];
   instructionClasses = new InstructionInfo*[numInstructionClasses];
   unsigned int i = 0;
   double base = 0.0;
   // now make the CDF array and assign ptr array at same time
   ii = instructionClassesHead;
   while (ii) {
      instructionClassProbabilities[i] = base + ii->getOccurProb();
      instructionClasses[i++] = ii;
      base += ii->getOccurProb();
      ii = ii->getNext();
   }
   // check to make sure probabilities added up right
   if (base < 0.99999 || base > 1.00001) {
      fprintf(stderr, "CDF error: instruction mix probabilities do not add up to 1! Quitting\n");
      exit(1);
   }
   // force last probability to be above 1 (rather than 0.99999)
   instructionClassProbabilities[i-1] = 1.00001;
   return;
}


/// @brief Read an INI-style config file
///
int McOpteron::readConfigFile(const char* filename)
{
   if (!config)
      config = new ConfigVars();
   if (config->readConfigFile(filename)) {
      fprintf(stderr, "Error: failed to process config file (%s)...Quitting\n",
              filename);
      exit(1);
   }
   return 0;
}
   

/// @brief redirect output to specific files
///
void McOpteron::setOutputFiles(const char* outFilename, const char* debugFilename)
{
   FILE *f;
   if (outFilename) {
      f = fopen(outFilename,"w");
      if (!f) {
         fprintf(stderr,"Error: output file (%s) cannot be opened...Quitting\n", 
                 outFilename);
         exit(1);
      }
      outputFP = f;
   }
   if (debugFilename) {
      f = fopen(debugFilename,"w");
      if (!f) {
         fprintf(stderr,"Error: debug log file (%s) cannot be opened...Quitting\n",
                 debugFilename);
         exit(1);
      }
      debugLogFP = f;
   }
}

/// @brief Initialize model 
///
int McOpteron::init(const char* definitionFilename, const char* mixFilename,
                    const char* cpuIniFilename, const char* appIniFilename, 
                    OffCpuIF *extif, const char *traceFilename)
{
   FunctionalUnit *fu;
   InstructionQueue *iq;

   if (Debug>0) fprintf(debugLogFP, "\nInitializing McOpteron model....\n");
   
   readConfigFile(cpuIniFilename);
   readConfigFile(appIniFilename);
   
   externalIF = extif;

   // Create and init memory model and 32-slot load-store unit and 72-slot reorder buffer
   
   // Load-store unit on Opteron is 12-slot LS1 plus 32-slot LS2; it sounds like insns in
   // LS1 immediately move to LS2 after probing the cache, so they wait in LS1 only until
   // their address is ready; our LSQ thus is modeled with 32 slots since that seems to
   // be where instructions sit the longest, though LS1 could be a bottleneck for some apps.
   // - we might be able to limit insns that don't have their address yet, to model LS1
   
   if (config->useDomain("Memory")) {
      fprintf(stderr, "Error: no configuration for Memory...Quitting\n");
      exit(1);
   } else {
      int l1latency, l2latency, l3latency, memlatency, tlblatency;
      const char *v;

      READIntConfigVar("L1Latency", l1latency);
      READIntConfigVar("L2Latency", l2latency);
      READIntConfigVar("L3Latency", l3latency);
      READIntConfigVar("MemoryLatency", memlatency);
      READIntConfigVar("TLBMissLatency", tlblatency);
      if (Debug) fprintf(debugLogFP, "MEM: Latencies: %d %d %d %d %d\n", l1latency,
                         l2latency, l3latency, memlatency, tlblatency);
      memoryModel = new MemoryModel();
      //
      // http://www.anandtech.com/IT/showdoc.aspx?i=3162&p=4
      // said L2 is 12 cycles and L3 is 44-48 cycles; 
      // memory is ~60ns? (120 cycles @ 2GHz)
      // - L1 is 3 cycles including address generation, so really 2
      // 
      memoryModel->initLatencies(tlblatency, l1latency, l2latency,
                                 l3latency, memlatency);
   }
   if (config->useDomain("Application")) {
      fprintf(stderr, "Error: no configuration for Memory...Quitting\n");
      exit(1);
   } else {
      double stfwd, dl1, dl2, dl3, dtlb, ic, il2, il3, itlb, brmiss;
      const char *v;

      READRealConfigVar("DStoreForwardRate", stfwd);
      READRealConfigVar("DL1HitRate", dl1);
      READRealConfigVar("DL2HitRate", dl2);
      READRealConfigVar("DL3HitRate", dl3);
      READRealConfigVar("DTLBMissRate", dtlb);
      READRealConfigVar("ICacheHitRate", ic);
      READRealConfigVar("IL2HitRate", il2);
      READRealConfigVar("IL3HitRate", il3);
      READRealConfigVar("ITLBMissRate", itlb);
      memoryModel->initProbabilities(stfwd, dl1, dl2, dl3, dtlb, 
                                     ic, il2, il3, itlb);
      READRealConfigVar("BranchMispredictRate", brmiss);
      probBranchMispredict = brmiss;
   }

   // The Opteron has a two-level load/store unit, but insns quickly move from
   // the 12-entry LS1 into the 32-entry LS2; essentially it sounds like at 
   // the LS1 they wait for addresses to be generated, at in LS2 wait for the
   // memop to complete. We could consider limiting the number of instructions
   // waiting for an address to be 12 while keeping the size of the LSQ at 32
   if (config->useDomain("LoadStoreQueue")) {
      fprintf(stderr, "Error: no configuration for LoadStoreQueue...Quitting\n");
      exit(1);
   } else {
      int numslots, memops;
      const char *v;
      
      READIntConfigVar("NumSlots", numslots);
      READIntConfigVar("MemOpsPerCycle", memops);
      loadStoreUnit = new LoadStoreUnit(numslots, memops, memoryModel, externalIF);
   }
   // Reorder buffer as 24 lanes with 3 entries each; we are not exactly modeling
   // 3-at-a-time retirement, but just force retirement to be in order and capping
   // the max # per cycle at 3. Hopefully close enough.
   if (config->useDomain("ReorderBuffer")) {
      fprintf(stderr, "Error: no configuration for ReorderBuffer...Quitting\n");
      exit(1);
   } else {
      int numslots, numretire;
      const char *v;
      
      READIntConfigVar("NumSlots", numslots);
      READIntConfigVar("RetirePerCycle", numretire);
      reorderBuffer = new ReorderBuffer(numslots, numretire, false);
      fakeIBuffer = new ReorderBuffer(50, 4, true); // for retiring fake LEAs
   }

   // Create physical model from configuration file
   // Official Opteron: three integer queues (regular, multiply,
   // and special) each with an ALU and AGU, then the floating point queue
   // with FADD, FMUL, and FSTORE functional units
   if (config->useDomain("Architecture")) {
      fprintf(stderr, "Error: no configuration for Architecture...Quitting\n");
      exit(1);
   } else {
      int numintqs, numfloatqs;
      char **qnames, **qunits;
      int *qsizes; int i;
      const char *v; char *tptr;
      char varname[24];
      //FunctionalUnit *fuTail = 0;
      InstructionQueue * iqTail = 0;
      // get number of int and float queues
      READIntConfigVar("NumIntegerQueues", numintqs);
      READIntConfigVar("NumFloatQueues", numfloatqs);
      // allocate config var arrays
      qnames = new char*[numintqs+numfloatqs];
      qunits = new char*[numintqs+numfloatqs];
      qsizes = new int[numintqs+numfloatqs];
      // read in int queue configs
      for (i = 0; i < numintqs; i++) {
         sprintf(varname,"IntQueue%dName",i+1);
         READStringConfigVar(varname, qnames[i]);
         sprintf(varname,"IntQueue%dUnits",i+1);
         READStringConfigVar(varname, qunits[i]);
         sprintf(varname,"IntQueue%dSize",i+1);
         READIntConfigVar(varname, qsizes[i]);
      }
      // read in float queue configs
      for (; i < numintqs+numfloatqs; i++) {
         sprintf(varname,"FloatQueue%dName",i-numintqs+1);
         READStringConfigVar(varname, qnames[i]);
         sprintf(varname,"FloatQueue%dUnits",i-numintqs+1);
         READStringConfigVar(varname, qunits[i]);
         sprintf(varname,"FloatQueue%dSize",i-numintqs+1);
         READIntConfigVar(varname, qsizes[i]);
      }
      // now create the int queues and functional units
      for (i=0; i < numintqs; i++) {
         InstructionQueue::QType qtype = InstructionQueue::INT;
         if (strstr(qunits[i],"ALUSP"))
            qtype = InstructionQueue::INTSP;
         else if (strstr(qunits[i],"ALUMULT"))
            qtype = InstructionQueue::INTMUL;
         iq = new InstructionQueue(qtype, qnames[i], i+1, qsizes[i]);
         if (iqTail) {
            iqTail->setNext(iq);
            iqTail = iq;
         } else {
            instructionQueuesHead = iqTail = iq;
         }
         if (Debug) fprintf(debugLogFP, "  Creating queue %d(%s)\n",i+1,qnames[i]);
         v = strtok_r(qunits[i],",",&tptr);
         while (v) {
            fu = 0;
            if (!strcmp(v,"AGU")) 
               fu = new FunctionalUnit(AGU, "Regular AGU", i+1);
            else if (!strcmp(v,"ALU")) 
               fu = new FunctionalUnit(ALU1, "Regular ALU", i+1);
            else if (!strcmp(v,"ALUSP")) 
               fu = new FunctionalUnit(ALU2, "Special ALU", i+1);
            else if (!strcmp(v,"ALUMULT")) 
               fu = new FunctionalUnit(ALU0, "Multiply ALU", i+1);
            if (!fu) {
               fprintf(stderr,"Error: unknown functional unit (%s)...Quitting\n",
                       v);
               exit(1);
            }
            if (Debug) fprintf(debugLogFP, "  Added func unit (%s)\n", v);
            fu->setNext(functionalUnitsHead);
            functionalUnitsHead = fu;
            iq->addFunctionalUnit(fu);
            v = strtok_r(0,",",&tptr);
         }
      }
      // now create the float queues and functional units
      for (; i < numintqs+numfloatqs; i++) {
         InstructionQueue::QType qtype = InstructionQueue::FLOAT;
         iq = new InstructionQueue(qtype, qnames[i], i+1, qsizes[i]);
         if (iqTail) {
            iqTail->setNext(iq);
            iqTail = iq;
         } else {
            instructionQueuesHead = iqTail = iq;
         }
         if (Debug) fprintf(debugLogFP, "  Creating queue %d(%s)\n",i+1,qnames[i]);
         v = strtok_r(qunits[i],",",&tptr);
         while (v) {
            fu = 0;
            if (!strcmp(v,"FADD")) 
               fu = new FunctionalUnit(FADD, "Float Adder", i+1);
            else if (!strcmp(v,"FMUL")) 
               fu = new FunctionalUnit(FMUL, "Float Multiplier", i+1);
            else if (!strcmp(v,"FSTORE")) 
               fu = new FunctionalUnit(FSTORE, "Float Store", i+1);
            if (!fu) {
               fprintf(stderr,"Error: unknown functional unit (%s)...Quitting\n",
                       v);
               exit(1);
            }
            if (Debug) fprintf(debugLogFP, "  Added func unit (%s)\n", v);
            fu->setNext(functionalUnitsHead);
            functionalUnitsHead = fu;
            iq->addFunctionalUnit(fu);
            v = strtok_r(0,",",&tptr);
         }
      }
   }
   /*** Old static configuration
   iq = new InstructionQueue(InstructionQueue::FLOAT, "Floating Pt Queue", 4, 42);
   //iq->setNext(instructionQueuesHead);
   instructionQueuesHead = iq;
   fu = new FunctionalUnit(FADD, "FP Adder", 7);
   iq->addFunctionalUnit(fu);
   //fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;
   fu = new FunctionalUnit(FMUL, "FP Multiplier", 8);
   iq->addFunctionalUnit(fu);
   fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;
   fu = new FunctionalUnit(FSTORE, "FP Storer", 9);
   iq->addFunctionalUnit(fu);
   fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;

   iq = new InstructionQueue(InstructionQueue::INTMUL, "Multiply Int Queue", 2, 8);
   iq->setNext(instructionQueuesHead);
   instructionQueuesHead = iq;
   fu = new FunctionalUnit(ALU0, "Multiply ALU", 3);
   iq->addFunctionalUnit(fu);
   fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;
   fu = new FunctionalUnit(AGU, "Regular AGU", 4);
   iq->addFunctionalUnit(fu);
   fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;
   
   iq = new InstructionQueue(InstructionQueue::INTSP, "Special Int Queue", 3, 8);
   iq->setNext(instructionQueuesHead);
   instructionQueuesHead = iq;
   fu = new FunctionalUnit(ALU2, "Special ALU", 5);
   iq->addFunctionalUnit(fu);
   fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;
   fu = new FunctionalUnit(AGU, "Regular AGU", 6);
   iq->addFunctionalUnit(fu);
   fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;
   
   iq = new InstructionQueue(InstructionQueue::INT, "Regular Int Queue", 1, 8);
   iq->setNext(instructionQueuesHead);
   instructionQueuesHead = iq;
   fu = new FunctionalUnit(ALU1, "Regular ALU", 1);
   iq->addFunctionalUnit(fu);
   fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;
   fu = new FunctionalUnit(AGU, "Regular AGU", 2);
   iq->addFunctionalUnit(fu);
   fu->setNext(functionalUnitsHead);
   functionalUnitsHead = fu;
   ***/
   
   // Now make the instruction info 

   // read static instruction definition information
   if (Debug) fprintf(debugLogFP, "IDef Input file: %s\n", definitionFilename);
   readIDefFile(definitionFilename);   
   
   // read application specific instruction mix, etc. information
   if (Debug) fprintf(debugLogFP, "IMix Input file: %s\n", mixFilename);
   readIMixFile(mixFilename);
   
   // use instruction type list to make a CDF
   createInstructionMixCDF();
   
   // set up a direct ptr to the LEA instruction (used for 
   // FP insns with memory accesses
   infoLEA = instructionClassesHead->findInstructionRecord("LEA", 64);
   if (!infoLEA) {
      fprintf(stderr, "Error: instruction record for LEA/64 not found! Quitting\n");
      exit(1);
   }

   //
   // If given a trace file, open it
   //
   if (traceFilename) {
      traceF = fopen(traceFilename, "r");
      if (!traceF) {
         fprintf(stderr, "Error: cannot open trace file (%s). Quitting.\n", traceFilename);
         exit(1);
      }
   }

   if (Debug) fprintf(debugLogFP, "Done initializing\n");
   
   return 0;
}


/// @brief Read the app statistics input file
///
/// This expects an input file with lines like this:<br>
///   Instruction: MNEMONIC SIZE CLASS<br>
///   Occurs: #   Loads: #   Stores: #<br>
///   Use distances<br>
///   [multiple 2-number lines, each with a distance and count]<br>
///   Total uses: 0<br>
///   ===<br>
///   [repeat for each instruction]<br>
///
int McOpteron::readIMixFile(const char *filename)
{
   char line[128];
   char mnemonic[24], iClass[24];
   unsigned int iOpSize, useDist, i;
   unsigned long long occurs, loads, stores, totalUses;
   unsigned long long numUses, totalInstructions;
   double iProbability, useHistogram[HISTOGRAMSIZE];
   FILE *inf;
   InstructionInfo *it;

   inf = fopen(filename, "r");
   if (!inf) {
       fprintf(stderr, "Error: Instruction def file (%s) not found...Quitting\n", filename);
      exit(1);
   }

   if (!fgets(line, sizeof(line), inf)) {
      fprintf(stderr, "First line not read! (%s)...Quitting\n", line);
      fclose(inf);
      exit(1);
   }
   if (sscanf(line, "Total instruction count: %llu", &totalInstructions) != 1) {
      fprintf(stderr, "First line not right! (%s)...Quitting\n", line);
      fclose(inf);
      exit(1);
   }

   while (!feof(inf)) {
      if (!fgets(line, sizeof(line), inf))
         break;
      iOpSize = 64;
      iClass[0] = '\0';
      if (sscanf(line,"Instruction: %lg %s %u %s", &iProbability, mnemonic, 
                 &iOpSize, iClass) != 4) {
         fprintf(debugLogFP, "Unknown line (%s), skipping\n", line);
         continue;
      }
      if (Debug>2)
         fprintf(debugLogFP, "Instruction (%s) (%u) (%s)\n", mnemonic, iOpSize, iClass);
      if (!fgets(line, sizeof(line), inf))
         break;
      if (sscanf(line,"Occurs: %llu Loads: %llu Stores: %llu", 
                 &occurs, &loads, &stores) != 3)
         break;
      if (Debug>2)
         fprintf(debugLogFP, " occur %llu  loads %llu  stores %llu\n",
                 occurs, loads, stores);
      if (!fgets(line, sizeof(line), inf))
         break;
      if (strncmp(line,"Use distances", 12))
         break;
      i = 0; totalUses = 0;
      // Now read in use distance histogram; it can have skipped entries
      // which are just zero entries
      while (!feof(inf)) {
         if (!fgets(line, sizeof(line), inf))
            break;
         if (strstr(line,"Total uses")) // reached end
            break;
         if (sscanf(line, "%u %llu", &useDist, &numUses) != 2)
            break;
         if (Debug>2)
            fprintf(debugLogFP, " (%u) (%llu)\n", useDist, numUses);
         // assign previous total uses (for CDF) to missing entries
         while (i < useDist) 
            useHistogram[i++] = totalUses;
         // now fill in this entry (i == useDist) (for CDF, use total uses so far)
         totalUses += numUses;
         useHistogram[i++] = totalUses;
      }
      // fill in rest of entries
      while (i < HISTOGRAMSIZE)
         useHistogram[i++] = totalUses;
      // now make probabilities
      for (i=0; i < HISTOGRAMSIZE; i++)
         useHistogram[i] = useHistogram[i] / (double) totalUses;
      if (!strstr(line,"Total uses"))
         break;
      if (!fgets(line, sizeof(line), inf))
         break;
      if (strncmp(line,"===", 3))
         break;
      // have all data, add it to a record
      it = instructionClassesHead->findInstructionRecord(mnemonic, iOpSize);
      if (!it) {
         fprintf(debugLogFP, "ERROR: instruction record for (%s,%u) not found!\n",
                 mnemonic, iOpSize);
         continue;
      }
      it->accumProbabilities((double) occurs / totalInstructions, occurs,
                             loads, stores, useHistogram);
   }
   fclose(inf);
   return 0;
}


/// @brief Read the instruction definition input file
///
/// This expects an input file with lines like this:<br>
///   MNEMONIC operands operation decodeunit execunits 
///            baselatency memlatency throughput category
///
int McOpteron::readIDefFile(const char *filename)
{
   char line[256];
   char mnemonic[24], operands[24], operation[24], decodeUnit[24],
        execUnits[32], category[24];
   unsigned int i, baseLatency, memLatency, throughputNum, throughputDem;
   FILE *inf = 0;
   InstructionInfo *it;

   inf = fopen(filename, "r");
   if (!inf) {
       fprintf(stderr, "Error: Instruction def file (%s) not found...Quitting\n", filename);
      exit(1);
   }
   
   i = 0;
   while (!feof(inf)) {
      if (!fgets(line, sizeof(line), inf))
         break;
      i++;
      // skip comment lines
      if (line[0] == '/' && (line[1] == '*' || line[1] == '/'))
         continue; 
      if (sscanf(line,"%s %s %s %s %s %u %u %u/%u %s", mnemonic, operands,
                 operation, decodeUnit, execUnits, &baseLatency, &memLatency,
                 &throughputNum, &throughputDem, category) != 10) {
         if (strlen(line) > 5)
            fprintf(debugLogFP, "Error on line %u  (%s), skipping\n", i, line);
         continue;
      }
      if (Debug>2)
         fprintf(debugLogFP, "I %s %s %s %s %s %u %u %u/%u %s\n", mnemonic, operands,
                 operation, decodeUnit, execUnits, baseLatency, memLatency,
                 throughputNum, throughputDem, category);
      // have all data, add it to a record
      it = new InstructionInfo();
      it->initStaticInfo(mnemonic, operands, operation, decodeUnit, execUnits, category);
      it->initTimings(baseLatency, memLatency, throughputNum, throughputDem);
      if (instructionClassesHead) {
         instructionClassesTail->setNext(it);
         instructionClassesTail = it;
      } else {
         instructionClassesHead = instructionClassesTail = it;
      }         
   }
   fclose(inf);
   return 0;
}


/// @brief Finalize simulation and report statistics
///
int McOpteron::finish(bool printInstMix)
{
   FunctionalUnit *fu;
   InstructionQueue *iq;
   InstructionInfo *it;
   
   if (traceF)
      fclose(traceF);

   fprintf(outputFP, "\n");
   fprintf(outputFP, "Total cycles simulated: %llu\n", currentCycle);
   fprintf(outputFP, "Total instructions generated: %llu\n", totalInstructions);
   //fprintf(outputFP, "Total instructions retired  : %llu\n", retiredInstructions);
   fprintf(outputFP, "Predicted CPIt: %g\n", currentCPI());
   //fprintf(outputFP, "Predicted CPIr: %g\n", (double) currentCycle /
   //          retiredInstructions);
   fprintf(outputFP, "\n");
   
   // deconstruct instruction info
   while (instructionClassesHead) {
      it = instructionClassesHead;
      instructionClassesHead = it->getNext();
      if ((Debug>2 || printInstMix) && 
          it->getSimulationCount() > 0)
          fprintf(outputFP, "%s: Trace prob: %g  actual prob: %g\n",
                  it->getName(), it->getOccurProb(),
                  (double) it->getSimulationCount() / totalInstructions);
      delete it;
   }

   // deconstruct physical model
   if (Debug) fprintf(debugLogFP,"\nDeleting stuff\n");
   while (instructionQueuesHead) {
      iq = instructionQueuesHead;
      instructionQueuesHead = iq->getNext();
      delete iq;
   }
   if (Debug) fprintf(debugLogFP, ".\n");
   while (functionalUnitsHead) {
      fu = functionalUnitsHead;
      functionalUnitsHead = fu->getNext();
      delete fu;
   }
   if (Debug) fprintf(debugLogFP, ".\n");
   return 0;
}


/// @brief Get current cycle count
///
CycleCount McOpteron::currentCycles()
{
   return currentCycle;
}


/// @brief Get current CPI
///
double McOpteron::currentCPI()
{
   return (double) currentCycle / reorderBuffer->retiredCount();
}

//=========================================================
//
// Main simulation routines: everything starts at simCycle()
//
//=========================================================

/// @brief Simulate one cycle
///
/// This function works backwards up the pipeline since
/// we need to open things up to move things forward, and
/// software doesn't all happen at once.
///
int McOpteron::simCycle()
{
   // print out a progress dot
   if (Debug && currentCycle % 100000 == 0) { fprintf(debugLogFP,".\n"); }
   currentCycle++;
   if (Debug>1) fprintf(debugLogFP, "======= Simulating cycle %llu ====== \n",
                        currentCycle);
   // Update reorder buffer (and fake buffer for FP memops)
   fakeIBuffer->updateStatus(currentCycle);
   reorderBuffer->updateStatus(currentCycle);
   if (Debug>1) fprintf(debugLogFP, "===Updated reorder buffer===\n");
   // Update load-store queue (must be done after reorder buffer update
   // so that we remove canceled insns)
   loadStoreUnit->updateStatus(currentCycle);
   if (Debug>1) fprintf(debugLogFP, "===Updated load/store unit===\n");
   // Update all functional units to see if any come free
   updateFunctionalUnits();
   if (Debug>1) fprintf(debugLogFP, "===Updated functional units===\n");
   // Assign new instructions to available functional units
   // - this is the heart of the simulation, really
   scheduleNewInstructions();
   if (Debug>1) fprintf(debugLogFP, "===Scheduled new instructions===\n");
   // Fetch new instructions to fill up the queues again
   // - this goes off and asks for new tokens to be generated, if needed
   refillInstructionQueues();
   if (Debug>1) fprintf(debugLogFP, "===Refilled instruction queues===\n");
   // Check for finishing conditions
   if (!(currentCycle % 500000)) {
      double cpi = (double) currentCycle / totalInstructions;
      if (fabs(lastCPI - cpi) < 0.01)
         return 1;
      lastCPI = cpi;
   }
   if (allQueuesEmpty() && traceF) // quit when trace is done
      return 1;
   else
      return 0;
}


/// @brief Check if all instruction queues are empty
///
bool McOpteron::allQueuesEmpty()
{
   InstructionQueue *iq = instructionQueuesHead;
   while (iq) {
      if (! iq->isEmpty())
         return false;
      iq = iq->getNext();
   }
   return true;
}


/// @brief Update all functional units to current cycle
///
/// The idea here is to allow functional units to decide
/// if they are busy or available at this cycle.
/// Functional units are busy only if new insns can't be
/// issued to them yet; due to pipelining an insn may still
/// be "in" the unit, but the unit is ready for another one.
///
int McOpteron::updateFunctionalUnits()
{
   FunctionalUnit *fu = functionalUnitsHead;
   while (fu) {
      fu->updateStatus(currentCycle);
      fu = fu->getNext();
   }
   return 0;
}


/// @brief Schedule new instructions onto functional units
///
/// This function allows queues to place new instructions on
/// available functional units.
///
int McOpteron::scheduleNewInstructions()
{
   InstructionQueue *iq = instructionQueuesHead;
   while (iq) {
      iq->scheduleInstructions(currentCycle);
      iq = iq->getNext();
   }
   return 0;
}


/// @brief Put newly fetched instructions into queues
///
/// Allow queues to get new instructions in them if they
/// have room.
/// - TODO: make double-dispatch instructions take two slots
///   when dispatching; must be able to "save" one slot from
///   previous cycle if a double-d insn couldn't dispatch
///   previously.
///
int McOpteron::refillInstructionQueues()
{
   int numAssigned = 0;
   //unsigned int randOffset = 0;
   bool didAssignment, quitNow = false;
   InstructionQueue *iq = 0;
   // if we have to stall for fetching, then stall
   if (nextAvailableFetch > currentCycle) {
      if (Debug>2) fprintf(debugLogFP,"Stalling for fetch, now %llu next %llu\n",
                           currentCycle, nextAvailableFetch);
      fetchStallCycles++;
      return 0;
   }

   // update for next fetch (notice the cycle+1 parameter; we are figuring
   // out if the NEXT cycle will have a stall; this is because the stall may
   // be for multiple cycles and so we have to re-use the value of 
   // nextAvailableFetch in the above if statement over multiple calls)
   nextAvailableFetch = memoryModel->serveILoad(currentCycle+1,0,16);
   // TODO: eventually link in off-cpu calls for instruction fetch
   //externalIF->memoryAccess(OffCpuIF::AM_READ, 0x1000, 9);
   
   // generate random queue start to mimic random assignment (this is a
   // deeper question of how insns really get allocated to queues, so it
   // is commented out; besides, I thought it would improve performance 
   // but it made it worse! (at least for hpccg))
   //randOffset = ((unsigned int)(genRandomProbability()*101)) % 4;
   
   // must iterate multiple times through the instruction queues
   // since a token generated later might be able to be assigned
   // to an earlier queue; we stop when no more assignments were
   // done, or we reached the max.
   do {
      iq = instructionQueuesHead;
      //while (iq && randOffset) {
      //   iq = iq->getNext();
      //   randOffset--;
      //}
      didAssignment = false;
      for (; iq && numAssigned < MAXCANASSIGN; iq = iq?iq->getNext():iq) {
         // we might have a token left over from the last call that couldn't
         // be assigned, but if not we generate a new one
         if (!lastToken)
            lastToken = generateToken();
         if (!lastToken) {
            quitNow = true;
            break; // no more can be generated now (end of trace)
         }
         if (!iq->canHandleInstruction(lastToken))
            // wrong queue for this insn, so skip
            continue;
         if (reorderBuffer->isFull()) {
            reorderBuffer->incFullStall();
            break; // this is fatal, so leave loop early
         }
         if (iq->isFull()) {
            // queue is full, so skip
            iq->incFullStall();
            continue;
         }
         if (iq->alreadyAssigned(currentCycle)) {
            // queue has already been assigned this cycle, so skip
            iq->incAlreadyAssignedStall();
            continue;
         }
         // Finally, token is ready to send to queue, but last check
         // is if it has a memop then the load-store queue must not be full
         if ((!lastToken->isLoad() && !lastToken->isStore()) ||
              loadStoreUnit->add(lastToken, currentCycle)) {
            iq->assignInstruction(lastToken, currentCycle);
            if (!lastToken->isFake())
               reorderBuffer->dispatch(lastToken, currentCycle);
            else
               fakeIBuffer->dispatch(lastToken, currentCycle);
            numAssigned++;
            didAssignment = true;
            // generate a fake LEA if this is an FPU insn with memop
            if (lastToken->getType()->isFPUInstruction() &&
                (lastToken->isLoad() || lastToken->isStore())) {
               // we give it the same instruction number, this should be ok
               Token *t = new Token(infoLEA, lastToken->instructionNumber(),
                                    currentCycle, true);
               Dependency *d = addNewDependency(lastToken->instructionNumber());
               t->setOutDependency(d);
               lastToken->setInDependency(d); // will be same as existing, if one
               // now force loop to assign the LEA to a queue
               lastToken = t;
               //randOffset = ((unsigned int)(genRandomProbability()*101)) % 4;
               //iq = 0; (do break?)
            } else
               lastToken = 0; // used it up, so force the generation of a new one
         }
      }
      if (Debug>1) fprintf(debugLogFP, "Refilling instruction queues, numassigned=%d\n",
                         numAssigned);
   } while (!quitNow && numAssigned < MAXCANASSIGN && didAssignment);
   return 0;
}


/// @brief Generate an instruction token
///
/// 
Token* McOpteron::generateToken()
{
   Token *t;
   double p;
   unsigned int i;
   Dependency *dep;
   
   // If running off a trace, bypass this routine and get token from trace
   if (traceF)
      return getNextTraceToken();
      
   if (Debug>1) fprintf(debugLogFP, "Generating token at %llu\n", currentCycle);
   // sample instruction mix histogram
   p = genRandomProbability();
   for (i=0; i < numInstructionClasses; i++)
      if (p < instructionClassProbabilities[i])
         break;
   assert(i < numInstructionClasses);
   // create token
   t = new Token(instructionClasses[i], totalInstructions++, currentCycle, false);
   // set optional probability (not really used right now)
   p = genRandomProbability();
   t->setOptionalProb(p);
   // set mispredicted flag if appropriate
   if (Debug>2 && t->getType()->isConditionalJump()) 
      fprintf(debugLogFP, "TTToken (%s) is a conditional jump\n", 
              t->getType()->getName());
   if (t->getType()->isConditionalJump() && p <= probBranchMispredict) {
      if (Debug>2) fprintf(debugLogFP, "  Mispredict!\n");
      t->setBranchMispredict();
   }
   // sample probability of address operand
   // TODO: allow both load and store on same insn
   if (genRandomProbability() <= t->getType()->getLoadProb()) {
      if (Debug>2) fprintf(debugLogFP, "  has load  at %llu\n", fakeAddress);
      t->setMemoryLoadInfo(fakeAddress++, 8); // 8-byte fetch
   } else if (genRandomProbability() <= t->getType()->getStoreProb()) {
      if (Debug>2) fprintf(debugLogFP, "  has store at %llu\n", fakeAddress);
      t->setMemoryStoreInfo(fakeAddress++, 8); // 8-byte store
   }
   // check if this instruction has incoming dependencies
   dep = checkForDependencies(t->instructionNumber());
   t->setInDependency(dep); // might be null, but ok
   if (Debug>2 && dep) fprintf(debugLogFP, "  num indeps: %u\n", dep->numProducers);
   // sample dependent distance
   p = genRandomProbability();
   i = t->getType()->getUseDistance(p);
   if (i > 0) {
      dep = addNewDependency(t->instructionNumber()+i);
      t->setOutDependency(dep);
      if (Debug>2 && dep) fprintf(debugLogFP, "  outdep insn: %llu (%u,%u)\n",
                                  dep->consumer, dep->numProducers, dep->numReady);
   }
   // Now fix InstructionInfo record in case there are multiple and
   // they depend on data direction, size, etc.
   t->fixupInstructionInfo();
   // MORE HERE TO DO?
   // done
   if (Debug>1) fprintf(debugLogFP, "  token %llu is %s type %u (%g %g %g) addr: %s\n",
           t->instructionNumber(), t->getType()->getName(), t->getType()->getCategory(),
           t->getType()->getOccurProb(),
           t->getType()->getLoadProb(), t->getType()->getStoreProb(), 
           t->needsAddressGeneration()?"T":"F");
   return t;
}

/// @brief Get an instruction token from a trace file
///
/// For now, the trace format is one instruction per line, each 
/// line formatted as "mnemonic opsize memflag usedist". The mnemonic
/// is the instruction name, opsize is the operand size (8/16/32/64/128),
/// memflag is 0/1/2 for none/load/store, and usedist is the number of
/// instructions ahead that the result of this one will be used 
/// (1 == next instruction).
///
Token* McOpteron::getNextTraceToken()
{
   Token *t;
   char line[256], mnemonic[24];
   unsigned int opSize, memFlag, useDist;
   int n;
   InstructionInfo *it;
   double p;
   Dependency *dep;

   if (Debug>1) fprintf(debugLogFP, "Getting trace token at %llu\n", currentCycle);
   if (!fgets(line, sizeof(line), traceF))
      return 0;
   n = sscanf(line, "%s %u %u %u", mnemonic, &opSize, &memFlag, &useDist);
   if (n != 4)
      return 0;
   it = instructionClassesHead->findInstructionRecord(mnemonic, opSize);
   if (!it)
      return 0;
      
   // if we got here, we read the line and parsed it ok, and we found the mnemonic
   // in the InstructionInfo records

   t = new Token(it, totalInstructions++, currentCycle, false);
   // set optional probability
   p = genRandomProbability();
   t->setOptionalProb(p);
   // set mispredicted flag if appropriate 
   // TODO: a trace can't have mispredicted jumps?
   if (t->getType()->isConditionalJump() && p <= probBranchMispredict) {
      t->setBranchMispredict();
   }
   // set memory access info
   if (memFlag == 1) {
      if (Debug>2) fprintf(debugLogFP, "  has load  at %llu\n", fakeAddress);
      t->setMemoryLoadInfo(fakeAddress++, 8); // 8-byte fetch
   } else if (memFlag == 2) {
      if (Debug>2) fprintf(debugLogFP, "  has store at %llu\n", fakeAddress);
      t->setMemoryStoreInfo(fakeAddress++, 8); // 8-byte store
   }
   // check if this instruction has incoming dependencies
   dep = checkForDependencies(t->instructionNumber());
   t->setInDependency(dep); // might be null, but ok
   if (Debug>1 && dep) fprintf(debugLogFP, "  num indeps: %u\n", dep->numProducers);
   // set dependent distance
   if (useDist > 0) {
      dep = addNewDependency(t->instructionNumber()+useDist);
      t->setOutDependency(dep);
      if (Debug>2 && dep) fprintf(debugLogFP, "  outdep insn: %llu (%u,%u)\n",
                                  dep->consumer, dep->numProducers, dep->numReady);
   }
   // Now fix InstructionInfo record in case there are multiple and
   // they depend on data direction, size, etc.
   t->fixupInstructionInfo();
   // MORE HERE TO DO?
   // done
   if (Debug>1) fprintf(debugLogFP, "  token %llu is %s type %u (%g %g %g) addr: %s\n",
           t->instructionNumber(), t->getType()->getName(), t->getType()->getCategory(),
           t->getType()->getOccurProb(),
           t->getType()->getLoadProb(), t->getType()->getStoreProb(), 
           t->needsAddressGeneration()?"T":"F");
   return t;
}


//=========================================================
//
// Main program driver, if not compiled into a library
//
//=========================================================

#ifndef SST
/// Dummy (empty) off-CPU interface class
class NullIF: public OffCpuIF
{
  public:
     void  memoryAccess(access_mode mode, unsigned long long address, 
                        unsigned long data_size)
     {
        if (Debug>3) fprintf(debugLogFP, "Memory access: mode %d addr %llx size %ld\n",
                             mode, address, data_size);
     }
     virtual void  NICAccess(access_mode mode, unsigned long data_size)
     {
        //fprintf(debugLogFP, "NIC access: mode %d size %ld\n",
        //        mode, data_size);
     }
};

#include <stdlib.h>

static const char* helpMessage = 
"\nUsage: mcopteron [options]\n\
Options:\n\
  --debug #        print lots of debugging information (# in 1-3)\n\
  --cycles #       set number of cycles to simulate\n\
  --converge       run until CPI converges\n\
  --deffile name   use 'name' as insn def file (default: opteron-insn.txt)\n\
  --dcycle #       start debugging output at cycle # (rather than 0)\n\
  --dfile name     send debugging output to named file (default: stderr)\n\
  --imix           print out simulation instruction mix at end\n\
  --mixfile name   use 'name' as insn mix input file (default: usedist.all)\n\
  --outfile name   send normal output to named file (default: stdout)\n\
  --seed #         set random number seed\n\
  --trace name     use 'name' as input instruction trace file\n\n";
  
void doHelp()
{
   fputs(helpMessage, stderr);
   exit(1);
}

/// @brief Standalone simulator driver
///
int main(int argc, char **argv)
{
   int i;
   unsigned long numSimCycles = 100000;
   unsigned long debugCycle = 0, c;
   unsigned int debug = 0;
   unsigned long seed = 100;
   bool untilConvergence = false;
   bool printIMix = false;
   const char *mixFile = "usedist.all";
   const char *defFile = "opteron-insn.txt";
   char *traceFile = 0;
   char *outFile = 0;
   char *debugFile = 0;
   NullIF extIF;
   McOpteron *cpu;
   Debug = 0;
   for (i = 1; i < argc; i++) {
      if (!strcmp("--debug",argv[i])) {
         if (i == argc-1) doHelp();
         debug = atoi(argv[++i]);
      } else if (!strcmp("--converge",argv[i])) {
         untilConvergence = true;
      } else if (!strcmp("--cycles",argv[i])) {
         if (i == argc-1) doHelp();
         numSimCycles = atoi(argv[++i]);
      } else if (!strcmp("--dcycle",argv[i])) {
         if (i == argc-1) doHelp();
         debugCycle = atol(argv[++i]);
      } else if (!strcmp("--dfile",argv[i])) {
         if (i == argc-1) doHelp();
         debugFile = argv[++i];
      } else if (!strcmp("--deffile",argv[i])) {
         if (i == argc-1) doHelp();
         defFile = argv[++i];
      } else if (!strcmp("--imix",argv[i])) {
         printIMix = true;
      } else if (!strcmp("--mixfile",argv[i])) {
         if (i == argc-1) doHelp();
         mixFile = argv[++i];
      } else if (!strcmp("--outfile",argv[i])) {
         if (i == argc-1) doHelp();
         outFile = argv[++i];
      } else if (!strcmp("--seed",argv[i])) {
         if (i == argc-1) doHelp();
         seed = atol(argv[++i]);
      } else if (!strcmp("--trace",argv[i])) {
         if (i == argc-1) doHelp();
         traceFile = argv[++i];
      } else  {
         doHelp();
         return 0;
      }
   }
   if (debugCycle == 0)
      Debug = debug;
   seedRandom(seed);
   cpu = new McOpteron();
   cpu->setOutputFiles(outFile, debugFile);
   fprintf(outputFP, "Initializing with input (%s,%s)\n", defFile, mixFile);
   cpu->init(defFile, mixFile, "cpuconfig.ini", "appconfig.ini",
             &extIF, traceFile);
   if (untilConvergence) {
      fprintf(outputFP, "Simulating till convergence\n");
      for (c = 0; true; c++) {
         if (c == debugCycle)
            Debug = debug;
         if (cpu->simCycle())
            break;
      }
   } else {
      fprintf(outputFP, "Simulating %ld cycles\n", numSimCycles);
      for (c = 0; c < numSimCycles; c++) {
         if (c == debugCycle)
            Debug = debug;
         if (cpu->simCycle())
            break;
      }
   }
   fprintf(outputFP, "Done simulating\n");
   cpu->finish(printIMix);
   delete cpu;
   return 0;
}

#endif
