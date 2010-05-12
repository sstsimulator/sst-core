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


#ifndef PPCTHREAD_H
#define PPCTHREAD_H

#include "sst_stdint.h"
#include "thread.h"
#include "pool.h"
#include "processor.h"
#include <utility>
#include <boost/serialization/set.hpp>

#include "loadInfo.h"

#include "sst/boost.h"
#include "sst/component.h"

#include "regs.h"

#ifdef __APPLE__
  #include <architecture/ppc/cframe.h>
#endif

static const unsigned int ppcMaxStackSize=32*1024;

#define ppcSimStackBase memory::segRange[GlobalDynamic][0]
#define ppcInitStackBase  memory::segRange[GlobalDynamic][0]+0x1000
#define getStackIdx(SA) (SA - memory::segRange[GlobalDynamic][0])

instType classifyMask(const unsigned int mask);

const int altivecWordSize = 16; //in words
const int ppcRegSize = 32+64+(32*altivecWordSize); //int, fp, and altivec registers

class processor; 
class ppcThread;

//: PowerPC instruction
//!SEC:ppcFront
class ppcInstruction : public instruction {
  friend class boost::serialization::access;
  template<class Archive> 
  void serialize(Archive & ar, const unsigned int version )
  {
      boost::serialization::
          void_cast_register(static_cast<ppcInstruction*>(NULL), 
                             static_cast<instruction*>(NULL));
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( instruction );
    ar & BOOST_SERIALIZATION_NVP(loadsAlwaysCheckFEB);
    ar & BOOST_SERIALIZATION_NVP(storesAlwaysSetFEB);
    ar & BOOST_SERIALIZATION_NVP(allowSelfModify);
    ar & BOOST_SERIALIZATION_NVP(totalCommitted);
    ar & BOOST_SERIALIZATION_NVP(parent);
    ar & BOOST_SERIALIZATION_NVP(magicStack);
    ar & BOOST_SERIALIZATION_NVP(fpu_mode_software);
    ar & BOOST_SERIALIZATION_NVP(fpu_mode_cplusplus); 
    ar & BOOST_SERIALIZATION_NVP(fpu_mode_asm_ppc);   
    ar & BOOST_SERIALIZATION_NVP(fpu_mode_asm_x86);     
    ar & BOOST_SERIALIZATION_NVP(debugPrintFPSCR);
    ar & BOOST_SERIALIZATION_NVP(_NPC);
    ar & BOOST_SERIALIZATION_NVP(_TPC);
    ar & BOOST_SERIALIZATION_NVP(_fu);
    ar & BOOST_SERIALIZATION_NVP(_specificOp);
    ar & BOOST_SERIALIZATION_NVP(_ins);
    ar & BOOST_SERIALIZATION_NVP(_outs);
    ar & BOOST_SERIALIZATION_NVP(ProgramCounter);
    ar & BOOST_SERIALIZATION_NVP(_memEA);
    ar & BOOST_SERIALIZATION_NVP(_op);
    ar & BOOST_SERIALIZATION_NVP(_exception);
    ar & BOOST_SERIALIZATION_NVP(_moveToTarget);
    ar & BOOST_SERIALIZATION_NVP(_febTarget);
    ar & BOOST_SERIALIZATION_NVP(simOp);
    ar & BOOST_SERIALIZATION_NVP(invalid);
    ar & BOOST_SERIALIZATION_NVP(aCurrentInstruction);
    ar & BOOST_SERIALIZATION_NVP(_state);
  }

  friend class ppcThread;
  static bool loadsAlwaysCheckFEB;
  static bool storesAlwaysSetFEB;
  static bool allowSelfModify;
  static unsigned totalCommitted;
  //: Thread parent
  //
  // The thread which created this instruction
  ppcThread *parent;

  static bool magicStack;

  // FPU Configuration mode
  static bool little_endian;
  static bool fpu_mode_software;   /* software IEEE compliant FPU operations             */
  static bool fpu_mode_cplusplus;  /* do op'n using C code (not IEEE compliant FPU)      */
  static bool fpu_mode_asm_ppc;    /* Use native PPC assembly when on a PPC              */
  static bool fpu_mode_asm_x86;    /* Use native x86 assembly when on intel Architecture */
  
  // Debugging options for FPSCR (useful for softFloat compatibility)
  // - important for compatibility with unit tests.
  static bool debugPrintFPSCR;

  //: Helper function
  void printRegs();

  simRegister _NPC;
  simRegister _TPC;

  int _fu;
  int _specificOp;
  static const int max_deps = 5;
  int _ins[max_deps+1];
  int _outs[max_deps+1];
  
