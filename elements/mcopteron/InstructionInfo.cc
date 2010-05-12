
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <InstructionInfo.h>

/// @brief Constructor (but use init() real data)
InstructionInfo::InstructionInfo()
{
   name = operands = operation = decodeUnit = execUnits = 0;
   latency = throughputNum = throughputDem = memLatency = 0;
   occurProbability = loadProbability = storeProbability = 0.0;
   category = UNKNOWN;
   stackOp = conditionalJump = false;
   opSize = 0; 
   totalOccurs = 0;
   execUnitMask = 0;
   allowedDataDirs = 0;
   next = 0;
   for (int i=0; i < HISTOGRAMSIZE; i++)
      toUseHistogram[i] = 0.0;
}


/// @brief Destructor
InstructionInfo::~InstructionInfo()
{
   if (name) free(name);
   if (operands) free(operands);
   if (operation) free(operation);
   if (decodeUnit) free(decodeUnit);
   if (execUnits) free(execUnits);
}

/// @brief Dump debug info about instruction record
///
void InstructionInfo::dumpDebugInfo()
{
   fprintf(debugLogFP, "II: name (%s) operands (%s) operation (%s) execUnits (%s)\n",
           name, operands, operation, execUnits);
   fprintf(debugLogFP, "II: category %u opsize %u stackop %s unitmask %lu, datadirs %u\n",
           category, opSize, stackOp?"T":"F", execUnitMask, allowedDataDirs);
}

/// @brief Initializer
///
void InstructionInfo::initStaticInfo(char *name, char *operands, char *operation,
                                     char *decodeUnit, char *execUnits, char *category)
{
   if (name) {
      if (name[0] == '*')
         this->name = strdup(name+1);
      else
         this->name = strdup(name);
   }
   if (operands) this->operands = strdup(operands);
   if (operation) this->operation = strdup(operation);
   if (decodeUnit) this->decodeUnit = strdup(decodeUnit);
   if (execUnits) this->execUnits = strdup(execUnits);
   // process exec units to figure out execution paths
   if (execUnits) {
      if (strstr(execUnits,"ALU0")) {
         this->category = MULTINT;
         execUnitMask = ALU0;
      } else if (strstr(execUnits,"ALU2")) {
         this->category = SPECIALINT;
         execUnitMask = ALU2;
      } else if (strstr(execUnits,"AGU")) {
         this->category = GENERICINT;
         execUnitMask = AGU;
      } else if (strstr(execUnits,"FADD")) {
         this->category = FLOAT;
         execUnitMask = FADD;
      } else if (strstr(execUnits,"FMUL")) {
         this->category = FLOAT;
         execUnitMask = FMUL;
      } else if (strstr(execUnits,"FSTORE")) {
         this->category = FLOAT;
         execUnitMask = FSTORE;
      } else {
         this->category = GENERICINT;
         execUnitMask = ALU0 | ALU1 | ALU2;
      }
   }
   // check if a stack instruction (these do not need to
   // use an AGU for address generation)
   if (operation && strstr(operation, "STACK"))
      stackOp = true;
   // check if a conditional jump
   if (operation && strstr(operation, "COND"))
      conditionalJump = true;
   // process operands to find data direction and size
   while (operands) { // really an if, but we want to use break
      char *dest, *src, *opcopy;
      if (strstr(operands,"128"))
         opSize |= OPSIZE128;
      if (strstr(operands,"64"))
         opSize |= OPSIZE64;
      if (strstr(operands,"32"))
         opSize |= OPSIZE32;
      if (strstr(operands,"16"))
         opSize |= OPSIZE16;
      if (strstr(operands,"8"))
         opSize |= OPSIZE8;
      if (strstr(operands,"xmm"))
         opSize |= OPSIZE64 | OPSIZE128;
      if (!opSize)
         opSize |= OPSIZE64; // default
      opcopy = strdup(operands);
      dest = strtok(opcopy, ",");
      src = strtok(0,",");
      if (!dest) {
         free(opcopy);
         break;
      }
      if (strstr(dest,"reg")) {
         if (!src) // unary op
            allowedDataDirs |= IREG2IREG;
         if (src && strstr(src,"reg"))
            allowedDataDirs |= IREG2IREG;
         if (src && strstr(src,"mem"))
            allowedDataDirs |= MEM2IREG;
         if (src && strstr(src,"mm"))
            allowedDataDirs |= FREG2IREG;
      }
      if (strstr(dest,"mm")) {
         if (!src) // unary op
            allowedDataDirs |= FREG2FREG;
         if (src && strstr(src,"reg"))
            allowedDataDirs |= IREG2FREG;
         if (src && strstr(src,"mem"))
            allowedDataDirs |= MEM2FREG;
         if (src && strstr(src,"mm"))
            allowedDataDirs |= FREG2FREG;
      }
      if (strstr(dest,"mem")) {
         if (!src) // unary op
            allowedDataDirs |= MEM2MEM;
         if (src && strstr(src,"reg"))
            allowedDataDirs |= IREG2MEM;
         if (src && strstr(src,"mem"))
            allowedDataDirs |= MEM2MEM;
         if (src && strstr(src,"mm"))
            allowedDataDirs |= FREG2MEM;
      }
      free(opcopy);
      break; // make the while an if
   }
   if (Debug>2)
      fprintf(debugLogFP, "IInfo-si: (%s) (%s)%u:%u (%s) (%s) (%u) (%s)\n",
              this->name, operands, opSize, allowedDataDirs, decodeUnit, 
              execUnits, this->category, operation);
}

