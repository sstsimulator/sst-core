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

#define NOW parent->home->getCurrentSimTime()

#define IS_CMMT 1 // allows for alternate implementations based on phase
/* this statement is used to isolate things that should ONLY be done during
 * commit (e.g. computing actual results) */
#define  IF_CMMT(xxx) xxx
#define  SET_GPR(xxx,yyy) (registers[xxx]=htonl(yyy))
#define  GPR(xxx) ntohl(registers[xxx])
#undef PPC_FPR
#define PPC_FPR(N)    (convertDWToDouble(endian_swap(readWhole(FPReg[(N)]))))

#undef PPC_SET_FPR_DW
// when EXPR is type quad
#define PPC_SET_FPR_DW(N,EXPR)    (FPReg[(N)] = convertDWToDouble(endian_swap((EXPR))))
#undef PPC_SET_FPR_D
// when EXPR is type double
#define PPC_SET_FPR_D(N,EXPR)    (FPReg[(N)] = convertDWToDouble(endian_swap(readWhole(EXPR))))

//#define SET_TPC(xxx) (_TPC=(xxx))
#define SET_TPC(xxx) (void)0

#define SET_VR(xxx,w,yyy) (VReg[(xxx*altivecWordSize)+w] = (yyy))
#define VR(xxx,w) (VReg[(xxx*altivecWordSize)+w])

#define  SET_NPC(xxx)  (NextProgramCounter=ntohl(xxx))
#define  CPC ntohl(ProgramCounter)

//: Used to determine if FEBs are full, if we are checking such things
//
// Note: we have to do the CHECK_FULL _after_ the READ_* because the
// def file uses "result = READ_XXX()". Thus, the read has to return
// something.

#define CHECK_FULL(xxx)	\
  if(loadsAlwaysCheckFEB && (isSpec==0) && proc->getFE(xxx)==0) {	\
    didCommit = 0;							\
    _exception=FEB_EXCEPTION;						\
    _febTarget=xxx;							\
    break;								\
  }

#define SET_FULL(xxx) if (storesAlwaysSetFEB && (isSpec==0)) {proc->setFE(xxx,1);}

#define IS_TEXT(xxx) 0
//ppcThread::isText(xxx)

#define TEXT_RD_MEM(xxx,ddd) 0
//ppcThread::readText(xxx,(uint8_t*)&ddd,sizeof(ddd))

#define TEXT_WR_MEM(xxx,ddd) 0
//(allowSelfModify) ?							
//  ppcThread::writeText(xxx,(uint8_t*)ddd,sizeof(ddd)) :			
//  fprintf (stderr, "!!!!  Tried to write text %x mem to sim\n", xxx)


uint8_t 
ppcInstruction::CommitReadByte (simAddress sa, bool isSpec, processor *proc) 
{
  uint8_t ret = (IS_TEXT(sa)) ? TEXT_RD_MEM(sa, ret) :
    (isStack(sa) ? parent->readStack8(sa, isSpec) : 
     proc->ReadMemory8(sa, isSpec));
  //parent->checkAccess(sa);
  return ret;
}
 
uint16_t 
ppcInstruction::CommitReadHalf (simAddress sa, bool isSpec, processor *proc) 
{
  uint16_t ret;
  //  ret = (IS_TEXT(sa)) ? TEXT_RD_MEM(sa, ret) :
  //    (isStack(sa) ? parent->readStack16(sa, isSpec) : 
  //     proc->ReadMemory16(sa, isSpec));
  if(IS_TEXT(sa)) {
    ret = TEXT_RD_MEM(sa,ret);
  } else if(isStack(sa)) {
    ret = parent->readStack16(sa,isSpec);
  } else {
    ret = proc->ReadMemory16(sa,isSpec);
  }
  //parent->checkAccess(sa);
  return ret;
}     

uint32_t 
ppcInstruction::CommitReadWord(simAddress sa, bool isSpec, processor *proc) 
{
  uint32_t ret;
  // ret = (IS_TEXT(sa)) ? TEXT_RD_MEM(sa, ret) :
  //   (isStack(sa) ? parent->readStack32(sa, isSpec) : 
  //    proc->ReadMemory32(sa, isSpec));
#if 0
  if(IS_TEXT(sa)) {
    ret = TEXT_RD_MEM(sa,ret);
  } else if(isStack(sa)) {
    ret = parent->readStack32(sa,isSpec);
  } else {
#endif
    ret = proc->ReadMemory32(sa,isSpec);
#if 0
  }
#endif
  //parent->checkAccess(sa);
  return ret;
} 

