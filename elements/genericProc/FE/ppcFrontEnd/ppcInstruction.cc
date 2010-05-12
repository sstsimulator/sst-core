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


#include "ppcFront.h"
#include "ppcMachine.h"
#include "processor.h"

#define IS_CMMT 1
#ifndef WORDS_BIGENDIAN
bool ppcInstruction::little_endian       = 1;
#else
bool ppcInstruction::little_endian       = 0;
#endif

bool ppcInstruction::magicStack          = 1;
bool ppcInstruction::loadsAlwaysCheckFEB = 0;
bool ppcInstruction::storesAlwaysSetFEB  = 0;
bool ppcInstruction::allowSelfModify     = 0;
bool ppcInstruction::debugPrintFPSCR     = 0;
bool ppcInstruction::fpu_mode_software   = 0;  
bool ppcInstruction::fpu_mode_cplusplus  = 0;  
bool ppcInstruction::fpu_mode_asm_ppc    = 0;
bool ppcInstruction::fpu_mode_asm_x86    = 0;
unsigned ppcInstruction::totalCommitted  = 0;

//: Classify instruction Mask for simple scalar
//
// Converts simple scalar instruction flags to instType for the back
// end to use after the issue stage.
instType classifyMask(const unsigned int mask)
{
  if (mask&F_ICOMP) {
    return ALU;
  } else if (mask&F_UNCOND) {
    return JMP;
  } else if (mask&F_FCOMP) {
    return FP;
  } else if (mask&F_LOAD) {
    return LOAD;
  } else if (mask&F_STORE) {
    return STORE;
  } else if (mask&F_COND) {
      return BRANCH;
  } else if (mask&F_TRAP) {
    return TRAP;
  } else if (mask == 512) {
    // this is for cache-type instructions (dcb*)
    return ALU;
  } else {
    // could happen when speculating
    //printf("unclassified op type? %d\n", mask);
    return ALU;
  }
}

bool ppcInstruction::isBranchLink() const {
  switch (simOp) {
  case BL: return true;
  case BLA: return true;
  case BCL: return true;
  case BCLA: return true;
  case BCCTRL: return true;
  default: return false;
  }
}

bool ppcInstruction::isReturn() const {
  switch (simOp) {
  case BCLR: return true;
  case BCLRL: return true;
  default: return false;
  }
}

instType ppcInstruction::getOp(const simAddress sa) {
  if (parent->_isDead || ProgramCounter == 0) {
    parent->_isDead = 1;
    _op = ISDEAD;
    return ISDEAD;
  }

#if 1
  if (magicStack) {
    printf("trying to do magic stack\n");
    if (isStack(ProgramCounter)) {
      printf("executing from stack %p\n", (void*)(size_t)ProgramCounter);
      aCurrentInstruction = parent->readStack32(ProgramCounter, 0);
    } else {
      //aCurrentInstruction = parent->ReadTextWord(ProgramCounter);
    }
  } else {
    aCurrentInstruction = ntohl(parent->home->ReadMemory32(ntohl(ProgramCounter), 0));
  }
#else
  ppcThread::readText(ProgramCounter, (uint8*)&aCurrentInstruction, 4);
#endif

  MD_SET_OPCODE(simOp, aCurrentInstruction);

  unsigned int mask=0;
  switch ( simOp ) {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5)\
    case OP:								\
      mask = FLAGS;	\
    break;
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2)        \
   case OP:                   \
     printf("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT) ;
#include "powerpc.def"
   default:
       ; // this could happen if we are speculatively fetching stuff
  }

  _op = classifyMask(mask);
  _specificOp = mask;
  return _op;
}

//: Initialze instruction
//
// Because ppcInstructions are contained as part of ppcThreads, this is
// called whenever a new thread is created.
ppcInstruction::ppcInstruction(ppcThread* par) {
  parent = par;
  for (int i = 0; i < max_deps; ++i) {
    _ins[i] = _outs[i] = 0;
  }
  _ins[max_deps] = _outs[max_deps] = -1;
}

//: Instruction Fetch
bool ppcInstruction::fetch(processor *proc) {
  static bool firstInst = 0;
  //printf("ppc::fetch %p\n", (void*)PC());
  // This is ugly, but I could find no other way to do it...
  if (magicStack && firstInst == 0) {
    firstInst = 1;
     //: NOTE: When making first, thread be sure to set its stack
    //register (1) to 0x7fff0000
    (parent->getRegisters())[1] = ntohl(ppcInitStackBase);
    //proc->getFrame(parent->registers)[1] = ppcInitStackBase;
  }

  if (invalid) {
    printf("invalid inst. shoud have been squashed\n");
    return false;
  }


  _state = FETCHED;
  return true;
}

//: Accessor for opcode
instType ppcInstruction::op() const {
  return _op;
}

//: Accessor for memory effective address
simAddress ppcInstruction::memEA() const {
  return _memEA;
}

//: Accessor for PC
simAddress ppcInstruction::PC() const {
  return ProgramCounter;
}

simPID ppcInstruction::pid() const {
  return parent->pid();
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(ppcInstruction)
#endif