/// @brief Initializer
///
void InstructionInfo::initTimings(unsigned int baseLatency, unsigned int memLatency,
                                  unsigned int throughputNum, unsigned int throughputDem)
{
   this->latency = baseLatency;
   this->memLatency = memLatency;
   this->throughputNum = throughputNum;
   this->throughputDem = throughputDem;
}

/// @brief Initializer
///
void InstructionInfo::initProbabilities(double occurProb, double loadProb,
                                        double storeProb, double useHistogram[])
{
   occurProbability = occurProb;
   loadProbability = loadProb;
   storeProbability = storeProb;
   for (int i=0; i < HISTOGRAMSIZE; i++)
      toUseHistogram[i] = useHistogram[i];
}

/// @brief Use this to group together multiple Pin instruction types
void InstructionInfo::accumProbabilities(double occurProb, unsigned long long occurs,
                               unsigned long long loads, unsigned long long stores,
                               double useHistogram[])
{
   int i;
   // if no occurs, don't do anything
   if (occurs == 0)
      return;
   // overall i-mix probability can just be added in
   occurProbability += occurProb;
   // reduce existing load/store probs by correct fraction
   if (totalOccurs) {
      loadProbability  = loadProbability  * totalOccurs / 
                         (totalOccurs + occurs);
      storeProbability = storeProbability * totalOccurs / 
                         (totalOccurs + occurs);
      for (i=0; i < HISTOGRAMSIZE; i++)
         toUseHistogram[i] = toUseHistogram[i] * totalOccurs / 
                             (totalOccurs + occurs);
   }
   // update total observations
   totalOccurs += occurs;
   // add in new probs
   loadProbability  += (double) loads  / totalOccurs;
   storeProbability += (double) stores / totalOccurs;
   for (i=0; i < HISTOGRAMSIZE; i++)
      toUseHistogram[i] += useHistogram[i] * occurs / 
                          (totalOccurs + occurs + 0.00001);
      // JEC: This is not quite right because histogram might not have
      // as many entries as occurs, since the use might never be recorded
      // if it gets too far. But it is close.
}

// TODO: use instruction size and operands (not passing these yet! -- need
// at least the load/store flags)
InstructionInfo* InstructionInfo::findInstructionRecord(const char *mnemonic,
                                                        unsigned int iOpSize)
{
   char *searchName = NULL, *p;
   OperandSize opSize;
   InstructionInfo *it = this, *first = 0;
   if (!mnemonic) return 0;
   // convert all conditional jumps into a generic JCC
   if (mnemonic[0] == 'J' && strcmp(mnemonic,"JMP") && !strstr(mnemonic,"CXZ"))
       searchName = strdup("JCC");
   // convert all conditional cmov's into a generic CMOVCC
   else if (strstr(mnemonic,"CMOV") == mnemonic)
       searchName = strdup("CMOVCC");
   // convert all conditional set's into a generic SETCC
   else if (strstr(mnemonic,"SET") == mnemonic)
       searchName = strdup("SETCC");
   // convert all conditional loop's into a generic SETCC
   else if (strstr(mnemonic,"LOOP") == mnemonic)
       searchName = strdup("LOOPCC");
   else {
       searchName = strdup(mnemonic);
       // remove any '_NEAR' or '_XMM' extensions
       if ((p=strstr(searchName,"_NEAR")))
           *p = '\0';
       if ((p=strstr(searchName,"_XMM")))
           *p = '\0';
   }
   switch (iOpSize) {
    case 8: opSize = OPSIZE8; break;
    case 16: opSize = OPSIZE16; break;
    case 32: opSize = OPSIZE32; break;
    case 64: opSize = OPSIZE64; break;
    case 128: opSize = OPSIZE128; break;
    default: opSize = OPSIZE64; 
   }
   if (Debug>2) fprintf(debugLogFP, "findII: searching for (%s)...", searchName);
   while (it && strcmp(it->name, searchName)) {
      it = it->next;
   }
   first = it;
   while (it && !strcmp(it->name, searchName) && !(opSize & it->opSize)) {
      it = it->next;
   }
   if (strcmp(it->name, searchName))
      it = first; // reset back to first one found
   if (Debug>2) fprintf(debugLogFP, "%p (%s)\n", it, it->name);

   if (NULL != searchName) free(searchName);
   return it;
}


/// @brief Create and initialize an object from string data
///
InstructionInfo* InstructionInfo::createFromString(char* infoString)
{
   return 0;
}


/// @brief Given a probability, retrieve matching use distance
///
/// This samples the CDF toUseHistogram to retrieve the distance
/// to the instruction that uses what this instruction produces
///
unsigned int InstructionInfo::getUseDistance(double prob)
{
   unsigned int i;
   for (i=0; i < HISTOGRAMSIZE; i++)
      if (prob <= toUseHistogram[i])
         break;
   if (i == HISTOGRAMSIZE)
      return 0;
   return i;
}
