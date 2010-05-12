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
#include "regs.h"
#include "ppcFront.h"
#include "processor.h"
#include <math.h>
#include "fe_debug.h"
#include "sst_stdint.h"

// KSH FIXME
// #define NOW parent->home->CurrentCycle()
// #define NOW parent->home->simulation->getCurrentSimCycle()
#define NOW 1

#define IS_CMMT 0
#define  IF_CMMT(xxx) do { } while (0)
#define  SET_GPR(xxx,yyy) do { /* ntohl(yyy) */ } while (0)
#define  GPR(xxx) ntohl(registers[xxx])
#undef PPC_FPR
#define PPC_FPR(N)    ((double)endian_swap((qword_t)FPReg[(N)]))
#define SET_VR(xxx,w,yyy) do { /* (yyy) */ } while (0)
#define VR(xxx,w) (VReg[(xxx*altivecWordSize)+w])

#define DNA                     (0)
/* PPC doesnot have Zero register semantics.  So shift dependencies by 1.
   Prserve DNA to be 0 */

/* Add register dependence decoders for PowerPC here */
#define PPC_DGPR(N)                   (N+1)
#define PPC_DFPR(N)                   (N+1+32)
#define PPC_VR(N)                     (N+70)
#define PPC_DCR                       (65)
#define PPC_DFPSCR                    (66)
#define PPC_DXER                      (67)
#define PPC_DLR                       (68)
#define PPC_DCNTR                     (69)