uint64_t
ppcInstruction::CommitReadDouble(simAddress sa, bool isSpec, processor *proc) 
{
  uint64_t ret;
  ret = proc->ReadMemory64(sa,isSpec);
  return ret;
} 

#define READ_BYTE(xxx,flt) CommitReadByte(xxx,isSpec,proc); CHECK_FULL(xxx); 
#define READ_HALF(xxx,flt) ntohs(CommitReadHalf(xxx,isSpec,proc)); CHECK_FULL(xxx); 
#define READ_WORD(xxx,flt) ntohl(CommitReadWord(xxx,isSpec,proc)); CHECK_FULL(xxx); 
#define READ_DOUBLE(xxx,flt) endian_swap(CommitReadDouble(xxx,isSpec,proc)); CHECK_FULL(xxx); 

bool 
ppcInstruction::CommitWriteByte (simAddress sa, uint8_t dd, bool isSpec, processor *proc) 
{
  bool ret = (IS_TEXT(sa)) ? TEXT_WR_MEM(sa, ret) :
    (isStack(sa) ? parent->writeStack8(sa, dd, isSpec) : 
     proc->WriteMemory8(sa, dd, isSpec));
  //parent->checkAccess(sa);
  return ret;
}
 
bool 
ppcInstruction::CommitWriteHalf (simAddress sa, uint16_t dd, bool isSpec, processor *proc) 
{
  bool ret = (IS_TEXT(sa)) ? TEXT_WR_MEM(sa, ret) :
    (isStack(sa) ? parent->writeStack16(sa, dd, isSpec) : 
     proc->WriteMemory16(sa, dd, isSpec));
  //parent->checkAccess(sa);
  return ret;
}     

bool 
ppcInstruction::CommitWriteWord (simAddress sa, uint32_t dd, bool isSpec, processor *proc) 
{
  bool ret = (IS_TEXT(sa)) ? TEXT_WR_MEM(sa, ret) :
    (isStack(sa) ? parent->writeStack32(sa, dd, isSpec) : 
     proc->WriteMemory32(sa, dd, isSpec));
  //parent->checkAccess(sa);
  return ret;
} 

bool 
ppcInstruction::CommitWriteDouble (simAddress sa, uint64_t dd, bool isSpec, processor *proc) 
{
  bool ret = (IS_TEXT(sa)) ? TEXT_WR_MEM(sa, ret) :
    (isStack(sa) ? parent->writeStack32(sa, dd, isSpec) : 
     proc->WriteMemory64(sa, dd, isSpec));
  //parent->checkAccess(sa);
  return ret;
} 

#define WRITE_BYTE(ddd,xxx,flt)  CommitWriteByte(xxx,ddd,isSpec,proc); SET_FULL((xxx));
#define WRITE_HALF(ddd,xxx,flt)  CommitWriteHalf(xxx,ntohs(ddd),isSpec,proc); SET_FULL((xxx));
#define WRITE_WORD(ddd,xxx,flt)  CommitWriteWord(xxx,ntohl(ddd),isSpec,proc); SET_FULL((xxx));
#define WRITE_DOUBLE(ddd,xxx,flt)  CommitWriteDouble(xxx,endian_swap(ddd),isSpec,proc); SET_FULL((xxx));

/*
#define WRITE_BYTE(ddd,xxx,flt) isStack(xxx) ? parent->writeStack8(xxx,ddd, isSpec) : proc->WriteMemory8(xxx, ddd, isSpec); SET_FULL(xxx);
#define WRITE_HALF(ddd,xxx,flt) isStack(xxx) ? parent->writeStack16(xxx,ddd, isSpec) : proc->WriteMemory16(xxx, ddd, isSpec); SET_FULL(xxx);
#define WRITE_WORD(ddd,xxx,flt) isStack(xxx) ? parent->writeStack32(xxx,ddd, isSpec) : proc->WriteMemory32(xxx, ddd, isSpec); SET_FULL(xxx);

#define READ_BYTE(xxx,flt) isStack(xxx) ? parent->readStack8(xxx, isSpec) : proc->ReadMemory8(xxx, isSpec); CHECK_FULL(xxx); 
#define READ_HALF(xxx,flt) isStack(xxx) ? parent->readStack16(xxx, isSpec) : proc->ReadMemory16(xxx, isSpec); CHECK_FULL(xxx);
#define READ_WORD(xxx,flt) isStack(xxx) ? parent->readStack32(xxx, isSpec) : proc->ReadMemory32(xxx, isSpec); CHECK_FULL(xxx); 
*/