  //: Program Counter
  simRegister   ProgramCounter;
  //: Effective address
  simAddress _memEA;
  //: Instruction opcode
  instType _op;
  //: Exception Type
  exceptType _exception;
  //: Target address for MOVE_TO exception
  simAddress _moveToTarget;
  //: Target address for FEB exception
  simAddress _febTarget;

  //: Decoded Opcode 
  //
  // We decode the opcode during instruction issue and then store it
  // here so we don't have to decode twice.
  int simOp;
  //: invald instruction
  //
  // indicates the instruction is invalid and should be squashed.
  bool invalid;

  //: Current Instruction
  uint32_t aCurrentInstruction;

  ppcInstruction(ppcThread*);
  static bool isStack(const simAddress s) {
    //if (magicStack) return (s > ppcSimStackBase && s < 0xd0000000);
    if (magicStack) return (memory::getAccType(s) == GlobalDynamic);
    else return 0;
  }
  void issueSystemTrap(processor*, uint32_t *At);
  bool commitSystemTrap(processor*, uint32_t *At, simRegister &nextPC);
#include "ppcSystemTrapHandler.h"
  instType getOp(const simAddress);
  string op_args(uint32_t, simRegister*, ppc_regs_t&) const;
  instState _state;

  uint8_t CommitReadByte (simAddress sa, bool specRd, processor *proc);
  uint16_t CommitReadHalf (simAddress sa, bool specRd, processor *proc);
  uint32_t CommitReadWord (simAddress sa, bool specRd, processor *proc);
  uint64_t CommitReadDouble (simAddress sa, bool specRd, processor *proc);

  bool CommitWriteByte (simAddress sa, uint8_t dd, bool specRd, processor *proc);
  bool CommitWriteHalf (simAddress sa, uint16_t dd, bool specRd, processor *proc);
  bool CommitWriteWord (simAddress sa, uint32_t dd, bool specRd, processor *proc);
  bool CommitWriteDouble (simAddress sa, uint64_t dd, bool specRd, processor *proc);

 public:
  ppcInstruction() : parent(0) {;}
  virtual bool fetch(processor*);
  virtual bool issue(processor* p) {return issue(p,0);}
  virtual bool commit(processor* p) {return commit(p,0);}

  virtual instState state() const {return _state;}
  virtual simAddress PC() const;
  virtual instType op() const;
  virtual simAddress memEA() const;
  //: Accessor for exception
  virtual exceptType exception() const {return _exception;};
  //: Accessor for moveToTarget
  virtual simAddress moveToTarget() const {return _moveToTarget;}
  //: Accessor for FEB target
  virtual simAddress febTarget() const {return _febTarget;}

  virtual simRegister NPC() const {return _NPC;}
  virtual simRegister TPC() const {return _TPC;}
  virtual bool issue(processor *p, const bool s);
  virtual bool commit(processor *p, const bool s);
  virtual int fu() const {return _fu;}
  virtual int specificOp() const {return _specificOp;}
  virtual const int* outDeps() const {return _outs;}
  virtual const int* inDeps() const {return _ins;}
  virtual bool isReturn() const;
  virtual bool isBranchLink() const;
  virtual simPID pid() const;
};

struct  ppc_regs_t;

