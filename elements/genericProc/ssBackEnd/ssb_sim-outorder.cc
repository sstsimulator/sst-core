/*
 * Sim-outorder.c - sample out-of-order issue perf simulator implementation
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: ssb_sim-outorder.cc,v 1.14 2007-07-09 18:37:50 wcmclen Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.13  2007/05/07 20:58:15  afrodri
 * prefetch support
 *
 * Revision 1.12  2007/03/13 21:00:45  afrodri
 * changes to make 'sync' instructions only issue()/commit() once the pipeline is clear
 *
 * Revision 1.11  2007/03/13 17:24:59  afrodri
 * added max number of outstanding stores to main memory to avoid resource exhaustion in odd cases. Added support for 'sync' instructions
 *
 * Revision 1.10  2007/01/12 17:03:46  mjleven
 *
 * Makefile.am, configure.in
 *   add new files
 *
 * global.h
 *   add new typedef
 *
 * main.cpp
 *   print out the user and system time
 *
 * memory.cpp, memory.h
 *   add the ability to delay a write by N tics
 *
 *   add the ability to regsiter other base_memory objects into a base_memory
 *   object and redirect memory access based on address
 *
 *   add the ability to send a notification to a component if a memory location
 *   is accessed
 *
 *
 * pim.cpp
 *   change cycle() -> TimeStamp()
 *
 * pimSysCallTypes.h
 *   add enums for new system calss cpu sync and memory region
 *
 * processor.cpp
 *   add new topo
 *
 * processor.h
 *   add ability to remap memory ranges
 *
 * thread.cpp
 *   change printf -> INFO
 *
 * threadStack.cpp
 *   change printf -> INFO
 *
 * topologies.cpp
 *   add new topo
 *
 * enkiduAPI.cc
 *   add new component constructor which can be called with single config string
 *
 *   add ordering to parcels delivered at the same tick
 *
 *   add ability for components register with a all components of a given name
 *
 *   when sending a parcel it should be done based on the components TimeStamp()
 *   sendParcel now calculates delivery tick based on TimeStamp() * timestep
 *
 * enkiduParcel.cc
 *   zero tag() field
 *
 * enkidu_api.h
 *   change cycle() to Cycle(), this was do to for change to TimeStamp()
 *
 * enkidu_parcel.h
 *   add _parcelNumber this allows parcel ordering
 *
 * DRAMHub.cpp
 * LPCNetwork.cpp
 * LWP.cpp
 * LWP.h
 * LWPpreTic.cpp
 * NIF.cpp
 * SW2.cpp
 * cache.cpp
 * fcEntry.h
 * parcelHandler.cpp
 * realDRAM.cpp
 *   change cycle() -> TimeStamp()
 *
 * ppcComitPIMTrap.cpp
 * ppcCommitSystemTrap.cpp
 * ppcIssueSystemTrap.cpp
 * ppcSystemTrapHandler.h
 * pimSysCallDefs.h
 *
 *   add MEM_REGION_CREATE system call
 *   add MEM_REGION_GET system call
 *   add CPU_SYNC system call
 *   add MHZ_GET system call
 *
 * ppcFront.h
 *   add loadInfo
 *
 * ppcInstructionCommit.cpp
 * ppcInstructionIssue.cpp
 *   change cycle() -> TimeStamp()
 *
 * ppcLoader.cxx
 *   change printf -> INFO
 *   set the text and data field in loadInfo
 *
 * ppcThread.cpp
 *   change printf -> INFO
 *   set heap field in loadInfo
 *
 * heteroLWP.cc
 * heteroNIF.cc
 * heteroProc.cc
 * packetget.cc
 * prefetch.cc
 * smpProc.cc
 * ssb_DMA.cc
 * ssb_NIC.cc
 * ssb_NIC_interface.cc
 * ssb_PIM_NICChip.cc
 * ssb_PIM_NICProc.cc
 * ssb_genericNIC.cc
 * ssb_link.cc
 * ssb_mainProc.cc
 * ssb_router.cc
 * ssb_sbim-outorder-eventq.cc
 * ssb_sbim-outorder-memory.cc
 * ssb_sbim-outorder.cc
 * ssb_sbim-outorder.h
 * ssb_simpleNet.cc
 * listUnit.cc
 * traceThread.cpp
 *   change cycle() -> TimeStamp()
 *
 * ssb_sbim-outorder-enkidu.cc
 *   change cycle() -> TimeStamp()
 *   invalidate cache based on tag returned from memory model
 *
 *
 * ssb_sbim-outorder-init.cc
 *   change printf -> INFO
 *
 * Revision 1.9  2006/12/01 17:45:50  afrodri
 * added extra checks to prefetcher
 *
 * Revision 1.8  2006/09/27 16:23:57  afrodri
 * added change to object rs_link
 *
 * Revision 1.7  2006/08/24 19:05:27  afrodri
 * improved prefetcher so it works right...
 *
 * Revision 1.6  2006/08/22 22:06:44  afrodri
 * added prefetcher support for main proc
 *
 * Revision 1.5  2006/07/18 21:58:02  afrodri
 * WACI load stuff
 *
 * Revision 1.4  2006/07/05 16:42:46  afrodri
 * waciLoad
 *
 * Revision 1.3  2006/06/28 23:25:29  afrodri
 * more waci fixes.
 *
 * Revision 1.2  2006/06/27 23:33:29  afrodri
 * more support for WACI Load experiment
 *
 * Revision 1.1.1.1  2006/01/31 16:35:51  afrodri
 * first entry
 *
 * Revision 1.22  2005/12/21 16:22:28  arodrig6
 * added
 *
 * Revision 1.21  2005/08/16 21:12:55  arodrig6
 * changes for docs
 *
 * Revision 1.20  2005/04/28 21:56:58  arodrig6
 * further optimizations. removed a mod from ruu_issue and removed some calls to op() by caching the valeue for LSQ entries
 *
 * Revision 1.19  2005/04/27 23:38:36  arodrig6
 * optimizations, mainly to lsq_refresh, removed a mod
 *
 * Revision 1.18  2005/04/27 19:19:55  arodrig6
 * optimized out a call to op(), removed an invarient load
 *
 * Revision 1.17  2005/04/14 22:40:33  arodrig6
 * added instruct mix gathering
 *
 * Revision 1.16  2005/03/23 23:32:27  arodrig6
 * added support for yield
 *
 * Revision 1.15  2004/12/23 16:50:39  arodrig6
 * added interrupt mode to NIC, added more accuracy (compared to vanilla SS).
 *
 * Revision 1.14  2004/10/29 23:05:35  arodrig6
 * added variable instructionSize. Also, fixed a dependancy bug. Also, made a cade for invalid addresses which don't fetch
 *
 * Revision 1.13  2004/10/27 21:03:39  arodrig6
 * added SMP support
 *
 * Revision 1.12  2004/10/26 19:50:11  arodrig6
 * clock adjustment stuff.
 *
 * Revision 1.11  2004/09/09 15:22:44  arodrig6
 * removed some warnings. Fixed pipeClear() bug (not checking retireList).
 *
 * Revision 1.10  2004/08/28 00:48:04  arodrig6
 * added DMA support
 *
 * Revision 1.9  2004/08/24 15:43:30  arodrig6
 * added 'condem()' to thread. Integrated off-chip dram with conventional processor
 *
 * Revision 1.8  2004/08/23 14:49:04  arodrig6
 * docs
 *
 * Revision 1.7  2004/08/20 01:17:50  arodrig6
 * began integrating SW2/DRAM with ssBack
 *
 * Revision 1.6  2004/08/17 23:39:04  arodrig6
 * added main and NIC processors
 *
 * Revision 1.5  2004/08/16 22:06:22  arodrig6
 * fixed caches and branch prediction
 *
 * Revision 1.4  2004/08/13 23:12:19  arodrig6
 * added support for speculative executiond own the wrong path
 *
 * Revision 1.3  2004/08/12 22:56:04  arodrig6
 * more support for OOE and rollback. removed old memory interfaces, added better ones
 *
 * Revision 1.2  2004/08/10 22:55:35  arodrig6
 * works, except for the speculative stuff, caches, and multiple procs
 *
 * Revision 1.1  2004/08/05 23:51:45  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.10  2001/06/29 03:58:41  karu
 * made some minor edits to simfast and simoutorder to remove some warnings
 *
 * Revision 1.9  2001/06/08 00:47:04  karu
 * fixed sim-cheetah. eio does not work because i am still not sure how much state to commit for system calls.
 *
 * Revision 1.6  2001/04/11 05:15:26  karu
 * Fixed Makefile to work properly in Solaris also
 * Fixed sim-cache.c
 * Fixed a minor bug in sim-outorder. initializes stack_recover_idx to 0
 * in line 5192. the top stack other has a junk value when the branch
 * predictor returns from a function call. Dont know how this worked
 * in AIX. the bug manifests itself only on Solaris.
 *
 * Revision 1.5  2000/09/29 05:58:33  ramdas
 * Shifted the register dependence decoders by 1
 *
 * Revision 1.4  2000/05/31 03:08:35  karu
 * sim-outorder merged
 *
 * Revision 1.2  2000/03/23 02:41:37  karu
 * Merged differences with ramdass's machine.def and differences
 * made to make sim-outorder work.
 *
 * Revision 1.1.1.1  2000/02/25 21:02:52  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.5  1998/08/27 16:27:48  taustin
 * implemented host interface description in host.h
 * added target interface support
 * added support for register and memory contexts
 * instruction predecoding moved to loader module
 * Alpha target support added
 * added support for quadword's
 * added fault support
 * added option ("-max:inst") to limit number of instructions analyzed
 * explicit BTB sizing option added to branch predictors, use
 *       "-btb" option to configure BTB
 * added queue statistics for IFQ, RUU, and LSQ; all terms of Little's
 *       law are measured and reports; also, measures fraction of cycles
 *       in which queue is full
 * added fast forward option ("-fastfwd") that skips a specified number
 *       of instructions (using functional simulation) before starting timing
 *       simulation
 * sim-outorder speculative loads no longer allocate memory pages,
 *       this significantly reduces memory requirements for programs with
 *       lots of mispeculation (e.g., cc1)
 * branch predictor updates can now optionally occur in ID, WB,
 *       or CT
 * added target-dependent myprintf() support
 * fixed speculative quadword store bug (missing most significant word)
 * sim-outorder now computes correct result when non-speculative register
 *       operand is first defined speculative within the same inst
 * speculative fault handling simplified
 * dead variable "no_ea_dep" removed
 *
 * Revision 1.4  1997/04/16  22:10:23  taustin
 * added -commit:width support (from kskadron)
 * fixed "bad l2 D-cache parms" fatal string
 *
 * Revision 1.3  1997/03/11  17:17:06  taustin
 * updated copyright
 * `-pcstat' option support added
 * long/int tweaks made for ALPHA target support
 * better defaults defined for caches/TLBs
 * "mstate" command supported added for DLite!
 * supported added for non-GNU C compilers
 * buglet fixed in speculative trace generation
 * multi-level cache hierarchy now supported
 * two-level predictor supported added
 * I/D-TLB supported added
 * many comments added
 * options package supported added
 * stats package support added
 * resource configuration options extended
 * pipetrace support added
 * DLite! support added
 * writeback throttling now supported
 * decode and issue B/W now decoupled
 * new and improved (and more precise) memory scheduler added
 * cruft for TLB paper removed
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include "ssb_sim-outorder.h"
#include "ssb_ruu.h"
#include "ssb_rs_link.h"
#include "ssb_fetch_rec.h"
#include "ssb_host.h"
#include "ssb_misc.h"
#include "ssb_machine.h"
#include "ssb_memory.h"
#include "ssb_cache.h"
#include "ssb_bpred.h"
#include "ssb_resource.h"
#include "ssb_bitmap.h"
#include "ssb_options.h"
#include "ssb_stats.h"

#include "instruction.h"
#include "thread.h"

#define PTRACE 0
/* effective address computation is transfered via the reserved name
   DTMP. This is a 'fake' register dependancy name*/