#undef PPC_RESET_CR0_EQ
#define PPC_RESET_CR0_EQ do { } while (0)
#undef PPC_RESET_CR0_GT
#define PPC_RESET_CR0_GT do { } while (0)
#undef PPC_RESET_CR0_LT
#define PPC_RESET_CR0_LT do { } while (0)
#undef PPC_RESET_CR0_SO
#define PPC_RESET_CR0_SO do { } while (0)
#undef PPC_RESET_CR1_FEX
#define PPC_RESET_CR1_FEX do { } while (0)
#undef PPC_RESET_CR1_FX
#define PPC_RESET_CR1_FX do { } while (0)
#undef PPC_RESET_CR1_OX
#define PPC_RESET_CR1_OX do { } while (0)
#undef PPC_RESET_CR1_VX
#define PPC_RESET_CR1_VX do { } while (0)
#undef PPC_RESET_FPSCR_FEX
#define PPC_RESET_FPSCR_FEX do { } while (0)
#undef PPC_RESET_FPSCR_FX
#define PPC_RESET_FPSCR_FX do { } while (0)
#undef PPC_RESET_FPSCR_OX
#define PPC_RESET_FPSCR_OX do { } while (0)
#undef PPC_RESET_FPSCR_UX
#define PPC_RESET_FPSCR_UX do { } while (0)
#undef PPC_RESET_FPSCR_VX
#define PPC_RESET_FPSCR_VX do { } while (0)
#undef PPC_RESET_FPSCR_VXCVI
#define PPC_RESET_FPSCR_VXCVI do { } while (0)
#undef PPC_RESET_FPSCR_VXIDI
#define PPC_RESET_FPSCR_VXIDI do { } while (0)
#undef PPC_RESET_FPSCR_VXIMZ
#define PPC_RESET_FPSCR_VXIMZ do { } while (0)
#undef PPC_RESET_FPSCR_VXISI
#define PPC_RESET_FPSCR_VXISI do { } while (0)
#undef PPC_RESET_FPSCR_VXSNAN
#define PPC_RESET_FPSCR_VXSNAN do { } while (0)
#undef PPC_RESET_FPSCR_VXSOFT
#define PPC_RESET_FPSCR_VXSOFT do { } while (0)
#undef PPC_RESET_FPSCR_VXSQRT
#define PPC_RESET_FPSCR_VXSQRT do { } while (0)
#undef PPC_RESET_FPSCR_VXVC
#define PPC_RESET_FPSCR_VXVC do { } while (0)
#undef PPC_RESET_FPSCR_VXZDZ
#define PPC_RESET_FPSCR_VXZDZ do { } while (0)
#undef PPC_RESET_FPSCR_XX
#define PPC_RESET_FPSCR_XX do { } while (0)
#undef PPC_RESET_FPSCR_ZX
#define PPC_RESET_FPSCR_ZX do { } while (0)
#undef PPC_RESET_XER_CA
#define PPC_RESET_XER_CA do { } while (0)
#undef PPC_RESET_XER_OV
#define PPC_RESET_XER_OV do { } while (0)
#undef PPC_RESET_XER_SO
#define PPC_RESET_XER_SO do { } while (0)
#undef PPC_SET_CNTR
#define PPC_SET_CNTR(EXPR)	do { /* ntohl(((EXPR))) */ } while (0)
#undef PPC_SET_CR
#define PPC_SET_CR(EXPR)	do { } while(0)
#undef PPC_SET_CR0_EQ
#define PPC_SET_CR0_EQ		do { /* (((CR)|0x20000000)) */ } while (0)
#undef PPC_SET_CR0_GT
#define PPC_SET_CR0_GT		do { /* (((CR)|0x40000000)) */ } while (0)
#undef PPC_SET_CR0_LT
#define PPC_SET_CR0_LT          do { /* (((CR)|0x80000000)) */ } while (0)
#undef PPC_SET_CR0_SO
#define PPC_SET_CR0_SO		do { /* (((CR)|0x10000000)) */ } while (0)
#undef PPC_SET_CR1_FEX
#define PPC_SET_CR1_FEX		do { /* (((CR)|0x04000000)) */ } while (0)
#undef PPC_SET_CR1_FX
#define PPC_SET_CR1_FX		do { /* (((CR)|0x08000000)) */ } while (0)
#undef PPC_SET_CR1_OX
#define PPC_SET_CR1_OX		do { /* (((CR)|0x01000000)) */ } while (0)
#undef PPC_SET_CR1_VX
#define PPC_SET_CR1_VX		do { /* (((CR)|0x02000000)) */ } while (0)
#undef PPC_CLEAR_CR6
#define PPC_CLEAR_CR6           do { /* (((CR)&0xffffff0f)) */ } while (0)
#undef PPC_SET_CR6
#define PPC_SET_CR6(xxx)        do { /* (((CR)|(xxx<<4))) */ } while (0)
#undef PPC_SET_FPSCR
#define PPC_SET_FPSCR(EXPR)	do { /* ntohl((EXPR)) */ } while (0)
#undef PPC_SET_FPSCR_FEX
#define PPC_SET_FPSCR_FEX	do { /* (((FPSCR)|0x40000000)) */ } while (0)
#undef PPC_SET_FPSCR_FPCC
#define PPC_SET_FPSCR_FPCC(EXPR)  do { /* ((((FPSCR)&0xffff0fff)|((EXPR)&0x0000f000))) */ } while (0)
#undef PPC_SET_FPSCR_FPRF
#define PPC_SET_FPSCR_FPRF(EXPR) do { /* ((((FPSCR)&0xfffe0fff)|((EXPR)&0x0001f000))) */ } while (0)
#undef PPC_SET_FPSCR_FX
#define PPC_SET_FPSCR_FX	do { /* (((FPSCR)|0x80000000)) */ } while (0)
#undef PPC_SET_FPSCR_OX
#define PPC_SET_FPSCR_OX	do { /* (((FPSCR)|0x10000000)) */ } while (0)
#undef PPC_SET_FPSCR_UX
#define PPC_SET_FPSCR_UX	do { /* (((FPSCR)|0x08000000)) */ } while (0)
#undef PPC_SET_FPSCR_VX
#define PPC_SET_FPSCR_VX	do { /* (((FPSCR)|0x20000000)) */ } while (0)
#undef PPC_SET_FPSCR_VXCVI
#define PPC_SET_FPSCR_VXCVI	do { /* (((FPSCR)|0x00000100)) */ } while (0)
#undef PPC_SET_FPSCR_VXIDI
#define PPC_SET_FPSCR_VXIDI	do { /* (((FPSCR)|0x00400000)) */ } while (0)
#undef PPC_SET_FPSCR_VXIMZ
#define PPC_SET_FPSCR_VXIMZ	do { /* (((FPSCR)|0x00100000)) */ } while (0)
#undef PPC_SET_FPSCR_VXISI
#define PPC_SET_FPSCR_VXISI	do { /* (((FPSCR)|0x00800000)) */ } while (0)
#undef PPC_SET_FPSCR_VXSNAN
#define PPC_SET_FPSCR_VXSNAN    do { /* (((FPSCR)|0x01000000)) */ } while (0)
#undef PPC_SET_FPSCR_VXSOFT
#define PPC_SET_FPSCR_VXSOFT	do { /* (((FPSCR)|0x00000400)) */ } while (0)
#undef PPC_SET_FPSCR_VXSQRT
#define PPC_SET_FPSCR_VXSQRT	do { /* (((FPSCR)|0x00000200)) */ } while (0)
#undef PPC_SET_FPSCR_VXVC
#define PPC_SET_FPSCR_VXVC	do { /* (((FPSCR)|0x00080000)) */ } while (0)
#undef PPC_SET_FPSCR_VXZDZ
#define PPC_SET_FPSCR_VXZDZ	do { /* (((FPSCR)|0x00200000)) */ } while (0)
#undef PPC_SET_FPSCR_XX
#define PPC_SET_FPSCR_XX	do { /* (((FPSCR)|0x02000000)) */ } while (0)
#undef PPC_SET_FPSCR_ZX
#define PPC_SET_FPSCR_ZX	do { /* (((FPSCR)|0x04000000)) */ } while (0)
#undef PPC_SET_LR
#define PPC_SET_LR(EXPR)		do { /* ntohl(((EXPR))) */ } while (0)
#undef PPC_SET_XER
#define PPC_SET_XER(EXPR)	 do { /* ntohl((EXPR)) */ } while (0)
#undef PPC_SET_XER_CA
#define PPC_SET_XER_CA		 do { } while (0)
#undef PPC_SET_XER_OV
#define PPC_SET_XER_OV		do { /* (((XER)|0x40000000)) */ } while (0)
#undef PPC_SET_XER_SO
#define PPC_SET_XER_SO		do { /* (((XER)|0x80000000)) */ } while (0)
#undef PPC_SET_FPR_DW
#define PPC_SET_FPR_DW(N,EXPR)    do { /* (convertDWToDouble((EXPR))) */ } while (0)
#undef PPC_SET_FPR_D
#define PPC_SET_FPR_D(N,EXPR)    do { /* ((EXPR)) */ } while (0)