#define SYSCALL didCommit = commitSystemTrap(proc, &CurrentInstruction, NextProgramCounter)

// 32-byte cache line clear
#undef EXEC_DCBZ
#define EXEC_DCBZ(EA)					\
  {							\
    simAddress blockAddr = EA & ~0x1f;			\
    /*printf("clearing %p %p\n", EA, blockAddr);*/	\
    for (int i = 0; i < 8; ++i) {			\
      WRITE_WORD(0,blockAddr+(i*4),flt);		\
    }							\
  }

/* We change the sematics for reservations a bit. The lwarx makes a
   reservation which includes the thread which is reserving. stwcx
   only performs the store if the reservation is held by the right
   thread. Because sometimes a thread makes a lwarx without a stwcx
   (silly IBM smp library) we can deadlock, so if the reservation is
   made in someone else's name, the next thread to make the lwarx has
   a 1:1024 chance to steal the reservation.
*/
#undef EXEC_LWARX
#define EXEC_LWARX \
  {									\
    word_t _result;                                                     \
    word_t ea;								\
    enum md_fault_type _fault;						\
    if(RA==0) {								\
	ea = GPR(RB);							\
    } else {								\
	ea = GPR(RA)+GPR(RB);						\
    }									\
    _result = READ_WORD(ea, _fault);					\
    if (_fault != md_fault_none)					\
      DECLARE_FAULT(_fault);						\
    SET_GPR(RD, _result);						\
    if (isSpec == 0) {							\
      ppcThread::addrPair resAddr(ea, parent->_pid);			\
      ppcThread::reservedSetT &rSet = ppcThread::reservedSet;		\
      if (rSet.find(resAddr) != rSet.end()) {				\
	/* someone already has it */					\
	static int steal = 0;						\
	steal++;							\
	if ((steal & 0x3ff) == 0) {					\
	  rSet[resAddr] = parent;					\
	  ERROR("stealing %p-%ld for %p @ %llu\n",                      \
                (void*)(size_t)ea, (long int) parent->_pid, parent, \
	        (unsigned long long) NOW);							\
	} else {							\
	  _exception = YIELD_EXCEPTION;					\
	  didCommit = false;						\
	}								\
      } else {								\
	rSet[resAddr] =	parent;						\
	/*printf("reserving %p-%ld for %p\n", (void*)ea, parent->_pid, parent);*/ \
      }									\
    }									\
  }

/* We change the sematics for reservations a bit. The lwarx makes a
   reservation for the address and notes which thread is reserving.
   The store only goes through if the right thread has the
   reservation. */
#undef EXEC_STWCXD			      
#define EXEC_STWCXD							\
   {                                                                    \
    word_t _src;                                                        \
    enum md_fault_type _fault;                                          \
    _src = (word_t)GPR(RS);                                     	\
    word_t ea;								\
                                                                        \
    if(RA==0){                                                          \
      ea = GPR(RB);							\
    } else {								\
      ea = GPR(RA) + GPR(RB);						\
    }									\
    ppcThread::addrPair srchAddr(ea, parent->_pid);			\
    ppcThread::reservedSetT::iterator si =				\
      ppcThread::reservedSet.find(srchAddr);				\
    if (si != ppcThread::reservedSet.end() && (si->second == parent)) {	\
      /*it is reserved for us, do the write*/				\
      WRITE_WORD(_src, ea, _fault);					\
      if (_fault != md_fault_none)					\
	DECLARE_FAULT(_fault);						\
      PPC_SET_CR0_EQ;							\
      /* unreserve */							\
      ppcThread::reservedSet.erase(si);					\
    } else {								\
      /*printf("lost reservation for %p\n", (void*)ea);*/		\
      PPC_RESET_CR0_EQ;							\
    }									\
									\
    PPC_RESET_CR0_GT;							\
    PPC_RESET_CR0_LT;							\
    if ((PPC_GET_XER_SO) == 1) {                                        \
      PPC_SET_CR0_SO;							\
    } else {								\
      PPC_RESET_CR0_SO;							\
    }									\
   }