#define DTMP 70

//#define DEBUG_ZERO 1

/*
 * This file implements a very detailed out-of-order issue superscalar
 * processor with a two-level memory system and speculative execution support.
 * This simulator is a performance simulator, tracking the latency of all
 * pipeline operations.
 */

/* simulated registers */
//static struct regs_t regs;

/* convert 64-bit inst text addresses to 32-bit inst equivalents */
#ifdef TARGET_PISA
#define IACOMPRESS(A)							\
  (compress_icache_addrs ? ((((A) - ld_text_base) >> 1) + ld_text_base) : (A))
#define ISCOMPRESS(SZ)							\
  (compress_icache_addrs ? ((SZ) >> 1) : (SZ))
#else /* !TARGET_PISA */
#define IACOMPRESS(A)		(A)
#define ISCOMPRESS(SZ)		(SZ)
#endif /* TARGET_PISA */

/*
 * functional unit resource configuration
 */
//#include "ssb_fu_config.h"

/* non-zero if all register operands are ready, update with MAX_IDEPS */
/* updated with two more dependencies for PPC */
#ifdef TARGET_PPC
#define OPERANDS_READY(RS)                                              \
((RS)->idep_ready[0] && (RS)->idep_ready[1] && (RS)->idep_ready[2] && (RS)->idep_ready[3] && (RS)->idep_ready[4])
#else
#define OPERANDS_READY(RS)                                              \
((RS)->idep_ready[0] && (RS)->idep_ready[1] && (RS)->idep_ready[2])
#endif

/*
 * input dependencies for stores in the LSQ:
 *   idep #0 - operand input (value that is store'd)
 *   idep #1 - effective address input (address of store operation)
 */
#define STORE_OP_INDEX                  0
#define STORE_ADDR_INDEX                1

#define STORE_OP_READY(RS)              ((RS)->idep_ready[STORE_OP_INDEX])
#define STORE_ADDR_READY(RS)            ((RS)->idep_ready[STORE_ADDR_INDEX])


/* service all functional unit release events, this function is called
   once per cycle, and it used to step the BUSY timers attached to each
   functional unit in the function unit resource pool, as long as a functional
   unit's BUSY count is > 0, it cannot be issued an operation */
inline void convProc::ruu_release_fu(void)
{
  int i;

  //printf("convProc::ruu_release_fu()\n");  /* wcm: debug */

  /* walk all resource units, decrement busy counts by one */
  int numRes = fu_pool->num_resources;
   res_desc *resources = fu_pool->resources;
  for (i=0; i<numRes; i++) {
    /* resource is released when BUSY hits zero */
    int &busy = resources[i].busy;
    if (busy > 0)
      busy--;
  }
}



