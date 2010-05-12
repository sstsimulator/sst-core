// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2007, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "ppcMachine.h"
#include "ppcFront.h"
#include "ppcLoader.h"
//#include "configuration.h"
#include "processor.h"
#include "regs.h"
#include <stdio.h>
#include <string>
#include <algorithm>
#include "environ.h"
#include "fe_debug.h"

using namespace std;

extern const char *instNames[];

#if 0
uint32 ppcThread::PageSize=0;
uint32 ppcThread::PageShift=0;
uint32 ppcThread::PageMask=0;
#endif
//: Array for instruction memory
//uint8  **ppcThread::textPageArray=0;
int ppcThread::verbose = 0;
pool<ppcInstruction> ppcThread::iPool;
ppcThread::reservedSetT ppcThread::reservedSet;
vector<ppcThread::adrRange> ppcThread::constData;
map<unsigned int, ppcThread*> ppcThread::threadIDMap;
unsigned int ppcThread::nextThreadID = 1;
bool ppcThread::realGettimeofday = 0;
bool ppcThread::exitSysCallExitsAll = 0;

#if 0
uint8 *ppcThread::progText;
simAddress ppcThread::lastTextAddr;
vector<simAddress> ppcThread::textRanges;
#endif

vector<thread*> ppcThread::init(processor *proc, 
				SST::Component::Params_t& paramsC)
{
  // Do this early
  {
    SST::Component::Params_t::iterator pi = paramsC.find("verbose");
    if (pi != paramsC.end()) {
      sscanf(pi->second.c_str(), "%d", &ppcThread::verbose);
    }
    if (ppcThread::verbose > 0)   {
      __print_info= 1;
      __dprint_level= ppcThread::verbose;
    } else   {
      __print_info= 0;
      __dprint_level= ppcThread::verbose;
    }
    if (ppcThread::verbose < 3)   {
      printf("#### You can set or increase the verbose parameter for genericProc in the xml file to get more output!\n");
    }
  }

  md_init_decoder();

  #if 0
  if (configuration::getValue(cfgstr +":frontEnd:exitSysCallExitsAll") == 1) {
    exitSysCallExitsAll = 1;
  } else {
    exitSysCallExitsAll = 0;
  }
 
  if (configuration::getValue(cfgstr + ":frontEnd:loadsAlwaysCheckFEB") == 1) {
    INFO("Loads always check FEB.\n");
    ppcInstruction::loadsAlwaysCheckFEB = 1;
  } else {
    ppcInstruction::loadsAlwaysCheckFEB = 0;
  }

  if (configuration::getValue( cfgstr + ":frontEnd:storesAlwaysSetFEB") == 1) {
    INFO("Loads always check FEB.\n");
    ppcInstruction::storesAlwaysSetFEB = 1;
  } else {
    ppcInstruction::storesAlwaysSetFEB = 0;
  }

  if (configuration::getValue( cfgstr + ":frontEnd:allowSelfModify") == 1) {
    INFO("Allowing Self Modifying code.\n");
    ppcInstruction::allowSelfModify = 1;
  } else {
    ppcInstruction::allowSelfModify = 0;
  }

  int magStack = configuration::getValue(cfgstr + ":frontEnd:magicStack");
  if (magStack == 1) {
    INFO("Using Magic Stack!\n");
    ppcInstruction::magicStack = 1;
  } else {
    INFO("No Magic Stack. Also, not using seperate TEXT memory.\n");
    ppcInstruction::magicStack = 0;
  }
#else
  exitSysCallExitsAll = 0;
  ppcInstruction::loadsAlwaysCheckFEB = 0;
  ppcInstruction::storesAlwaysSetFEB = 0;
  ppcInstruction::allowSelfModify = 0;
  ppcInstruction::magicStack = 0;
#endif

  // wcmclen: force softfloat library for FP operations via config file
  //          Useful for debugging and unit testing.
#ifdef __ppc__
  bool on_ppc = 1;
#else
  bool on_ppc = 0;
#endif
  /* initialize default values to use ppc native assembly */
  ppcInstruction::fpu_mode_software  = 0;
  ppcInstruction::fpu_mode_cplusplus = 0;
  ppcInstruction::fpu_mode_asm_x86   = 0;
  ppcInstruction::fpu_mode_asm_ppc   = 1;
  /* read configuration options */
#if 0
  int _fpu_mode_software = configuration::getValue(":fpu:software-emulate");
  int _fpu_mode_cplusplus= configuration::getValue(":fpu:software-cplusplus");
  int _fpu_mode_asm_x86  = configuration::getValue(":fpu:asm-x86");
#else
  int _fpu_mode_software = 0;
  int _fpu_mode_cplusplus= 0;
  //int _fpu_mode_asm_x86  = 0;
#endif
  if(!on_ppc) {
    _fpu_mode_software = 1;
  }

  if(_fpu_mode_software==1)
  {
    INFO("***************************************************\n");
    INFO("***                                             ***\n");
    INFO("***                W A R N I N G                ***\n");
    INFO("***                                             ***\n");
    INFO("***  Software-emulation of floating-point       ***\n");
    INFO("***  operations is enabled.  FP Status Register ***\n");
    INFO("***  (FPSCR) non-IEEE 754 flags' accuracy not   ***\n");
    INFO("***  guaranteed.                                ***\n");
    INFO("***                                             ***\n");
    INFO("***      To disable this warning banner add     ***\n");
    INFO("***           :fpu:software-emulate             ***\n");
    INFO("***      to your SST configuration file.        ***\n");
    INFO("***                                             ***\n");
    INFO("***************************************************\n");
    ppcInstruction::fpu_mode_software  = 1;
    ppcInstruction::fpu_mode_cplusplus = 0;
    ppcInstruction::fpu_mode_asm_ppc   = 0;
    ppcInstruction::fpu_mode_asm_x86   = 0;
  }
  else if(_fpu_mode_cplusplus==1) 
  {
    INFO("***************************************************\n");
    INFO("***                                             ***\n");
    INFO("***                W A R N I N G                ***\n");
    INFO("***                                             ***\n");
    INFO("***          FPU C++ mode is enabled.           ***\n");
    INFO("***                                             ***\n");
    INFO("*** Floating-point operations will not set the  ***\n");
    INFO("*** Floating Point Status Control Reg (FPSCR)   ***\n");
    INFO("***                                             ***\n");
    INFO("***************************************************\n");
    ppcInstruction::fpu_mode_software  = 0;
    ppcInstruction::fpu_mode_cplusplus = 1;
    ppcInstruction::fpu_mode_asm_ppc   = 0;
    ppcInstruction::fpu_mode_asm_x86   = 0;
  }

  /* wcm: could rework some of this to also detect x86 assembly sometime. */

  // wcmclen: allow FPSCR register to be printed after FP operations.
  //          used in debugging and unit testing for correctness validation.
#if 0
  int debugFPSCR = configuration::getValue(":fpu:FPSCR:debug");
#else
  int debugFPSCR = 0;
#endif
  if(debugFPSCR == 1) {
      INFO("FPSCR debugging mode enabled\n");
      ppcInstruction::debugPrintFPSCR = 1;
  } else {
      ppcInstruction::debugPrintFPSCR = 0;
  }

  // init TEXT memory system
  /*const uint32 Pages = 0x20000; // 2Gig
  const uint32 inPageSize = 0x4000; //16Kpages
  uint32 Index = inPageSize>>1;
  uint32 Index2 = 1;   
  uint32 Index3 = 1;
  while(Index) {
    ++Index2;
    Index3 = (Index3 << 1)|1;
    Index = Index >> 1;
  }
  PageShift = Index2-1;
  PageMask = Index3 >> 1;
  PageSize = (1<<(PageShift));

  
  textPageArray = new uint8 *[Pages];
  Index = 0;
  for(Index=0;Index<Pages;Index++) {
    textPageArray[Index] = NULL;
    }*/

  const procStartVec &homes = proc->getFirstThreadsHomes();
  vector<ppcThread*> initialPPCThreads;
  vector<thread*> initialThreads;
  bool altBinary = 0;


  /* Threads, processors, and PIDs which get alternate binary */
  map<string, vector <processor*> > altProc;
  map<string, vector <simPID> > altPID;
  map<string, vector<ppcThread*>  > altThreads;
  vector<string> altNames;
  for (uint nh = 0; nh < homes.size(); ++nh) {
    ppcThread* iThread = new ppcThread( homes[nh].first, homes[nh].second,
			homes[nh].binaryName );
    if (homes[nh].binaryName != "") {
      const string &altName = homes[nh].binaryName;
      altNames.push_back(altName);
      altBinary = 1;
      altProc[altName].push_back(homes[nh].first);
      altPID[altName].push_back(homes[nh].second);
      altThreads[altName].push_back(iThread);
    }
    initialPPCThreads.push_back(iThread);
    initialThreads.push_back(iThread);
  }


#if 0
  if(configuration::getValue(cfgstr + ":frontEnd:ppc:realGettimeofday") == 1) {
    realGettimeofday = 1;
  } else {
    realGettimeofday = 0;
  }
#else
  realGettimeofday = 0;
#endif

#if 0
  string execFile = configuration::getStrValue(cfgstr + ":frontEnd:execFile");
  int isLLE = configuration::getValue(cfgstr + ":frontEnd:ppc:isLLE");
#else
  SST::Component::Params_t::iterator pi = paramsC.find("execFile");
  string execFile;
  if (pi != paramsC.end()) {
    execFile = pi->second;
  } else {
    WARN("No exec file specified for processor!\n");
    exit(-1);
  }
  int isLLE = 0;
#endif
  bool ret;
  if (isLLE == 1) {
    if (1||altBinary) {
      WARN("Alternative Binary not supported with LLEs");
      exit(-1);
    }
    //ret = ppcLoader::LoadLLE(execFile.c_str(), initialPPCThreads, 
    //	       (char**)0, (char**)0);
  } else {
    // do the normal load
    ret = 1;
    if ( strlen( execFile.c_str() ) ) {
      ret = ppcLoader::LoadFromDevice(execFile.c_str(), initialPPCThreads, proc,
			      (char**)0, (char**)0);

      for (uint nh = 0; nh < homes.size(); ++nh) {
	// record c++ contructor locations
	ppcThread *pt = initialPPCThreads[nh];
	pt->loadInfo.constrLoc = ppcLoader::constrLoc;
	pt->loadInfo.constrSize = ppcLoader::constrSize; 
      }
    }
#if 0
    if (ret && altBinary) {
      // load any alternate binaries

      for (unsigned int a = 0; a < altNames.size(); ++a) {
	const string &altName = altNames[a];
	// set the subset info
	ppcLoader::subProc = &altProc[altName];
	ppcLoader::subPID = &altPID[altName];
	// do the load
	//string altExecFile = configuration::getStrValue(cfgstr + ":frontEnd:altExecFile");
	string altExecFile = configuration::getStrValue(cfgstr + altName +
						":frontEnd:execFile");
	ret = ppcLoader::LoadFromDevice(altExecFile.c_str(),
					altThreads[altName], proc,
					(char**)0, (char**)0, 1);
      }

    }
#endif
  }
  if (ret == false) {
    WARN("Couldn't load %s\n", execFile.c_str());
    exit(-1);
  }
  INFO("init ppcFront initial %ld threads\n", (long int)initialThreads.size());

  return initialThreads;
}