//: PowerPC Thread
//
// This thread class uses a 'magic stack' - stack memory references do
// not go through the normal interface.
//
//!SEC:ppcFront
class ppcThread : public thread {
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version )
  {
    boost::serialization::
        void_cast_register(static_cast<ppcThread*>(NULL),
                           static_cast<thread*>(NULL));
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(thread);
    ar & BOOST_SERIALIZATION_NVP(loadInfo); 
    ar & BOOST_SERIALIZATION_NVP(Name);
    ar & BOOST_SERIALIZATION_NVP(ShouldExit);
    ar & BOOST_SERIALIZATION_NVP(threadIDMap);
    ar & BOOST_SERIALIZATION_NVP(nextThreadID);
    ar & BOOST_SERIALIZATION_NVP(exitSysCallExitsAll);
    ar & BOOST_SERIALIZATION_NVP(realGettimeofday);
    ar & BOOST_SERIALIZATION_NVP(verbose);
    ar & BOOST_SERIALIZATION_NVP(ThreadID);
    ar & BOOST_SERIALIZATION_NVP(isFuture);
    ar & BOOST_SERIALIZATION_NVP(SequenceNumber);
    ar & BOOST_SERIALIZATION_NVP(_pid);
    ar & BOOST_SERIALIZATION_NVP(iPool);
    ar & BOOST_SERIALIZATION_NVP(ProgramCounter);
    ar & BOOST_SERIALIZATION_NVP(setStack);
    ar & BOOST_SERIALIZATION_NVP(ppcRegisters);  
    ar & BOOST_SERIALIZATION_NVP(specPPCRegisters); 
    ar & BOOST_SERIALIZATION_NVP(packagedRegisters);
    ar & BOOST_SERIALIZATION_NVP(outstandingInsts);
    ar & BOOST_SERIALIZATION_NVP(condemnedInsts);
    ar & BOOST_SERIALIZATION_NVP(registers);
    ar & BOOST_SERIALIZATION_NVP(home);
    ar & BOOST_SERIALIZATION_NVP(yieldCount);
    ar & BOOST_SERIALIZATION_NVP(myFrame);    
  }

  LoadInfo loadInfo;

  string Name;
  bool ShouldExit;
  friend class ppcInstruction;
  friend class ppcLoader;
  static const bool usingMagicStack() {return ppcInstruction::magicStack;}
  static map<uint32_t, ppcThread*> threadIDMap;
  static unsigned int nextThreadID;
  static bool exitSysCallExitsAll;
  //: real gettimofday or not
  static bool realGettimeofday;
  //: Print more info about instructions
  static int verbose;
  simAddress ThreadID;
  bool isFuture;
  uint32_t SequenceNumber;
  simPID _pid;
  //: instruction pool
  static pool<ppcInstruction> iPool;
  //: reservation set
  //
  // set of reserved addresses (for LWARX/STCWX instructions)
  typedef pair<simAddress, simPID> addrPair;
  typedef map<addrPair, ppcThread*> reservedSetT;
  static reservedSetT reservedSet;
  //: Program Counter
  simRegister   ProgramCounter;
  bool setStack;
  //: PowerPC special registers
  ppc_regs_t *ppcRegisters;
  ppc_regs_t *specPPCRegisters;
  //: Storage space for frames
  //
  // Threads which need to migrate/travel, package thier frame's state
  // into this region before being migrated. Upon arriving at the
  // destination, it allocates a new frame and "unpacks" itself.
  //
  // Also, used for speculative register scratch space
  simRegister packagedRegisters[ppcRegSize];

#if 0
  //: Text memory
  //
  // NOTE: Ideally, this should be replaced by having ppcThread inherit
  // from the memory class
  //static uint8_t  **textPageArray;
  //: Size of instruction pages
  static uint32_t PageSize;
  //: Instruction page shift
  static uint32_t PageShift;
  //: Instruction page mask
  static uint32_t PageMask;
  static uint8_t *GetTextPage (const simAddress sa);