//: instruction retirement pipeline stage
//
// this function commits the results of the oldest completed entries
// from the RUU and LSQ to the architected reg file, stores in the LSQ
// will commit their store data to the data cache at this point as
// well
//
// all values must be retired to the architected reg file in program
// order. We retire() instructions here (except for remote stores).
void convProc::ruu_commit(void)
{
  int i, lat, events, committed = 0;
  bool mainMemAc=0;

  //printf("convProc::ruu_commit()\n");  /* wcm: debug */

  /* register port accounting */
  regPortAvail += portLimitedCommit;
  if (regPortAvail > portLimitedCommit) regPortAvail = portLimitedCommit;

  /* all values must be retired to the architected reg file in program order */
  while (RUU_num > 0 && committed < ruu_commit_width &&
	 (portLimitedCommit < 1 || regPortAvail > 0)) {
     RUU_station *rs = &(RUU[RUU_head]);

    if (!rs->completed)	{
      /* at least RUU entry must be complete */
      break;
    }

    /* default commit events */
    events = 0;

    /* load/stores must retire load/store queue entry as well */
    if (RUU[RUU_head].ea_comp)	{
      /* load/store, retire head of LSQ as well */
      if (LSQ_num <= 0 || !LSQ[LSQ_head].in_LSQ)
	panic("RUU out of sync with LSQ");

      /* load/store operation must be complete */
      if (!LSQ[LSQ_head].completed) {
	/* load/store operation is not yet complete */
	break;
      }

      instruction *IR = LSQ[LSQ_head].IR;
      if (IR->op() == STORE) {
	int i;
	md_addr_t temp_addr;
	temp_addr = LSQ[LSQ_head].addr;

	/* stores must retire their store value to the cache at
	   commit, try to get a store port (functional unit
	   allocation) */
	bool needBreak = 0;
	for (i=0;i<=LSQ[LSQ_head].lsq_count;i++) {
	  res_template *fu = 0;
	  if (mainMemStores.size() < (uint)maxMMStores) 
	    fu = res_get(fu_pool, IR->fu());
	  if (fu) {
	    /* reserve the functional unit */
	    if (fu->master->busy)
	      panic("functional unit already in use");
	    
	    /* schedule functional unit release event */
	    fu->master->busy = fu->issuelat;
	    
	    /* go to the data cache */
	    if (cache_dl1) {
	      /* commit store value to D-cache */
	      mainMemAc = 0;
	      noteWrite(temp_addr&~3);
	      lat = cache_access(cache_dl1, Write, (temp_addr&~3),
				 NULL, 4, TimeStamp(), NULL, NULL, mainMemAc, NULL);
#if PTRACE == 1
	      if (lat > cache_dl1_lat)
		events |= PEV_CACHEMISS;
#endif
	    }
		    
	    /* all loads and stores must to access D-TLB */
	    if (dtlb) {
	      /* access the D-TLB */
	      bool dontCare;
	      lat = cache_access(dtlb, Read, (temp_addr & ~3),
				 NULL, 4, TimeStamp(), NULL, NULL, dontCare, NULL);
#if PTRACE == 1
	      if (lat > 1)
		events |= PEV_TLBMISS;
#endif
	    }

	    if (mainMemAc) {
	      mainMemStores.insert(LSQ[LSQ_head].IR);
	      mainMemAccess(LSQ[LSQ_head].IR);
	    }
	    // prefetcher
	    if (pref) pref->memRef(temp_addr, prefetcher::DATA, prefetcher::WRITE, (mainMemAc!=0));
	  } else {
	    /* no store ports left, cannot continue to commit insts */
	    needBreak = 1;
	    break;
	  }
	  temp_addr = temp_addr + 0x4;
	}
	if (needBreak) break;
      }

      if (IS_MULT_LSQ(LSQ[LSQ_head].IR->op()))
	lsq_mult--;
      
      /* invalidate load/store operation instance */
      LSQ[LSQ_head].tag++;
      
#if PTRACE == 1
      /* indicate to pipeline trace that this instruction retired */
      ptrace_newstage(LSQ[LSQ_head].ptrace_seq, PST_COMMIT, events);
      ptrace_endinst(LSQ[LSQ_head].ptrace_seq);
#endif

      /* commit head of LSQ as well */
      LSQ_head = (LSQ_head + 1) % LSQ_size;
      LSQ_num--;
    }

    if (pred
	&& bpred_spec_update == spec_CT
	&& (rs->IR->specificOp() & F_CTRL)) {
      bpred_update(pred,
		   /* branch address */rs->PC,
		   /* actual target address */rs->next_PC,
		   /* taken? */rs->next_PC != (rs->PC +
					       instructionSize),
		   /* pred taken? */rs->pred_PC != (rs->PC +
						    instructionSize),
		   /* correct pred? */rs->pred_PC == rs->next_PC,
		   /* opcode */(md_opcode)rs->IR->specificOp(),
		   /* dir predictor update pointer */&rs->dir_update);
    }

    /* invalidate RUU operation instance */
    RUU[RUU_head].tag++;
#if PTRACE == 1    
    /* indicate to pipeline trace that this instruction retired */
    ptrace_newstage(RUU[RUU_head].ptrace_seq, PST_COMMIT, events);
    ptrace_endinst(RUU[RUU_head].ptrace_seq);
#endif
    /* commit head entry of RUU */
    RUU_head = (RUU_head + 1) % RUU_size;
    RUU_num--;

    /* If we care, Count number of registers committed */
    if (portLimitedCommit > 0) {
      const int *outs = rs->IR->outDeps();
      int newReg = 0;
      for(int ii = 0; ii < MAX_ODEPS; ++ii) {
	if (*outs != -1) {
	  newReg++; //printf("%d ", *outs);
	} else {
	  break;
	} 
	outs++;
      }
      /*int *ins = rs->IR->inDeps();
      printf("<- ");
      for(int ii = 0; ii < MAX_IDEPS; ++ii) {
	if (ins[ii] != -1) {
	  printf("%d ", ins[ii]);
	  if (ins[ii] > 100) 
	    printf("!");
	} else {break;}
	}*/
      if (rs->IR->op() == ALU && newReg == 0) newReg = 1;
      regPortAvail -= newReg;
      //printf(" nr %d\n", newReg);
    }

    /* one more instruction committed to architected state */
    committed++;
    
    /* Debugging printfs */
#ifdef DEBUG_ZERO 
    printf("Checkpt 0: Committed instruction is %d.", committed);
    //    md_print_insn(rs->IR,0x0,stdout);  // wcm: broken.
    printf("\n");fflush(stdout);
#endif 
    
    /* clean up*/
    //printf("comm %p %d\n", rs->IR->PC(), rs->IR->op());
    if (mainMemAc || 
	!retireList.empty()) { // we keep main mem accesses around a
			       // bit longer so they are valid
			       // instructions
      retireList.push_back(rs->IR);
    } else {
      thr->retire(rs->IR);      
    }

    for (i=0; i<MAX_ODEPS; i++)	{
      if (rs->odep_list[i])
	panic ("retired instruction has odeps\n");
    }
  }
}


//: instruction result writeback pipeline stage
//
// writeback completed operation results from the functional units to
// RUU, at this point, the output dependency chains of completing
// instructions are also walked to determine if any dependent
// instruction now has all of its register operands, if so the
// (nearly) ready instruction is inserted into the ready instruction
// queue 
void convProc::ruu_writeback(void)
{
  int i;
  RUU_station *rs;

  //printf("convProc::ruu_writeback()\n");  /* wcm: debug */

  /* service all completed events */
  while ((rs = eventq_next_event())) {
    /* RS has completed execution and (possibly) produced a result */
    if (!OPERANDS_READY(rs) || rs->queued || !rs->issued || rs->completed)
      panic("inst completed and !ready, !issued, or completed");
    
    /* operation has completed */
    rs->completed = TRUE;
    
    /* does this operation reveal a mis-predicted branch? */
    if (rs->recover_inst) {
      if (rs->in_LSQ)
	panic("mis-predicted load or store?!?!?");

      /* recover processor state and reinit fetch to correct path */
      ruu_recover(rs - RUU);
      tracer_recover();
      bpred_recover(pred, rs->PC, rs->stack_recover_idx);

      /* stall fetch until I-fetch and I-decode recover */
      ruu_fetch_issue_delay = ruu_branch_penalty;
      
      /* continue writeback of the branch/control instruction */
    }
    
    /* if we speculatively update branch-predictor, do it here */
    if (pred
	&& bpred_spec_update == spec_WB
	&& !rs->in_LSQ
	&& ((rs->IR->specificOp()) & F_CTRL)) {
      bpred_update(pred,
		   /* branch address */rs->PC,
		   /* actual target address */rs->next_PC,
		   /* taken? */rs->next_PC != (rs->PC +
					       instructionSize),
		   /* pred taken? */rs->pred_PC != (rs->PC +
						    instructionSize),
		   /* correct pred? */rs->pred_PC == rs->next_PC,
		   /* opcode */(md_opcode)rs->IR->specificOp(),
		   /* dir predictor update pointer */&rs->dir_update);
    }

#if PTRACE == 1    
    /* entered writeback stage, indicate in pipe trace */
    ptrace_newstage(rs->ptrace_seq, PST_WRITEBACK,
		    rs->recover_inst ? PEV_MPDETECT : 0);
#endif    

    /* broadcast results to consuming operations, this is more efficiently
       accomplished by walking the output dependency chains of the
       completed instruction */
    //printf("WB %p %llu\n", rs->PC, TimeStamp());
    for (i=0; i<MAX_ODEPS; i++) {
      if (rs->onames[i] != NA) {
	 CV_link link;
	 RS_link *olink, *olink_next;

	if (rs->spec_mode) {
	  /* update the speculative create vector, future operations
	     get value from later creator or architected reg file */
	  link = spec_create_vector[rs->onames[i]];
	  if (/* !NULL */link.rs
	      && /* refs RS */(link.rs == rs && link.odep_num == i)) {
	    /* the result can now be read from a physical register,
	       indicate this as so */
	    spec_create_vector[rs->onames[i]] = CVLINK_NULL;
	    spec_create_vector_rt[rs->onames[i]] = TimeStamp();
	  }
	  /* else, creator invalidated or there is another creator */
	} else {
	  /* update the non-speculative create vector, future
	     operations get value from later creator or architected
	     reg file */
	  link = create_vector[rs->onames[i]];
	  if (/* !NULL */link.rs
	      && /* refs RS */(link.rs == rs && link.odep_num == i)) {
	    /* the result can now be read from a physical register,
	       indicate this as so */
	    create_vector[rs->onames[i]] = CVLINK_NULL;
	    create_vector_rt[rs->onames[i]] = TimeStamp();
	  }
	  /* else, creator invalidated or there is another creator */
	}

	/* walk output list, queue up ready operations */
	for (olink=rs->odep_list[i]; olink; olink=olink_next) {
	  if (RSLINK_VALID(olink)) {
	    if (olink->rs->idep_ready[olink->x.opnum])
	      panic("output dependence already satisfied");

	    /* input is now ready */
	    olink->rs->idep_ready[olink->x.opnum] = TRUE;
	    /* are all the register operands of target ready? */
	    if (OPERANDS_READY(olink->rs)) {
	      /* yes! enqueue instruction as ready, NOTE: stores
		 complete at dispatch, so no need to enqueue them */
	      if (!olink->rs->in_LSQ
		  || (olink->rs->IR->op() == STORE))
		//printf("Q2 %p %llu\n", olink->rs->PC, TimeStamp());
		readyq_enqueue(olink->rs);
	      /* else, ld op, issued when no mem conflict */
	    }
	  }

	  /* grab link to next element prior to free */
	  olink_next = olink->next;
	  
	  /* free dependence link element */
	  rs_free_list.RSLINK_FREE(olink);
	}
	/* blow away the consuming op list */
	rs->odep_list[i] = NULL;

      } /* if not NA output */
    } /* for all outputs */
  } /* for all writeback events */
}