ppcThread::~ppcThread() 
{
  if (outstandingInsts.empty() == false) {
    INFO("trying to delete a thread with Outstanding instructions!\n");
  } else {
    /*if (home) {
      home->returnFrame(registers);
      }*/
    if (ppcRegisters) delete ppcRegisters;
    if (specPPCRegisters) delete specPPCRegisters;
    threadIDMap.erase(SequenceNumber);
  }
}

//: deallocates a thread once it's dead
void ppcThread::deleteThread(thread *t)
{
  delete t;
}

#if 0
//: Return pointer to a page
//
// Returns a host pointer to the simulation page containting the
// requested address. If the page has not been accessed, it will be
// allocated and zeroed
uint8 *ppcThread::GetTextPage (const simAddress sa)
{
  simAddress saCp = sa & 0x7fffffff;
  uint32        Index = (saCp >> PageShift);
  if(!textPageArray[Index]) {
    textPageArray[Index] = new uint8[PageSize];
    memset(textPageArray[Index], 0x00, PageSize);
  }
  return(textPageArray[Index]);
}

//: Write to instruction memory
bool ppcThread::WriteTextByte(simAddress sa, uint8 Data) {
  uint8 *Page = GetTextPage(sa);
  if(!Page)     return(0);
  
  Page[sa&PageMask] = Data;
  return ( true );
}