#endif

  typedef deque<ppcInstruction*> instList;
  //: Current ppcInstructions
  //
  // It is an collection of all the outstanding instructions. new
  // instructions are placed in the back, and retired out the front.
  // Only the top instruction can be retired to ensure proper
  // ordering.
  //
  // Probably this should be a list for performance. Change it at
  // some point and also add an accounting feature so the
  // numOutstanding() function isn't slow.
  instList outstandingInsts;
  set<ppcInstruction*> condemnedInsts;
  uint numOutstanding() const {return outstandingInsts.size();}
  instList &getOutstandingInsts() {return outstandingInsts;}
  //: FrameID for thread's register set
  frameID registers;
  //: Home processor of thread
  //
  // This is the processor where the thread is executing. 
  processor *home;
  // yield count
  int yieldCount;

  //: my register set
  simRegister myFrame[ppcRegSize];
  //: Get reference to register set
  simRegister *getRegisters() {return myFrame;}
  simRegister *getSpecRegisters() {return packagedRegisters;}

  //: copy date to the simulated memory
  //
  // ppcthread needs its own CopyToSim because some data memory may be
  // on the stack.
  bool CopyToSIM(simAddress dest, void* source, const unsigned int Bytes);

  //: copy date from the simulated memory
  //
  // ppcthread needs its own CopyFromSim because some data memory may be
  // on the stack.
  bool CopyFromSIM(void* dest, const simAddress source, const unsigned int Bytes);

  uint8_t getSpecStackByte(const simAddress sa) {
    map<simAddress, uint8_t>::iterator i = specStackData.find(sa);
    if (i != specStackData.end()) {
      return i->second;
    } else {
      simAddress s2 = getStackIdx(sa);
      if (s2 <= ppcMaxStackSize) {
	return stackData[s2];
      } else {
	return 0;
      }
    }
  }

  void writeSpecStackByte(const simAddress sa, const uint8_t Data) {
    specStackData[sa] = Data;
  }

  //: Read 1 byte from the "magic" stack
  uint8_t readStack8(const simAddress sa, const bool isSpec) {
    if (isSpec) {
      return getSpecStackByte(sa);
    } else {
      simAddress s2 = getStackIdx(sa);
      return stackData[s2];
    }
  }
  //: Read 2 bytes from the "magic" stack
  uint16_t readStack16(const simAddress sa, const bool isSpec) {
    if (isSpec) {
      uint16_t r = (getSpecStackByte(sa) << 8);
      r += getSpecStackByte(sa+1);
      return r;
    } else {
      simAddress s2 = getStackIdx(sa);
      uint16_t *t = (uint16_t*)&stackData[s2];
      return (uint16_t)(*t);
    }
  }
  //: Read 4 bytes from the "magic" stack
  uint32_t readStack32(const simAddress sa, const bool isSpec) {
    if (isSpec) {
      uint32_t r = (getSpecStackByte(sa) << 24);
      r += getSpecStackByte(sa+1) << 16;
      r += getSpecStackByte(sa+2) << 8;
      r += getSpecStackByte(sa+3);
      return r;
    } else {
      simAddress s2 = getStackIdx(sa);
      if (s2 >= ppcMaxStackSize) {
	printf("overstack %p %d > %d\n",
		(void*)(size_t)sa,s2,ppcMaxStackSize);
      }
      uint32_t *t = (uint32_t*)&stackData[s2];
      return (uint32_t)(*t);
    }
  }
  bool writeStack8(const simAddress, const uint8_t, const bool isSpec);
  bool writeStack16(const simAddress, const uint16_t, const bool isSpec);
  bool writeStack32(const simAddress, const uint32_t, const bool isSpec);

  typedef pair<simAddress,simAddress> adrRange;
  static vector<adrRange> constData;

 public:
  static vector<thread*> init(processor *p, SST::Component::Params_t& paramsC);
  static void deleteThread(thread *t);
#if 0
  static bool WriteTextByte(simAddress bDestination, uint8_t Byte); 
  static uint32_t ReadTextWord(const simAddress bDestination);
#endif

  ppcThread( processor* hme, simPID, string name = "");
  ppcThread() {;}
  virtual ~ppcThread();
  //: Accessor for program counter
  simRegister GetProgramCounter() {return(ProgramCounter);}
  //: Magic Stack storage
  uint8_t         stackData[ppcMaxStackSize];
  //: speculative stack
  map<simAddress, uint8_t> specStackData;

#if 0
  //: Program Text
  static uint8_t *progText;
  static simAddress lastTextAddr;
  static vector<simAddress> textRanges;
#endif  

  static bool isText (simAddress addr) {
#if 1
    return true;
#else
    if (addr > lastTextAddr)
      return false;

    int ranges = textRanges.size() >> 1;
    for (int i  = 0; i < ranges; i += 2) {
      if ((addr >= textRanges[i]) && (addr < textRanges[i+1]))
	return true;
    }
    return false;
#endif
  }

#if 0
  static bool writeText (simAddress addr, uint8_t *data, unsigned int size) {
    if (lastTextAddr < (addr + size)) {
      lastTextAddr = addr + size;
      progText = (uint8_t*)realloc(progText, lastTextAddr);
    }
    textRanges.push_back(addr);
    textRanges.push_back(addr + size);

    for (int i = 0; i < size; i++) {
      progText[i+addr] = data[i];
    }
    return true;
  }
  static bool readText (simAddress addr, uint8_t *buffer, unsigned int size) {
    if (addr > lastTextAddr)
      return false;
    for (int i = 0; i < size; i++) {
      buffer[i] = progText[addr+i];
    }
    return true;
  }
#endif	

  virtual instruction* getNextInstruction();
  virtual bool squash(instruction * i);
  virtual bool retire(instruction *); 
  virtual void assimilate(processor *);
  virtual void packageToSend(processor *);

  virtual simAddress getStartPC();
  virtual bool isPCValid(const simAddress);
  virtual instruction* getNextInstruction(const simAddress);
  virtual void squashSpec();
  virtual void prepareSpec();
  virtual bool condemn(instruction*);
  simPID pid() const {return _pid;};
  virtual void changePid(const simPID p){ _pid = p;}
  int getInstructionSize() {return 4;}

  simRegister getStack() const {
    if (home) {
      return  myFrame[1];
    } else {
      return packagedRegisters[1];
    }
  }

  bool isConstSection(const simAddress, const simPID) const;
};

#endif 