#define MAX_STD_UNKNOWNS		256

//: memory access dependence checker/scheduler
//
// this function locates ready instructions whose memory dependencies
// have been satisfied, this is accomplished by walking the LSQ for
// loads, looking for blocking memory dependency condition (e.g.,
// earlier store with an unknown address)
void convProc::lsq_refresh(void)
{
  //printf("convProc::lsq_refresh()\n"); fflush(stdout);  /* wcm: DEBUG */
  int i, j, index, n_std_unknowns;
  md_addr_t std_unknowns[MAX_STD_UNKNOWNS];

  /* scan entire queue for ready loads: scan from oldest instruction
     (head) until we reach the tail or an unresolved store, after which no
     other instruction will become ready */
  for (i=0, index=LSQ_head, n_std_unknowns=0;
       i < LSQ_num;
       i++, /*index=(index + 1) % LSQ_size*/ index++) {
    if (index >= LSQ_size) index = 0; 
    /* terminate search for ready loads after first unresolved store,
       as no later load could be resolved in its presence */
    //const instType op = LSQ[index].IR->op();
    const instType op = LSQ[index].op;
    if (op == STORE) {
      if (!STORE_ADDR_READY(&LSQ[index])) {
	/* FIXME: a later STD + STD known could hide the STA unknown */
	/* sta unknown, blocks all later loads, stop search */
	break;
      } else if (!OPERANDS_READY(&LSQ[index])) {
	/* sta known, but std unknown, may block a later store, record
	   this address for later referral, we use an array here because
	   for most simulations the number of entries to search will be
	   very small */
	if (n_std_unknowns == MAX_STD_UNKNOWNS)
	  fatal("STD unknown array overflow, increase MAX_STD_UNKNOWNS");
	std_unknowns[n_std_unknowns++] = LSQ[index].addr;
      } else { /* STORE_ADDR_READY() && OPERANDS_READY() */
	/* a later STD known hides an earlier STD unknown */
	for (j=0; j<n_std_unknowns; j++) {
	  if (std_unknowns[j] == /* STA/STD known */LSQ[index].addr)
	    std_unknowns[j] = /* bogus addr */0;
	}
      }
#if 0
    } else if (/* (op == LOAD) && AFR: if its not a store, it must be a load*/
	       /* queued? */!LSQ[index].queued
	       && /* waiting? */!LSQ[index].issued
	       && /* completed? */!LSQ[index].completed
	       && /* regs ready? */OPERANDS_READY(&LSQ[index])) {
#else
    } else if (
	       !(/* queued? */(LSQ[index].queued) |
		/* waiting? */(LSQ[index].issued) |
		/* completed? */(LSQ[index].completed))
	       && /* regs ready? */OPERANDS_READY(&LSQ[index])) {
#endif
      /* no STA unknown conflict (because we got to this check), check for
	 a STD unknown conflict */
      for (j=0; j<n_std_unknowns; j++) {
	/* found a relevant STD unknown? */
	if (std_unknowns[j] == LSQ[index].addr)
	  break;
      }
      if (j == n_std_unknowns) {
	/* no STA or STD unknown conflicts, put load on ready queue */
	//printf("Q3 %p %llu\n", LSQ[index].PC, TimeStamp());
	readyq_enqueue(&LSQ[index]);
      }
    }
  }
}