//: Read from instruction memory
uint32 ppcThread::ReadTextWord(const simAddress sa) {
  //INFO("reading from thread imem\n");
  uint8 *Page = GetTextPage(sa);
  if(!Page) {
    WARN("read text word could not find page\n");
    return(0xffffffff);
  }
  
  Page = &Page[sa&PageMask];
  uint32        *Temp = (uint32*)Page;
  return(*Temp);
}
#endif

//: Create a thread
ppcThread::ppcThread( processor *hme, simPID p, string name ) : 
  Name(name), ShouldExit(true), _pid(p), home(hme) {

#if 0
  int tmp = configuration::getValue( name + ":frontEnd:shouldExit");
#else
  int tmp = 1;
#endif

  //if ( !_config_error ) {
    ShouldExit = tmp;
    //}

#if 0
  if ( ShouldExit ) {
    ++component::keepAlive();
  }
#endif

  _isDead = 0;
  registers = 0;
  setStack = 1;
  yieldCount = 0;
  isFuture = false;
  ppcRegisters = new ppc_regs_t;
  specPPCRegisters = new ppc_regs_t;
  regs_init(ppcRegisters);
  regs_init(specPPCRegisters);
}
//: Assimilate thread to a processor
// 
// Requests a frame from the processor, and, if necessary, unpack
// registers.
void ppcThread::assimilate(processor *hme)  {
  home = hme;
  //registers = home->requestFrame(ppcRegSize); // int, fp and altivec registers
  //unpack
  simRegister *newRegs = getRegisters();
  for (int i = 0; i < ppcRegSize; ++i) {
    newRegs[i] = packagedRegisters[i];
  }
#if 0
  if (setStack == 0) {
    INFO("setting stack - ?? in %s\n", __FILE__);
    //newRegs[1] = memory::fastMalloc(ppcMaxStackSize, StackMem) + ppcMaxStackSize;
    //setStackLimits (newRegs[1] + C_ARGSAVE_LEN, newRegs[1] - ppcMaxStackSize);

    //uint stackSize = configuration::getValue( Name + ":frontEnd:stack:size");
    //if ( _config_error ) {
    uint stackSize = ppcMaxStackSize;
      //}

    stackSize /= hme->getNumCores();

    //simAddress newStack =
    //	 configuration::getValue( Name + ":frontEnd:stack:base");
    //if ( _config_error ) {
    //simAddress newStack = memory::globalAllocate(stackSize);
    simAddress newStack = 0xc0000000;
      //}
    newStack += stackSize * hme->getCoreNum();

    DPRINT(1,"newStack=%#lx stackSize=%#lx\n",(long unsigned int)newStack,(long unsigned int)stackSize);

    newRegs[1] = newStack + stackSize;
    //setStackLimits (newRegs[1], newRegs[1] - stackSize);
    newRegs[1] -= ENVIRON_SIZE;

#if 0
    loadInfo.heap_addr = configuration::getValue( Name + ":frontEnd:heap:base");
    if ( _config_error ) {
      loadInfo.heap_addr = 0;
    }

    loadInfo.heap_len = configuration::getValue( Name + ":frontEnd:heap:size");
    if ( _config_error ) {
      loadInfo.heap_len = 0;
    }
#else
    loadInfo.heap_addr = 0;
    loadInfo.heap_len = 0;
#endif

    loadInfo.stack_addr = newStack;
    loadInfo.stack_len = stackSize;

    environInit( Name + ":frontEnd" , hme,newRegs[1],ENVIRON_SIZE);

    setStack = 1;
  }
#endif
}