#undef  PPC_SET_CR
#define PPC_SET_CR(xxx) do { /* 0 */ } while (0)

#define  SET_NPC(xxx)  _NPC = ntohl(xxx);
#define  SET_TPC(xxx)  _TPC = ntohl(xxx);
#define  CPC ProgramCounter
#define READ_BYTE(xxx,flt) isStack(xxx) ?  _memEA = 0 : _memEA = (xxx)
#define READ_HALF(xxx,flt) isStack(xxx) ?  _memEA = 0 : _memEA = (xxx)
#define READ_WORD(xxx,flt) isStack(xxx) ?  _memEA = 0 : _memEA = (xxx)
#define READ_DOUBLE(xxx,flt) isStack(xxx) ?  _memEA = 0 : _memEA = (xxx)
#define WRITE_BYTE(ddd,xxx,flt) isStack(xxx) ?  _memEA = 0 : _memEA = (xxx)
#define WRITE_HALF(ddd,xxx,flt) isStack(xxx) ?  _memEA = 0 : _memEA = (xxx)
#define WRITE_WORD(ddd,xxx,flt) isStack(xxx) ?  _memEA = 0 : _memEA = (xxx)
#define WRITE_DOUBLE(ddd,xxx,flt) isStack(xxx) ?  _memEA = 0 : _memEA = (xxx)
#define SYSCALL issueSystemTrap(proc, &CurrentInstruction)
/* ignore inlined assembly and memcopies*/
#define asm(...) ;
#define memcpy(a,b,c) ;

#undef EXEC_LWARX
#define EXEC_LWARX LWZX_IMPL;

#undef EXEC_STWCXD
#define EXEC_STWCXD STWX_IMPL;