//: issue instructions to functional units
//
// attempt to issue all operations in the ready queue; insts in the
// ready instruction queue have all register dependencies satisfied,
// this function must then 1) ensure the instructions memory
// dependencies have been satisfied (see lsq_refresh() for details on
// this process) and 2) a function unit is available in this cycle to
// commence execution of the operation; if all goes well, the function
// unit is allocated, a writeback event is scheduled, and the
// instruction begins execution
//
// Note, remote loads are sent off here and then entered into the
// queue when they return.
void convProc::ruu_issue(void)
{
  int i, n_issued;
  RS_link *node, *next_node;
  res_template *fu;

  //printf("convProc::ruu_issue()\n"); fflush(stdout); /* wcm: debug */
   
  /* copy and then blow away the ready list, NOTE: the ready list is
     always totally reclaimed each cycle, and instructions that are not
     issue are explicitly reinserted into the ready instruction queue,
     this management strategy ensures that the ready instruction queue
     is always properly sorted */
  node = ready_queue;
  ready_queue = NULL;

  /* visit all ready instructions (i.e., insts whose register input
     dependencies have been satisfied, stop issue when no more instructions
     are available or issue bandwidth is exhausted */
  for (n_issued=0; node && n_issued < ruu_issue_width; node = next_node) 
  {
    next_node = node->next;

    /* still valid? */
    if (RSLINK_VALID(node)) {
       RUU_station *rs = RSLINK_RS(node);
       instruction *IR = rs->IR;
       instType op = IR->op();

      int extraLatency = 0;
      // Check the instruction for extra latency.
      // NOTE:if this is too slow, we can limit it to only TRAP
      // instructions
      latencyMap::iterator exLIter = extraInstLat.find(IR);
      if (exLIter != extraInstLat.end()) {
	extraLatency = exLIter->second;
	//printf("%llu: found %s instruction %p with %d extra Latency\n", 
	//       TimeStamp(), instNames[op], rs->IR, extraLatency);
	extraInstLat.erase(IR);
      }

      /* issue operation, both reg and mem deps have been satisfied */
      if (!OPERANDS_READY(rs) || !rs->queued
	  || rs->issued || rs->completed)
	panic("issued inst !ready, issued, or completed");

      /* node is now un-queued */
      rs->queued = FALSE;

      if (rs->in_LSQ
	  && (op == STORE)) {
	/* stores complete in effectively zero time, result is
	   written into the load/store queue, the actual store into
	   the memory system occurs when the instruction is retired
	   (see ruu_commit()) */
	rs->issued = TRUE;
	      
	/* For stores, that create update, equivalent to 
	   the case when no functional units are required */	

	/* In PPC the stores should pass through the writeback stage */
#if defined(TARGET_PPC)&&1
	/* Just queue the event to go through
	 * the writeback stage at the next cycle */
#if 0
	eventq_queue_event(rs, TimeStamp() + clockRatio + extraLatency);
#else
	eventq_queue_event(rs, TimeStamp() + 1 + extraLatency);
#endif
#else 
	/* Other ISA's stores are completed here */
	rs->completed = TRUE; 
#endif      
	
	/* A store had earlier been defined as an op that does 
	   not do any register update.  PPC should bypass this check*/
#if defined(TARGET_PPC)&&0
	if (rs->onames[0] || rs->onames[1])
	  panic("store creates result"); 
#endif
	      
	if (rs->recover_inst)
	  panic("mis-predicted store");

#if PTRACE == 1	      
	/* entered execute stage, indicate in pipe trace */
	ptrace_newstage(rs->ptrace_seq, PST_WRITEBACK, 0);
#endif	      

	/* one more inst issued */
	n_issued++;
      } else {
	/* issue the instruction to a functional unit */
	if (IR->fu() != NA)	{

	  if (rs->ea_comp) {
	    fu = res_get(fu_pool, IntALU);
	  } else {
	    fu = res_get(fu_pool, IR->fu());
	  }

	  if (fu) {
	    /* got one! issue inst to functional unit */
	    rs->issued = TRUE;
	    
	    /* reserve the functional unit */
	    if (fu->master->busy)
	      panic("functional unit already in use");

	    /* schedule functional unit release event */
	    fu->master->busy = fu->issuelat;
	    if (waciLoadExtra > 0 && (IR->specificOp() & F_WACILOAD)) {
	      fu->master->busy += waciLoadExtra;
	    }

	    /* schedule a result writeback event */
	    if (rs->in_LSQ
		&& (op == LOAD)) {	     
#if PTRACE == 1
	      int events = 0;
#endif
	      bool mainMemAc = 0;
	      int j;
	      md_addr_t temp_addr;
	      int tot_lat,load_lat,tlb_lat;			  

	      temp_addr = rs->addr;
	      tot_lat = 0;

	      /* sanity check - lsq_count not be more than 32, except
	       * for unaligned LMW/STMW where it be 33 max */
			  
	      if (rs->lsq_count > (MD_NUM_IREGS + 1))
		panic("Request for more load/store (s)");
			 
	      /* Check LSQ */
	      for (j=0;j<=rs->lsq_count;j++) {
		load_lat = 0;tlb_lat = 0;
		i = (rs - LSQ);
		if (i != LSQ_head) {		  
		  for (;;) {
		    lsqCompares++;
		    /* go to next earlier LSQ entry */
#if 0
		    i = (i + (LSQ_size-1)) % LSQ_size;
#else
		    i = (i + (LSQ_size-1));
		    if (i >= LSQ_size) i -= LSQ_size;
#endif
		    
		    if ((LSQ[i].op == STORE)
			&& (LSQ[i].addr == temp_addr)) {
		      /* hit in the LSQ */
		      load_lat = 1;
		      break;
		    }

		    /* scan finished? */
		    if (i == LSQ_head)
		      break;
		  }
		}
		
		/* was the value store forwared from the LSQ? */
		if (load_lat != 1) {
		  /*		int valid_addr = MD_VALID_ADDR(rs->addr);*/
		  int valid_addr = MD_VALID_ADDR(temp_addr);
		  if (!spec_mode && !valid_addr)
		    sim_invalid_addrs++;
				
		  /* no! go to the data cache if addr is valid */
		  if (cache_dl1 && valid_addr) {
		    /* access the cache if non-faulting */
		    load_lat = cache_access(cache_dl1, Read,
					    (temp_addr & ~3), NULL, 4,
					    TimeStamp(), NULL, NULL, mainMemAc, 
					    NULL);
#if PTRACE == 1
		    if (load_lat > cache_dl1_lat)
		      events |= PEV_CACHEMISS;
#endif
		  } else {
		    /* no caches defined, just use op latency */
		    /*printf("spec_mode = %d, PC = 0x%08x\n",spec_mode, rs->PC);
		      printf("ERROR NO CACHE DEFINED\n");fflush(stdout); */
		    load_lat = fu->oplat;
		  }
		}
			    
		/* all loads and stores must access D-TLB */
		if (dtlb && MD_VALID_ADDR(temp_addr)) {
		  /* access the D-DLB, NOTE: this code will
		     initiate speculative TLB misses */
		  bool dontCare;
		  tlb_lat = cache_access(dtlb, Read, (temp_addr & ~3),
					 NULL, 4, TimeStamp(), NULL, NULL, 
					 dontCare, NULL);
#if PTRACE == 1
		  if (tlb_lat > 1)
		    events |= PEV_TLBMISS;
#endif		
		  /* D-cache/D-TLB accesses occur in parallel */
		  load_lat = MAX(tlb_lat, load_lat);
		}
		temp_addr += 0x4;
		tot_lat += load_lat;
	      }
			  
	      /* Another sanity check */
	      if (tot_lat <= 0) {
		panic("Latency of memory operation is <= 0");
	      }
			  
	      /* use computed cache access latency */
	      if (!mainMemAc) {
		eventq_queue_event(rs, TimeStamp() + tot_lat + extraLatency);
	      } else {
		mainMemLoads[rs->IR] = rs;
		mainMemAccess(rs->IR);
	      }
	      // prefetcher
	      if (pref) pref->memRef(temp_addr, prefetcher::DATA, prefetcher::READ, (mainMemAc!=0));
#if PTRACE == 1			  
	      /* entered execute stage, indicate in pipe trace */
	      ptrace_newstage(rs->ptrace_seq, PST_EXECUTE,
			      ((rs->ea_comp ? PEV_AGEN : 0)
			       | events));
#endif
	    } else { /* !load && !store */
	      /* use deterministic functional unit latency */
#if 0
	      eventq_queue_event(rs, TimeStamp() + (clockRatio * fu->oplat) + extraLatency);
#else
	      eventq_queue_event(rs, TimeStamp() + (1 * fu->oplat) + extraLatency);
#endif
#if PTRACE == 1      
	      /* entered execute stage, indicate in pipe trace */
	      ptrace_newstage(rs->ptrace_seq, PST_EXECUTE, 
			      rs->ea_comp ? PEV_AGEN : 0);
#endif
	    }
		      
	    /* one more inst issued */
	    n_issued++;
	  } else { /* no functional unit */
	    /* insufficient functional unit resources, put operation
	       back onto the ready list, we'll try to issue it
	       again next cycle */
	    readyq_enqueue(rs);
	  }
	} else { /* does not require a functional unit! */
	  /* FIXME: need better solution for these */
	  /* the instruction does not need a functional unit */
	  rs->issued = TRUE;
	  
	  /* schedule a result event */
#if 0
	  eventq_queue_event(rs, TimeStamp() + clockRatio + extraLatency);
#else
	  eventq_queue_event(rs, TimeStamp() + 1 + extraLatency);
#endif
#if PTRACE == 1
	  /* entered execute stage, indicate in pipe trace */
	  ptrace_newstage(rs->ptrace_seq, PST_EXECUTE,
			  rs->ea_comp ? PEV_AGEN : 0);
#endif
	  /* one more inst issued */
	  n_issued++;
	}
      } /* !store */
      
    }
    /* else, RUU entry was squashed */
    
    /* reclaim ready list entry, NOTE: this is done whether or not the
       instruction issued, since the instruction was once again reinserted
       into the ready queue if it did not issue, this ensures that the ready
       queue is always properly sorted */
    rs_free_list.RSLINK_FREE(node);
  }

  /* put any instruction not issued back into the ready queue, go through
     normal channels to ensure instruction stay ordered correctly */
  for (; node; node = next_node) {
    next_node = node->next;
    
    /* still valid? */
    if (RSLINK_VALID(node)) {
      RUU_station *rs = RSLINK_RS(node);
      
      /* node is now un-queued */
      rs->queued = FALSE;
      
      /* not issued, put operation back onto the ready list, we'll try to
	 issue it again next cycle */
      readyq_enqueue(rs);
    }
    /* else, RUU entry was squashed */
    
    /* reclaim ready list entry, NOTE: this is done whether or not the
       instruction issued, since the instruction was once again reinserted
       into the ready queue if it did not issue, this ensures that the ready
       queue is always properly sorted */
    rs_free_list.RSLINK_FREE(node);
  }
}