//: Package a thread to migrate
//
// Copies registers from frame to internal storage. Assumes all other
// instructions have been squashed already. If needed, Retracts the current
// instruction, so when it arrives getNextInstruction() will work.
// Deallocate the old frame.
void ppcThread::packageToSend(processor *home)  {
  /// Copy stuff here and all...
  simRegister *regs = getRegisters();
  for (int i = 0; i < ppcRegSize; ++i) {
    packagedRegisters[i] = regs[i];
  }

  if (numOutstanding() > 1) {
    WARN("warning! packaging thread with outstanding instructions\n");
  }

  // retract current instruction, so next getNextInstruction works
  if (!outstandingInsts.empty()) {
    squash(outstandingInsts[0]);
  }

  // return frame
  //home->returnFrame(registers);
}

//: Retrieve next instruction
//
// Return pointer to next instruction. In this implementation, we
// can't speculate through branches, unconditional branches, or traps
// (sc). We can't speculate through traps because of the BSD
// convention for sucessful traps to advance the PC by 2.
instruction* ppcThread::getNextInstruction()
{
  if (_isDead) {
    WARN("is Dead\n");
    return 0;
  } else {
    simRegister newPC = ntohl(ntohl(ProgramCounter) + (numOutstanding()*4));
    //printf("next inst %p\n", ntohl(newPC));
#if 0
    if (!isText(newPC)) {
      WARN ("Error: new pc set to non text address %x\n", newPC);
      printCallStack();
    }
#endif

    if (newPC != 0) {
      ppcInstruction *ret = iPool.getItem();      
      ret->parent = this;
      ret->invalid = 0;
      ret->ProgramCounter = newPC;
      ret->_memEA = 0;
      instType opT = ret->getOp(ret->ProgramCounter);
      ret->_op = opT;
      ret->_state = NEW;
      outstandingInsts.push_back(ret);      
      return ret;
    } else {
      _isDead = 1;
      return 0;
    }
  }
}