// wcm: why were these done with space there?  not even valid.  
//      changing to contiguous word to match what's in powerpc.def
//#undef EXEC DCBI
//#define EXEC DCBI(EA) proc->dataCacheInvalidate(EA);
#undef EXEC_DCBI
#define EXEC_DCBI(EA) proc->dataCacheInvalidate(EA);

#undef EXEC_SYNC
//#define EXEC_SYNC if( proc->wcUnit() ) proc->wcUnit()->flushAll() 
#define EXEC_SYNC ;

/* This is to make debugging commit statements easier to read, and above all
 * more useful. It's not really intended for anything else, so, no complaining!
 *
 * This function takes an instruction and a set of registers and produces the
 * list of arguments that the instruction has been given along with their
 * current values (i.e. if they're registers). */
string ppcInstruction::op_args(uint32_t op, simRegister *registers, ppc_regs_t &regs) const
{
    /* the logic (though not the functionality) of this function is shamelessly
     * stolen from md_print_insn in ppcMachine.cc (from the ORIGINAL SST).
     * LONG LIVE SST V1.0!!! */
    const uint32_t &CurrentInstruction = aCurrentInstruction;
    const uint32_t &inst = CurrentInstruction;
    const double * fpreg = (double*)(registers+32);
    const char *s = MD_OP_FORMAT(op);
    string o("");
    char tmp[100] = {0};
    while (*s) {
	switch (*s) {
	    case 'a': snprintf(tmp, 100, "r%d[0x%08x]", RA, GPR(RA)); break;
	    case 'b': snprintf(tmp, 100, "r%d[0x%08x]", RB, GPR(RB)); break;
	    case 'c': snprintf(tmp, 100, "r%d[0x%08x]", RC, GPR(RC)); break;
	    case 'd': snprintf(tmp, 100, "r%d[0x%08x]", RD, GPR(RD)); break;
	    case 'f': if (BO == 12 && BI == 0) { // conditional branch type
			  snprintf(tmp, 100, "[lt]");
		      } else if (BO == 4 && BI == 10) {
			  snprintf(tmp, 100, "[ne]");
		      } else if (BO == 16 && BI == 0) {
			  snprintf(tmp, 100, "[dnz]");
		      } else if (((BO>>1)&0xf) == 0) {
			  snprintf(tmp, 100, "[if CTR-1 & false]");
		      } else if (((BO>>1)& 0xf) == 1) {
			  snprintf(tmp, 100, "[if CTR-1==0 & false]");
		      } else if (((BO>>2)&0x7) == 1) {
			  snprintf(tmp, 100, "[if false]");
		      } else if (((BO>>1)&0xf) == 4) {
			  snprintf(tmp, 100, "[if CTR-1 & true]");
		      } else if (((BO>>1)&0xf) == 5) {
			  snprintf(tmp, 100, "[if CTR-1==0 & true]");
		      } else if (((BO>>2)&0x7) == 3) {
			  snprintf(tmp, 100, "[if true]");
		      } else if (((BO>>1)&0x3) == 0 && (BO&0x10)) {
			  snprintf(tmp, 100, "[if CTR-1]");
		      } else if (((BO>>1)&0x3) == 1 && (BO&0x10)) {
			  snprintf(tmp, 100, "[if CTR-1==0]");
		      } else if ((BO&0x4) && (BO&0x10)) {
			  snprintf(tmp, 100, "[always]");
		      } else {
			  snprintf(tmp, 100, "%d[unk]", BO);
		      }
		      break;
	    case 'g': snprintf(tmp, 100, "cr%d[%c]", BI, ((ntohl(regs.regs_C.cr)>>(31-BI))&1)?'1':'0'); break; // branch condition bit
	    case 'h': snprintf(tmp, 100, "%d", SH); break; // rotate # bits
	    case 'i': snprintf(tmp, 100, "0x%x", IMM); break; // immediate value (e.g. for addi)
	    case 'j': snprintf(tmp, 100, "%#-7x", SEXT26(LI)+CPC); break; // immediate jump (show target addr)
	    case 'k': snprintf(tmp, 100, "%#-7x", (BD<<2)+CPC); break; // conditional branch target
	    case 'l': snprintf(tmp, 100, "%d", ISSETL); break; // sign extended flag for cmp*; must be 0 for 32-bit
	    case 'm': snprintf(tmp, 100, "%d", MB); break; // start bit mask
	    case 'e': snprintf(tmp, 100, "%d", ME); break; // end bit mask
	    case 'o': snprintf(tmp, 100, "0x%x", OFS); break; // offset (e.g. for lwz target addr)
	    case 's': snprintf(tmp, 100, "r%d[0x%08x]", RS, GPR(RS)); break; // store target addr
	    case 't': snprintf(tmp, 100, "%d", TO); break;
	    case 'u': snprintf(tmp, 100, "0x%x", UIMM); break; // immediate unsigned (e.g. for ori)
	    case 'w': snprintf(tmp, 100, "cr%u[%c]", CRFS, ((ntohl(regs.regs_C.cr)>>(31-CRFS))&1)?'1':'0'); break;
	    case 'x': snprintf(tmp, 100, "cr%u[%c]", CRBD, ((ntohl(regs.regs_C.cr)>>(31-CRBD))&1)?'1':'0'); break;
	    case 'y': snprintf(tmp, 100, "cr%u[%c]", CRBA, ((ntohl(regs.regs_C.cr)>>(31-CRBA))&1)?'1':'0'); break;
	    case 'z': snprintf(tmp, 100, "cr%u[%c]", CRBB, ((ntohl(regs.regs_C.cr)>>(31-CRBB))&1)?'1':'0'); break;
#define Q_FPR(N) (convertDWToDouble(endian_swap(readWhole(fpreg[(N)]))))
	    case 'A': snprintf(tmp, 100, "f%d[%g]", FA, Q_FPR(FA)); break;
	    case 'B': snprintf(tmp, 100, "f%d[%g]", FB, Q_FPR(FB)); break;
	    case 'C': snprintf(tmp, 100, "f%d[%g]", FC, Q_FPR(FC)); break;
	    case 'D': snprintf(tmp, 100, "f%d[%g]", FD, Q_FPR(FD)); break;
	    case 'S': snprintf(tmp, 100, "f%d[%g]", FS, Q_FPR(FS)); break;
	    case 'N': snprintf(tmp, 100, "%db[r%i-r%i]", NB, RA, RA+(int)ceil(NB/4.0)); break;
	    case 'M': snprintf(tmp, 100, "%#x", MTFSFI_FM); break; // field mask
	    case 'P': // mfspr's target spr
	    case 'p': // mtspr's target spr
		      switch (SPRVAL) {
			  case 1: snprintf(tmp, 100, "xer[%#08x]", XER); break;
			  case 8: snprintf(tmp, 100, "lr[%#08x]", LR); break;
			  case 9: snprintf(tmp, 100, "ctr[%#08x]", CNTR); break;
				  // XXX: we don't handle timestamp junk
			  default: snprintf(tmp, 100, "%d[UNKNOWN_SPR]", SPRVAL); break;
		      }
		      break;
	    case 'r': snprintf(tmp, 100, "cr%d[%c]", CRFD, ((ntohl(regs.regs_C.cr)>>(31-CRFD))&1)?'1':'0'); break;
	    case 'R': snprintf(tmp, 100, "0x%02x", CRM); break; // field mask (e.g. for mtcrf)
	    case 'U': snprintf(tmp, 100, "0x%x", UIMM); break;
	    case ' ': break;
	    default: /* anything unrecognized */
		o += *s;
	}
	o += tmp;
	tmp[0] = 0;
	s++;
    }
    return o;
}