//: Decode instructions and allocate RUU and LSQ resources
//
// dispatch instructions from the IFETCH -> DISPATCH queue:
// instructions are first decoded, then they allocated RUU (and LSQ
// for load/stores) resources and input and output dependence chains
// are updated accordingly
//
// Also, we detect transitions to speculative mode here
//
// We issue() and commit() instructions here.
void convProc::ruu_dispatch(void)
{
  int n_dispatched=0;			/* total insts dispatched */
  instruction *inst;			/* actual instruction bits */
  instType op;  			/* decoded opcode enum */
  md_opcode specificOp=(md_opcode)0;	/* decoded opcode enum */
  int out[MAX_ODEPS], in[MAX_IDEPS];    /* output/input register names */
  md_addr_t target_PC;			/* actual next/target PC address */
  md_addr_t addr=0;			/* effective address, if load/store */
  RUU_station *rs;                      /* RUU station being allocated */
  RUU_station *lsq;                     /* LSQ station for ld/st's */
  bpred_update_t *dir_update_ptr=0;     /* branch predictor dir update ptr */
  int stack_recover_idx=0;		/* bpred retstack recovery index */
  unsigned int pseq=0;			/* pipetrace sequence number */
  int is_write;				/* store? */
  int made_check;			/* used to ensure DLite entry */
  int br_taken, br_pred_taken;		/* if br, taken?  predicted taken? */
  int fetch_redirected = FALSE;
  enum md_fault_type fault;
  md_addr_t regs_regs_PC=0, regs_regs_NPC=0;


  //printf("convProc::ruu_dispatch()\n"); fflush(stdout); /* wcm: debug */
  
  made_check = FALSE;
  n_dispatched = 0;
  while (/* instruction decode B/W left? */
	 n_dispatched < (ruu_decode_width * fetch_speed)
	 /* RUU and LSQ not full? */
	 && RUU_num < RUU_size && LSQ_num < LSQ_size
	 /* insts still available from fetch unit? */
	 && fetch_num != 0
	 /* on an acceptable trace path */
	 && (ruu_include_spec || !spec_mode)
	 /* Are we still executing a LMW/STMW instruction */
	 && !(lsq_mult) ) {

    for (int i = 0; i < MAX_ODEPS; ++i) {
      out[i] = NA;
      in[i] = NA;
    }
    
    /* if issuing in-order, block until last op issues if inorder issue */
    if (ruu_inorder_issue
	&& (last_op.rs && RSLINK_VALID(&last_op)
	    && !OPERANDS_READY(last_op.rs))){
      /* stall until last operation is ready to issue */
      break;
    }
    
    /* get the next instruction from the IFETCH -> DISPATCH queue */
    inst = fetch_data[fetch_head].IR;
    if (inst) {
      op = inst->op();
      specificOp = (md_opcode)inst->specificOp();
      
      // delay the issue()/commit() of any sync instructions while we
      // are still waiting for the pipeline to clear.
      if (isSyncing && (specificOp & F_SYNC)) {
	break;
      }

      regs_regs_PC = fetch_data[fetch_head].regs_PC;
    
      pred_PC = fetch_data[fetch_head].pred_PC;
      dir_update_ptr = &(fetch_data[fetch_head].dir_update);
      stack_recover_idx = fetch_data[fetch_head].stack_recover_idx;
      pseq = fetch_data[fetch_head].ptrace_seq;

      /* Debugging printfs */
#ifdef DEBUG_ZERO 
      //      printf("Checkpt: Decoded instruction is 0x%08x ",regs.regs_PC);
      //      md_print_insn(inst,0x0,stdout);
      //      printf("\n");fflush(stdout);
#endif
      /* compute default next PC */
      regs_regs_NPC = regs_regs_PC + instructionSize;
      
      
      /* drain RUU for TRAPs and system calls */
      if (op == TRAP) {
	if (RUU_num != 0) {
	  break;
	}
	/* else, syscall is only instruction in the machine, at this
	   point we should not be in (mis-)speculative mode */
	if (spec_mode)
	  panic("drained and speculative");
      }
      
      /* drain RUU for LMW and STMW */
      if (IS_MULT_LSQ(op)) {
	if (RUU_num != 0) {
	  break;
	} else {
	  lsq_mult++;
	}
      }
      
      if (!spec_mode) {
	/* one more non-speculative instruction executed */
	sim_num_insn++;
      }
      
      /* default effective address (none) and access */
      addr = 0; is_write = FALSE;
      
      /* set default fault - none */
      fault = md_fault_none;
      
      /* more decoding and execution */
      bool iRet = inst->issue(myProc, spec_mode);
      if (iRet == 0) 
	printf("Issue failed\n");

      committingInst = inst;
      bool cRet = inst->commit(myProc, spec_mode);
      if (cRet == 0) {
	if (inst->exception() == FEB_EXCEPTION ||  
	    inst->exception() == YIELD_EXCEPTION) {
	  ruu_dispatch_delay = getFEBDelay();
	  break;
	}
	printf("sbb %p: Commit failed for %x\n", this, (uint)inst->PC());
      }
      committingInst = 0;

      if (op == LOAD || op == STORE) {
	addr = inst->memEA();
      }

      /* compute output/input dependencies to out1-2 and in1-3 */
      {
	const int *outs = inst->outDeps();
	const int *ins = inst->inDeps();  /* wcm: suspect??? */
	int i;
	for(i = 0; i < MAX_ODEPS; ++i) {
	  if (*outs != -1) {
	    out[i] = *outs;
	  } else {
	    out[i] = 0;
	    break;
	  } 
	  outs++;
	}
	for (;i<MAX_ODEPS;++i) {out[i] = 0;}
	for(i = 0; i < MAX_IDEPS; ++i) {
	  if (*ins != -1) {
	    in[i] = *ins;
	  } else {
	    in[i] = 0;
	    break;
	  } 
	  ins++;
	}
	for (;i<MAX_IDEPS;++i) {in[i] = 0;}
      }
      /* operation sets next PC */
      regs_regs_NPC = inst->NPC();
      target_PC = inst->TPC();
      
      if (fault != md_fault_none)
	fatal("non-speculative fault (%d) detected @ 0x%08p",
	      fault, regs_regs_PC);
      
      /* update memory access stats */
      if (specificOp & F_MEM) {
	sim_total_refs++;
	if (!spec_mode)
	  sim_num_refs++;
	
	if (op == STORE)
	  is_write = TRUE;
	else {
	  sim_total_loads++;
	  if (!spec_mode)
	    sim_num_loads++;
	}
      }

      br_taken = (regs_regs_NPC != (regs_regs_PC + instructionSize));
      br_pred_taken = (pred_PC != (regs_regs_PC + instructionSize));

      //printf("%p %p %p\n", regs_regs_PC, pred_PC, regs_regs_NPC);
      if (((pred_PC != regs_regs_NPC && pred_perfect)
	   || ((specificOp & (F_CTRL|F_DIRJMP)) == (F_CTRL|F_DIRJMP)
	       && target_PC != pred_PC && br_pred_taken))  ) {
	/* Either 1) we're simulating perfect prediction and are in a
	   mis-predict state and need to patch up, or 2) We're not simulating
	   perfect prediction, we've predicted the branch taken, but our
	   predicted target doesn't match the computed target (i.e.,
	   mis-fetch).  Just update the PC values and do a fetch squash.
	   This is just like calling fetch_squash() except we pre-anticipate
	   the updates to the fetch values at the end of this function.  If
	   case #2, also charge a mispredict penalty for redirecting fetch */
	fetch_pred_PC = fetch_regs_PC = regs_regs_NPC;
	/* was: if (pred_perfect) */
	if (pred_perfect) {
	  pred_PC = regs_regs_NPC;
	}
	
	// squash others in fetch buffer
	for(fetch_head = (fetch_head+1) & (ruu_ifq_size - 1), fetch_num--;
	    fetch_num > 0;
	    fetch_head = (fetch_head+1) & (ruu_ifq_size - 1), fetch_num--) {
	  thr->squash(fetch_data[fetch_head].IR);
	}

	fetch_head = (ruu_ifq_size-1);
	fetch_num = 1;
	fetch_tail = 0;
	
	if (!pred_perfect) {
	  ruu_fetch_issue_delay = ruu_branch_penalty;
	}

	fetch_redirected = TRUE;
      }
    } else {
      op = BUBBLE;
    }

      /* is this a NOP */
    if (op != BUBBLE) {
      /* for load/stores:
	 idep #0     - store operand (value that is store'ed)
	 idep #1, #2 - eff addr computation inputs (addr of access)
	 
	 resulting RUU/LSQ operation pair:
	 RUU (effective address computation operation):
	 idep #0, #1 - eff addr computation inputs (addr of access)
	 LSQ (memory access operation):
	 idep #0     - operand input (value that is store'd)
	 idep #1     - eff addr computation result (from RUU op)
	 
	 effective address computation is transfered via the reserved
	 name DTMP
      */
      
      /* fill in RUU reservation station */
      rs = &RUU[RUU_tail];

      rs->IR = inst;
      rs->PC = regs_regs_PC;
      rs->next_PC = regs_regs_NPC; rs->pred_PC = pred_PC;
      rs->in_LSQ = FALSE;
      rs->ea_comp = FALSE;
      rs->recover_inst = FALSE;
      rs->dir_update = *dir_update_ptr;
      rs->stack_recover_idx = stack_recover_idx;
      rs->spec_mode = spec_mode;
      rs->addr = 0;
      /* rs->tag is already set */
      rs->seq = ++inst_seq;
      rs->queued = rs->issued = rs->completed = FALSE;
      rs->ptrace_seq = pseq;
      rs->lsq_count = 0;
	  
      /* split ld/st's into two operations: eff addr comp + mem access */
      if (op == LOAD || op == STORE) {
	/* convert RUU operation from ld/st to an add (eff addr comp) */
	// AFR cmntd out: rs->op = MD_AGEN_OP;
	rs->ea_comp = TRUE;

	/* fill in LSQ reservation station */
	lsq = &LSQ[LSQ_tail];
	
	lsq->IR = inst;
	lsq->op = op;
	lsq->PC = regs_regs_PC;
	lsq->next_PC = regs_regs_NPC; lsq->pred_PC = pred_PC;
	lsq->in_LSQ = TRUE;
	lsq->ea_comp = FALSE;
	lsq->recover_inst = FALSE;
	lsq->dir_update.pdir1 = lsq->dir_update.pdir2 = NULL;
	lsq->dir_update.pmeta = NULL;
	lsq->stack_recover_idx = 0;
	lsq->spec_mode = spec_mode;
	lsq->addr = addr;
	/* lsq->tag is already set */
	lsq->seq = ++inst_seq;
	lsq->queued = lsq->issued = lsq->completed = FALSE;
	lsq->ptrace_seq = ptrace_seq++;
	lsq->lsq_count = 0;
#if PTRACE == 1	
	/* pipetrace this uop */
	ptrace_newuop(lsq->ptrace_seq, "internal ld/st", lsq->PC, 0);
	ptrace_newstage(lsq->ptrace_seq, PST_DISPATCH, 0);
#endif

	/* link eff addr computation onto operand's output chains */
	ruu_link_idep(rs, /* idep_ready[] index */0, NA);
	ruu_link_idep(rs, /* idep_ready[] index */1, in[1]);
	ruu_link_idep(rs, /* idep_ready[] index */2, in[2]);
	/* Extra input dependencies for PPC */
#if defined(TARGET_PPC)
	ruu_link_idep(rs, /* idep_ready[] index */3, NA);	 
	ruu_link_idep(rs, /* idep_ready[] index */4, NA);
#endif

	/* install output after inputs to prevent self reference */
	ruu_install_odep(rs, /* odep_list[] index */0, DTMP);
	for (int ioi = 1; ioi < MAX_ODEPS; ++ioi) {
	  ruu_install_odep(rs, /* odep_list[] index */ioi, NA);
	}

	/* link memory access onto output chain of eff addr operation */
	ruu_link_idep(lsq,
		      /* idep_ready[] index */STORE_OP_INDEX/* 0 */,
		      in[0]);
	ruu_link_idep(lsq,
		      /* idep_ready[] index */STORE_ADDR_INDEX/* 1 */,
		      DTMP);
	ruu_link_idep(lsq, /* idep_ready[] index */2, NA);
	/* Extra input dependencies for PPC */
#if defined(TARGET_PPC)
	ruu_link_idep(lsq, /* idep_ready[] index */3, NA);	 
	ruu_link_idep(lsq, /* idep_ready[] index */4, NA);
#endif
	      
	/* install output after inputs to prevent self reference */
	for (int i = 0; i < MAX_ODEPS; ++i) {
	  ruu_install_odep(lsq, /* odep_list[] index */i, out[i]);
	}

	/* install operation in the RUU and LSQ */
	n_dispatched++;
	RUU_tail = (RUU_tail + 1) % RUU_size;
	RUU_num++;
	LSQ_tail = (LSQ_tail + 1) % LSQ_size;
	LSQ_num++;

	if (OPERANDS_READY(rs))	{
	  /* eff addr computation ready, queue it on ready list */
	  //printf("Qe %p %llu\n", rs->PC, TimeStamp());
	  readyq_enqueue(rs);
	}
	/* issue may continue when the load/store is issued */
	RSLINK_INIT(last_op, lsq);

	/* issue stores only, loads are issued by lsq_refresh() */
	if ((op == STORE) && OPERANDS_READY(lsq)) {
	  /* put operation on ready list, ruu_issue() issue it later */
	  //printf("Qs %p %llu\n", lsq->PC, TimeStamp());
	  readyq_enqueue(lsq);
	}
      } else { /* !(MD_OP_FLAGS(op) & F_MEM) */
	for (int i = 0; i < MAX_IDEPS; ++i) {
	  /* link onto producing operation */
	  ruu_link_idep(rs, /* idep_ready[] index */i, in[i]);
	} 
	for (int i = 0; i < MAX_IDEPS; ++i) {
	  /* install output after inputs to prevent self reference */
	  ruu_install_odep(rs, /* odep_list[] index */i, out[i]);
	}	      
	      
	/* install operation in the RUU */
	n_dispatched++;
	RUU_tail = (RUU_tail + 1) % RUU_size;
	RUU_num++;
	      
	/* issue op if all its reg operands are ready (no mem input) */
	if (OPERANDS_READY(rs))	{
	  /* put operation on ready list, ruu_issue() issue it later */
	  //printf("Q  %p %llu\n", rs->PC, TimeStamp());
	  readyq_enqueue(rs);
	  /* issue may continue */
	  last_op = rs_free_list.RSLINK_NULL;
	} else {
	  /* could not issue this inst, stall issue until we can */
	  RSLINK_INIT(last_op, rs);
	}
      }
    } else {
      /* this is a NOP, no need to update RUU/LSQ state */
      rs = NULL;
    }
      
    /* one more instruction executed, speculative or otherwise */
    sim_total_insn++;
    if (op == BRANCH || op == JMP)
      sim_total_branches++;
      
    if (inst && !spec_mode) {	  
      /* if this is a branching instruction update BTB, i.e., only
	 non-speculative state is committed into the BTB */
      if (specificOp & F_CTRL) {
	sim_num_branches++;
	if (pred && bpred_spec_update == spec_ID) {
	  bpred_update(pred,
		       /* branch address */regs_regs_PC,
		       /* actual target address */regs_regs_NPC,
		       /* taken? */regs_regs_NPC != (regs_regs_PC +
						     instructionSize),
		       /* pred taken? */pred_PC != (regs_regs_PC +
						    instructionSize),
		       /* correct pred? */pred_PC == regs_regs_NPC,
		       /* opcode */specificOp,
		       /* predictor update ptr */&rs->dir_update);
	}
      }
	  
      /* is the trace generator trasitioning into mis-speculation mode? */
      if (!simpleFetch && pred_PC != regs_regs_NPC && !fetch_redirected) {
	/* entering mis-speculation mode, indicate this and save PC */
	spec_mode = TRUE;
	/* Debugging printfs */
#if defined(DEBUG_ZERO) 
	printf("Transition to speculative mode at 0x%08x, 0x%08x\n",pred_PC, regs_regs_NPC);
#endif 
	rs->recover_inst = TRUE;
	recover_PC = regs_regs_NPC;
	thr->prepareSpec();
      }
    }
      
#if PTRACE == 1
    /* entered decode/allocate stage, indicate in pipe trace */
    ptrace_newstage(pseq, PST_DISPATCH,
		    (pred_PC != regs_regs_NPC) ? PEV_MPOCCURED : 0);
    if (op == BUBBLE)	{
      /* end of the line */
      ptrace_endinst(pseq);
    }
#endif      

    /* update any stats tracked by PC */
    for (int i=0; i<pcstat_nelt; i++) {
      counter_t newval;
      int delta;
      
      /* check if any tracked stats changed */
      newval = STATVAL(pcstat_stats[i]);
      delta = newval - pcstat_lastvals[i];
      if (delta != 0) {
	stat_add_samples(pcstat_sdists[i], regs_regs_PC, delta);
	pcstat_lastvals[i] = newval;
      }
    }

#if GET_IMIX == 1
    // iMix Trace
    if (!spec_mode) {
      iMix[op]++;
    }
#endif
      
    /* consume instruction from IFETCH -> DISPATCH queue */
    fetch_head = (fetch_head+1) & (ruu_ifq_size - 1);
    fetch_num--;
      
    /* check for DLite debugger entry condition */
#if 0
    made_check = TRUE;
    if (dlite_check_break(pred_PC,
			  is_write ? ACCESS_WRITE : ACCESS_READ,
			  addr, sim_num_insn, sim_cycle))
      dlite_main(regs_regs_PC, pred_PC, sim_cycle, &regs, mem);
#endif
  }
  
  /* need to enter DLite at least once per cycle */
#if 0
  if (!made_check) {
    if (dlite_check_break(/* no next PC */0,
			  is_write ? ACCESS_WRITE : ACCESS_READ,
			  addr, sim_num_insn, sim_cycle))
      dlite_main(regs_regs_PC, /* no next PC */0, sim_cycle, &regs, mem);
  }
#endif
}