//: squash
//
// conservatively squash everything. 
bool ppcThread::squash(instruction * i) {
  //printf("squash %p\n", ntohl(i->PC()));
  instList::iterator ii = find(outstandingInsts.begin(), 
			       outstandingInsts.end(), i);
  if (ii == outstandingInsts.end()) {
    set<ppcInstruction*>::iterator cIter = condemnedInsts.find((ppcInstruction*)i);
    if (cIter == condemnedInsts.end()) {
      WARN("attempt to squash an instruction from the wrong thread\n");
      return false;
    } else { // eliminate a condemed instr, returns to pool
      condemnedInsts.erase(*cIter);
      iPool.returnItem(*cIter);
      return true;
    }
  } else {
    // invalidate other instructions
    instList::iterator e = outstandingInsts.end();    
    for (instList::iterator nextI = ii;
	 (nextI != e) && ((*nextI)->invalid == 0); ++nextI) {
      (*nextI)->_state = SQUASHED;
      (*nextI)->invalid = 1;
    }
    iPool.returnItem(*ii);
    outstandingInsts.erase(ii);
    return true;
  }
}

bool ppcThread::condemn(instruction* i) {
   instList::iterator ii = find(outstandingInsts.begin(), 
			       outstandingInsts.end(), i);
  if (ii == outstandingInsts.end()) {
    WARN("attempt to condemn an instruction from the wrong thread\n");
    return false;
  } else {
    // invalidate other instructions
    instList::iterator e = outstandingInsts.end();    
    for (instList::iterator nextI = ii;
	 (nextI != e) && ((*nextI)->invalid == 0); ++nextI) {
      (*nextI)->invalid = 1;
    }
    condemnedInsts.insert(*ii);
    outstandingInsts.erase(ii);
    return true;
  }
}

//: Retire an instruction which has finished execution
bool ppcThread::retire(instruction *i) {  
  instList::iterator ii = outstandingInsts.begin();
  if (i != *ii) {
    WARN("attempt to retire an instruction from the wrong thread or OOO\n");
    WARN(" thr=%p inst=%p\n", this, i);
    WARN(" %p %s\n", (char*)(size_t)i->PC(), instNames[i->op()]);
    if (find(outstandingInsts.begin(), outstandingInsts.end(), i) 
	== outstandingInsts.end()) {
      WARN(" wrong thread\n");
    } else {
      WARN(" OOO\n");
    }
    return false;
    exit(-1);
  } else {
    //INFO("retire %p %p %s\n", (char*)ntohl(i->PC()), ntohl(i->NPC()), instNames[i->op()]);    
    ProgramCounter = i->NPC();
    (*ii)->_state = SQUASHED;
    iPool.returnItem(*ii);
    outstandingInsts.pop_front();
    return true;
  }
}

//: Write one byte to magic stack
bool ppcThread::writeStack8(const simAddress sa, const uint8 Data, 
			    const bool isSpec) 
{
  if (isSpec) {
    writeSpecStackByte(sa, Data);
    return 1;
  } else {
    simAddress s2 = getStackIdx(sa);
    if ((s2 > 0) && (s2 < (ppcMaxStackSize))) {
      stackData[s2] = Data;
      return 1;
    } else {
      WARN("stack exceeded\n");
      return 0;
    }  
  }
}

//: Write two bytes to magic stack
bool ppcThread::writeStack16(const simAddress sa, const uint16 Data,
			    const bool isSpec) 
{
  if (isSpec) {
    writeSpecStackByte(sa, Data>>8);
    writeSpecStackByte(sa+1, Data & 0xff);
    return 1;
  } else {
    simAddress s2 = getStackIdx(sa);
    if ((s2 > 0) && (s2 < (ppcMaxStackSize))) {
      uint16 *t = (uint16*)&stackData[s2];
      *t = Data;
      return 1;
    } else {
      WARN("stack exceeded\n");
      return 0;
    }
  }
}