static void strswap(char *str, char find, char repl) {
    while (*str) { if (*str == find) { *str = repl; } str++; }
}
//: Instruction Commit
//
// This function is dominated by the "big switch statement" from
// simple scalar's ss.def. If the instruction commits, it will update
// the program counter and registers.
//
// The various macros which define how ss.def is interpreted are found
// in ssInstructionCommit.cpp.
bool ppcInstruction::commit(processor *proc, const bool isSpec) 
{
  uint32_t &CurrentInstruction = aCurrentInstruction;
  simRegister *registers = 
    isSpec ? parent->getSpecRegisters() : parent->getRegisters();
  double *FPReg = (double*)(registers+32);
  simRegister *VReg = (registers+32+64);

  ppc_regs_t &regs = 
    isSpec ? *(parent->specPPCRegisters) : *(parent->ppcRegisters);

  simRegister NextProgramCounter = ntohl(ntohl(ProgramCounter) + 4);
  
  //simAddress old_lr = regs.regs_L;

  /* Various checks */
  if (invalid) {
    printf("invalid inst. shoud have been squashed\n");
    return false;
  }

  if (parent->_isDead || ProgramCounter == 0) {
    parent->_isDead = 1;
    _op = ISDEAD;
    return true;
  }

  bool didCommit = true;
  if (ppcThread::verbose > 5) {
    if (1) { 
      /*int rd = RD;
      int ra = RA;
      int rb = RB;
      uint32_t &inst = CurrentInstruction;
      uint cpc = CPC;
      uint li = LI;*/
      char tmp[50] = {0};
      if (_memEA != 0) {
	  snprintf(tmp, 50, "0x%08x{0x%08x,0x%08x}", (uint32_t)_memEA,
		  ntohl(proc->ReadMemory32(_memEA, false)), // false means "non-speculatively"
		  ntohl(proc->ReadMemory32(_memEA+4, false)));
      }
      char CRstr[11] = {0};
      snprintf(CRstr, 11, "0x%08x", CR);
      strswap(CRstr+2, '0', '_');
      char FPSCRstr[11] = {0};
      snprintf(FPSCRstr, 11, "0x%08x", FPSCR);
      strswap(FPSCRstr+2, '0', '_');
      char XERstr[11] = {0};
      snprintf(XERstr, 11, "0x%08x", XER);
      strswap(XERstr+2, '0', '_');
      printf("-0x140a000: Commit %6s %#-7x %33s %-47s CR:%8s XER:%8s FPSCR:%8s %s%lu\n", 
	     /*parent,*/ /* -%p */
	     MD_OP_NAME(simOp),  /* %6s */
	     CPC, /* %#x */
	     tmp, /* %s */
	     op_args(simOp, registers, regs).c_str(), /* %-47s */
	     CRstr, /* %8s */
	     XERstr, /* %8s */
	     FPSCRstr, /* %8s */
	     isSpec ? "(spec)" : "", /* %s */
	     (unsigned long) NOW /* %lu */
	    );
      fflush(stdout); 
    }  
  }
  uint32_t &inst = CurrentInstruction;

  switch ( simOp ) {
#define TRAP (void)0;
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) \
    case OP:								    \
      SYMCAT(OP,_IMPL);							    \
    break;
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2)                              \
   case OP:                                                                 \
     printf("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT);
#define PPC_INSTRUCTION_MODE 1
#include "powerpc.def"
   default:
     if (!isSpec) {
       printf("attempted to execute a bogus opcode %x (cycle %llu)\n",
	      simOp, (unsigned long long) NOW);
     }
#undef TRAP
  }