//: instruction fetch pipeline stage(s)
//
// fetch up as many instruction as one branch prediction and one cache
// line acess will support without overflowing the IFETCH -> DISPATCH
// QUEUE
//
// We fetch() instructions here.
void convProc::ruu_fetch(void)
{
  int i, lat, tlb_lat, done = FALSE;
  instruction *inst;
  int stack_recover_idx;
  int branch_cnt;
  int bogus_pc; /* A flag to check if PC is a bogus PC */  

  //printf("convProc::ruu_fecth()\n"); fflush(stdout); /*wcm: debug*/

  if (clearPipe == 1 || thr == NULL || thr->isDead()) {
    // check if we are syncing 
    if (isSyncing && pipeDispatchClear()) {
      // done syncing
      isSyncing = 0;
      clearPipe = 0;
    } else {
      return;
    }
  }

  for (i=0, branch_cnt=0;
       /* fetch up to as many instruction as the DISPATCH stage can decode */
       i < (ruu_decode_width * fetch_speed)
       /* fetch until IFETCH -> DISPATCH queue fills */
       && fetch_num < ruu_ifq_size
       /* and no IFETCH blocking condition encountered */
       && !done;
       i++)
    {
      /* fetch an instruction at the next predicted fetch address */
      fetch_regs_PC = fetch_pred_PC;
      bogus_pc = 0;
      
      /* is this a bogus text address? (can happen on mis-spec path) */
      if (simpleFetch || (thr->isPCValid(fetch_regs_PC) && !thr->isDead())) {
	/* read instruction from memory */
	if (simpleFetch) {
	  inst = thr->getNextInstruction();
	} else {
	  inst = thr->getNextInstruction(fetch_regs_PC);
	}

	lat = cache_il1_lat;
	bool mainMemAc = 0;
	if (!simpleFetch && cache_il1) { /* access the I-cache */
	  lat = cache_access(cache_il1, Read, IACOMPRESS(ntohl(fetch_regs_PC)),
			     NULL, ISCOMPRESS(instructionSize), TimeStamp(),
			     NULL, NULL, mainMemAc, NULL);
	  // prefetcher
	  if (pref) pref->memRef(fetch_regs_PC, prefetcher::INST, prefetcher::READ, (mainMemAc!=0));
	  if (lat > cache_il1_lat)
	    last_inst_missed = TRUE;
	}
	
	if (!simpleFetch && itlb) {
	  /* access the I-TLB, NOTE: this code will initiate
	     speculative TLB misses */
	  bool dontCare;
	  tlb_lat = cache_access(itlb, Read, IACOMPRESS(ntohl(fetch_regs_PC)),
				 NULL, ISCOMPRESS(instructionSize), 
				 TimeStamp(), NULL, NULL, dontCare, NULL);
	  if (tlb_lat > 1)
	    last_inst_tmissed = TRUE;
	  
	  /* I-cache/I-TLB accesses occur in parallel */
	  lat = MAX(tlb_lat, lat);
	}
	
	/* missed to main memory */
	if (mainMemAc) {
	  // don't squash now, wait till later. 
	  iFetchBlocker = inst;	  
	  mainMemAccess(inst);
	  break;
	}
	
	/* I-cache/I-TLB miss? assumes I-cache hit >= I-TLB hit */
	if (lat != cache_il1_lat) {
	  /* I-cache miss, block fetch until it is resolved */
	  ruu_fetch_issue_delay += lat - 1;
	  /* return the missed instruction */
	  thr->squash(inst);
	  break;
	}
	/* else, I-cache/I-TLB hit */
      } else {
	/* fetch PC is bogus, send a NOP down the pipeline */
	bogus_pc = 1;
	inst = 0;
      }
      
      /* have a valid inst, here */
      if (inst) {
	bool fRet = inst->fetch(myProc);
	if (fRet != true) {printf("fetch failed\n");}
      }
      
      /* possibly use the BTB target */
      if (inst && pred)	{
	instType op = inst->op();
	/* get the next predicted fetch address; only use branch predictor
	   result for branches (assumes pre-decode bits); NOTE: returned
	   value may be 1 if bpred can only predict a direction */
	stack_recover_idx = 0;
	if (op == JMP || op == BRANCH) {
	  fetch_pred_PC =
	    bpred_lookup(pred,
			 /* branch address */fetch_regs_PC,
			 /* target address *//* FIXME: not computed */0,
			 /* opcode */(md_opcode)inst->specificOp(),
			 /* call? */MD_IS_CALL(),
			 /* return? */inst->isReturn(),
			 /* updt */&(fetch_data[fetch_tail].dir_update),
			 /* RSB index */&stack_recover_idx);
	} else {
	  fetch_pred_PC = 0;
	}

	/* valid address returned from branch predictor? */
	if (!fetch_pred_PC) {
	  /* no predicted taken target, attempt not taken target */
	  fetch_pred_PC = ntohl(ntohl(fetch_regs_PC) + instructionSize);
	} else {
	  /* go with target, NOTE: discontinuous fetch, so terminate */
	  branch_cnt++;
	  if (branch_cnt >= fetch_speed)
	    done = TRUE;
	}
      } else {
	if (inst == 0 && bogus_pc == 0) {
	  /* for some reason we couldn't getNextInst from the thread,
	     but the PC was valid. So, we stay at this address */
	  printf("validPC, but no inst\n");
	  break;
	} else if (inst == 0) { 
	  // invalid address
	  break;
	} else {
	  /* no predictor, just default to predict not taken, and
	     continue fetching instructions linearly */
	  fetch_pred_PC = ntohl(ntohl(fetch_regs_PC) + instructionSize);
	}
      }
      
      /* commit this instruction to the IFETCH -> DISPATCH queue */
      fetch_data[fetch_tail].IR = inst;
      fetch_data[fetch_tail].regs_PC = fetch_regs_PC;
      fetch_data[fetch_tail].pred_PC = fetch_pred_PC;
      fetch_data[fetch_tail].stack_recover_idx = stack_recover_idx;
      fetch_data[fetch_tail].ptrace_seq = ptrace_seq++;

      /* Debugging prints */
#if 0
      printf("Checkpt 2: Fetched instruction is 0x%08x ",fetch_regs_PC);
      md_print_insn(fetch_data[fetch_tail].IR,0x0,stdout);
      printf("\n");fflush(stdout);
#endif
      
#if PTRACE == 1
      /* for pipe trace */
      ptrace_newinst(fetch_data[fetch_tail].ptrace_seq,
		     inst, fetch_data[fetch_tail].regs_PC,
		     0);
      ptrace_newstage(fetch_data[fetch_tail].ptrace_seq,
		      PST_IFETCH,
		      ((last_inst_missed ? PEV_CACHEMISS : 0)
		       | (last_inst_tmissed ? PEV_TLBMISS : 0)));
#endif
      last_inst_missed = FALSE;
      last_inst_tmissed = FALSE;
      
      /* adjust instruction fetch queue */
      fetch_tail = (fetch_tail + 1) & (ruu_ifq_size - 1);
      fetch_num++;

      // check for and handle sync/eieio instructions
      if (inst->specificOp() & F_SYNC) {
	clearPipe = 1;
	isSyncing = 1;
	break; // don't fetch any more
      }
    }
}