//: Write 4 bytes to magic stack
bool ppcThread::writeStack32(const simAddress sa, const uint32 Data,
			     const bool isSpec)
{
  if (isSpec) {
    writeSpecStackByte(sa, Data>>24);
    writeSpecStackByte(sa+1, Data>>16 & 0xff);
    writeSpecStackByte(sa+2, Data>>8 & 0xff);
    writeSpecStackByte(sa+3, Data & 0xff);
    return 1;
  } else {
    simAddress s2 = getStackIdx(sa);
    if ((s2 > 0) && (s2 < (ppcMaxStackSize))) {
      uint32 *t = (uint32*)&stackData[s2];
      *t = Data;
      return 1;
    } else {
      WARN("stack exceeded %d %x\n", s2, sa);
      return 0;
    }
  }
}

bool ppcThread::CopyFromSIM(void* dest, const simAddress source, const unsigned int Bytes)
{
  if (ppcInstruction::isStack((simAddress)(size_t)source)) {  //wcm: 64b
    /* Note: this is slow, but can optimize later if its a problem*/
    simAddress BytePtr = source;
    uint8 *bDestination = (uint8*) dest;

    for (size_t i=0; i<Bytes; i++) {
      *bDestination = readStack8(BytePtr,0);
      ++BytePtr;
      ++bDestination;
    }
    return true;
  } else {
    //return processor::CopyFromSIM(dest,source,pid(),Bytes);
    return home->CopyFromSIM(dest,source,pid(),Bytes);
  }
}

bool ppcThread::CopyToSIM(simAddress dest, void* source, const unsigned int Bytes)
{
  if (ppcInstruction::isStack((simAddress)(size_t)dest)) { // wcm:: 64b
    uint8 *BytePtr = (uint8*)source;
    uint8 *bDestination = (uint8*) dest;
    bool working = 1;

    for (size_t i=0; working && i < Bytes; i++) {
      working = writeStack8((simAddress)(size_t)bDestination, (uint8)*BytePtr, 0); 
      ++BytePtr;
      ++bDestination;
    }

    if (!working) {WARN("problem writing to stack\n");}
    return working;
  } else {
    //return processor::CopyToSIM(dest, pid(), source, Bytes);
    return home->CopyToSIM(dest, pid(), source, Bytes);
  }
}

bool ppcThread::isPCValid(const simAddress testPC) {
#if 1
  return (isText(testPC));
#else
  // not very good way of checking, but for now...
  simAddress saCp = testPC & 0x7fffffff;
  uint32        Index = (saCp >> PageShift);
  if(!textPageArray[Index]) {
    return 0;
  } else {
    return 1;
  }
#endif
}

// temporary
instruction* ppcThread::getNextInstruction(const simAddress reqPC) {
  if (_isDead) {
    WARN("is Dead\n");
    return 0;
  } else {
    ppcInstruction *ret = iPool.getItem();      
    ret->parent = this;
    ret->invalid = 0;
    ret->_memEA = 0;
    ret->ProgramCounter = reqPC;
    instType opT = ret->getOp(ret->ProgramCounter);
    //INFO("getNext %p %s\n", (char*)ntohl(ret->ProgramCounter), instNames[opT]);
    ret->_op = opT;
    ret->_state = NEW;
    outstandingInsts.push_back(ret);      
    return ret;
  } 
}

void ppcThread::squashSpec() {
  specStackData.clear();
  regs_init(specPPCRegisters);
}

void ppcThread::prepareSpec() {
  memcpy(specPPCRegisters, ppcRegisters, sizeof(*ppcRegisters));
}

simAddress ppcThread::getStartPC() {
  return ProgramCounter;
}

//: Check if a seciton is const 
//
// Note: should change to an O(log n) rather than O(n)
bool ppcThread::isConstSection(const simAddress addr,
				     const simPID) const {
  if (constData.empty() == false) {
    for (vector<adrRange>::const_iterator i = constData.begin();
	 i != constData.end(); ++i) {
      if (addr < i->first) {
	return false;
      } else {
	if (/*addr >= i->first && */ addr <= i->second) return true;
      }
    }
  }

  return false;
}
  
// export pool class
typedef pool<ppcInstruction> iPool_t;
#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT_GUID(iPool_t, "pool<ppcInstruction>")

BOOST_CLASS_EXPORT(ppcThread)
#endif