#if 0
  static int stopPoint = 0x14ae060;
  static unsigned int stopCyc = 000;
  if (!ppcThread::verbose && (ProgramCounter == stopPoint) && component::cycle() > stopCyc) { //0x2ca5bf4) {
    ppcThread::verbose = 1;
    printf("here %p @ %lld\n", (void*)registers[3], component::cycle());
  }
  //if (ppcThread::verbose) printRegs();
  /*if (_memEA == 0xd1100688 && _op == STORE) {
    printf("%p writes %x to %p\n", ProgramCounter,proc->ReadMemory32(_memEA,0), _memEA);
  }*/
#endif

  if(didCommit) {
    if (!isSpec) {
      totalCommitted++;

#if 0
      if (isReturn() && (_NPC != (ProgramCounter + 4))) {
	//simAddress isNPC = this->parent->popFromCallStack();
      }
      if (isBranchLink() && (_NPC != (ProgramCounter + 4))) {
	this->parent->pushToCallStack(regs.regs_L, registers[MD_REG_SP], _NPC);
      }
#endif
    }
    
    //update program counter
    _NPC = NextProgramCounter;
    _state = COMMITTED;

#if 0
    if (1 || ppcThread::verbose) {
      const int numAddrsSave = 150;
      static simAddress lastAddrs[numAddrsSave];
      static int lctr = 0;
      if (NextProgramCounter != ProgramCounter + 4) {
	lastAddrs[lctr % numAddrsSave] = ProgramCounter;
	lctr++;
      }
    }
#endif
  }
  return didCommit;
}

void ppcInstruction::printRegs() {
  simRegister *registers = parent->getRegisters();
  simRegister *reg = (registers);
  for (int i = 0; i < 32; ++i) {
    printf("%2d: %08lx ", i, (unsigned long) reg[i]);
    /*for (int j = 0; j < altivecWordSize; ++j) {
      printf(" %x ",(uint) VR(i,j));
      }*/
    if (i % 4 == 3) printf("\n");
  }
}