//: Simulate a cycle
// was sim_main 
void convProc::sim_loop(void) {
  /* main simulator loop, NOTE: the pipe stages are traverse in reverse order
     to eliminate this/next state synchronization and relaxation problems */
  tickCount++;

  if (pref) pref->preTic();

#if 0
  // these sanity checks incurr a signifigant performance hit
  /* RUU/LSQ sanity checks */
  if (RUU_num < LSQ_num)
    panic("RUU_num < LSQ_num");
  if (((RUU_head + RUU_num) % RUU_size) != RUU_tail)
    panic("RUU_head/RUU_tail wedged");
  if (((LSQ_head + LSQ_num) % LSQ_size) != LSQ_tail)
    panic("LSQ_head/LSQ_tail wedged");
#endif  

#if PTRACE == 1
  /* check if pipetracing is still active */
  ptrace_check_active(regs.regs_PC, sim_num_insn, TimeStamp());
  
  /* indicate new cycle in pipetrace */
  ptrace_newCyle(sim_cycle);
#endif
  
  /* commit entries from RUU/LSQ to architected register file */
  ruu_commit();
  
  /* service function unit release events */
  ruu_release_fu();
  
  /* ==> may have ready queue entries carried over from previous cycles */
    
  /* service result completions, also readies dependent operations */
  /* ==> inserts operations into ready queue --> register deps resolved */
  ruu_writeback();
  
  /* try to locate memory operations that are ready to execute */
  /* ==> inserts operations into ready queue --> mem deps resolved */
  lsq_refresh();
  
  /* issue operations ready to execute from a previous cycle */
  /* <== drains ready queue <-- ready operations commence execution */
  ruu_issue();	
  
  /* decode and dispatch new operations */
  /* ==> insert ops w/ no deps or all regs ready --> reg deps resolved */
  if (!ruu_dispatch_delay) {
    ruu_dispatch();
  } else {
    ruu_dispatch_delay--;
  }
  
  /* call instruction fetch unit if it is not blocked */
  if (!ruu_fetch_issue_delay) {
    if (iFetchBlocker == 0) {
      if (!ruu_dispatch_delay) {
	ruu_fetch();
      } 
    }
  } else {
    ruu_fetch_issue_delay--;
  }
  
  /* update buffer occupancy stats */
  IFQ_count += fetch_num;
  IFQ_fcount += ((fetch_num == ruu_ifq_size) ? 1 : 0);
  RUU_count += RUU_num;
  RUU_fcount += ((RUU_num == RUU_size) ? 1 : 0);
  LSQ_count += LSQ_num;
#if WANT_LSQ_HIST == 1
  LSQ_hist[LSQ_num]++;
#endif
  LSQ_fcount += ((LSQ_num == LSQ_size) ? 1 : 0);
    
#ifdef TARGET_PPC
  /* A few sanity checks specific to PPC*/
  if (lsq_mult < 0)
    panic("Internal error: lsq_mult < 0");
#endif
  
  /* finish early? */
  if (max_insts && sim_total_insn >= max_insts) {
    return;
  }
}