//: Instruction Issue
//
// This function is dominated by the "big switch statement" from
// simple scalar's ss.def. 
//
// The various macros which define how ss.def is interpreted are found
// in ssInstructionIssue.cpp. None of these macros modify the state of
// the thread.
bool ppcInstruction::issue(processor *proc, const bool isSpec) {
  uint32_t &CurrentInstruction = aCurrentInstruction;
  const simRegister *registers = 
    isSpec ? parent->getSpecRegisters() : parent->getRegisters();
  const double *FPReg = (double*)(registers+32);
  const simRegister *VReg = (registers+32+64);
  const ppc_regs_t &regs = 
    isSpec ? *(parent->specPPCRegisters) : *(parent->ppcRegisters);

  if (invalid) {
    printf("invalid inst. shoud have been squashed\n");
    return false;
  }

  // fetch
  if (parent->_isDead || ProgramCounter == 0) {
    parent->_isDead = 1;
    _op = ISDEAD;
    _specificOp = 0;
    _fu = FUClass_NA;
    return true;
  }

  //CurrentInstruction = parent->ReadTextWord(ProgramCounter);
  //ppcThread::readText(ProgramCounter, (uint8_t*)&CurrentInstruction, 4);
  //MD_SET_OPCODE(simOp, CurrentInstruction);
  //printf("Issue %s %p %lld\n", MD_OP_NAME(simOp), ProgramCounter, component::Cycle());
  //unsigned int mask;
  // set default NPC
  _NPC = _TPC =  ntohl(ntohl(ProgramCounter) + 4);
  uint32_t &inst = CurrentInstruction;
  switch ( simOp ) {
#define TRAP (void)0;
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) \
    case OP:								\
      _fu = RES;							\
      SYMCAT(OP,_IMPL);							\
      _ins[0] = I1; _ins[1] = I2; _ins[2] = I3; _ins[3] = I4; _ins[4] = I5; \
      _outs[0] = O1; _outs[1] = O2; _outs[2] = O3; _outs[3] = O4; _outs[4] = O5; \
    break;
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2)        \
   case OP:                   \
     printf("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT) do { } while(0)
#define PPC_INSTRUCTION_MODE 0
#include "powerpc.def"
   default:
     if(!isSpec) {
       printf("attempted to issue a bogus opcode %x (%lx) pc=%p, cycle=%llu\n",
	       simOp, (unsigned long) CurrentInstruction, 
	      (void*)(size_t)PC(), (unsigned long long) NOW);
       printf("%p@%d: Issue %6s %p %p(%lx) %8p %8p %s%llu\n", 
	      this->parent, proc->getProcNum(), MD_OP_NAME(simOp), 
	      (void*)(size_t)ProgramCounter, (void*)(size_t)_memEA, 
	      (unsigned long) proc->ReadMemory32(_memEA,0), 
          (void*)(size_t)registers[0], 
	      (void*)(size_t)registers[31], 
	      isSpec ? "(spec)" : "", (unsigned long long) NOW); 
       printf ("Memory at %p: %x\n", 
	       (void*)(size_t)ProgramCounter, 
	       (unsigned int)proc->ReadMemory32(ProgramCounter, 0));
       printf ("Op returned from %x: %x, simOp %x\n", (unsigned int)ProgramCounter, getOp(ProgramCounter), simOp);
       //parent->printCallStack();
       fflush(stdout);
       ERROR("bogus opcode\n");
     }
#undef TRAP
  }

  if (!ppcThread::isText(ProgramCounter)) {
    printf ("Issue Instruction Program Counter set to non text addr %x\n", (unsigned int)ProgramCounter);
    ERROR("fix me\n");
#if 0
    printf("%p@%d: Issue %6s %p %p(%lx) %8p %8p %s%llu\n", 
	   this->parent, proc->getProcNum(), MD_OP_NAME(simOp), 
	   (void*)ProgramCounter, (void*)_memEA, 
	   proc->ReadMemory32(_memEA,0), (void*)registers[0], 
	   (void*)registers[31], 
	   isSpec ? "(spec)" : "", component::cycle()); 
#endif
    //parent->printCallStack();
    fflush(stdout);
    //exit(1);
  }

  _state = ISSUED;
  return true;
}

