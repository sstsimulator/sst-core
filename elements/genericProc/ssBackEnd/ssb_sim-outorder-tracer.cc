// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "ssb_sim-outorder.h"
#include "ssb_fetch_rec.h"
#include "ssb_ruu.h"
#include "ssb_rs_link.h"
#include "FE/processor.h"

#define TRACER_DBG 0

//: Recover to precise state
//
// recover instruction trace generator state to precise state state
// immediately before the first mis-predicted branch; this is
// accomplished by resetting all register value copied-on-write
// bitmasks are reset, and the speculative memory hash table is
// cleared 
void convProc::tracer_recover(void)
{
  /* better be in mis-speculative trace generation mode */
  if (!spec_mode)
    panic("cannot recover unless in speculative mode");

  /* reset to non-speculative trace generation mode */
  spec_mode = FALSE;

  /* reset memory state back to non-speculative state */
  myProc->squashSpec();

  /* if pipetracing, indicate squash of instructions in the inst fetch queue */
#if PTRACE == 1
  if (ptrace_active)
    {
      while (fetch_num != 0)
	{
	  /* squash the next instruction from the IFETCH -> DISPATCH queue */
	  ptrace_endinst(fetch_data[fetch_head].ptrace_seq);

	  /* consume instruction from IFETCH -> DISPATCH queue */
	  fetch_head = (fetch_head+1) & (ruu_ifq_size - 1);
	  fetch_num--;
	}
    }
#endif

  /* reset IFETCH state */
#if TRACER_DBG == 1
  printf(" tracer_recover\n");
#endif
  for(; fetch_num > 0;
      fetch_head = (fetch_head+1) & (ruu_ifq_size - 1), fetch_num--) {
    instruction *sInst = fetch_data[fetch_head].IR;
#if TRACER_DBG == 1
    printf("  squashing %p\n", (char*)sInst->PC());
#endif
    thr->squash(sInst);
  }

  fetch_num = 0;
  fetch_tail = fetch_head = 0;
  fetch_pred_PC = fetch_regs_PC = recover_PC;
}


//: initialize the speculative instruction state
void convProc::tracer_init(void)
{
  /* initially in non-speculative mode */
  spec_mode = FALSE;
}

//: squash mispredicted microarchitecture state
//
// recover processor microarchitecture state back to point of the
// mis-predicted branch at RUU[BRANCH_INDEX]. Also calls
// tracer_recover to finish and reset speculative state.
void
convProc::ruu_recover(int branch_index)	/* index of mis-pred branch */
{
#if TRACER_DBG == 1
  printf("ruu_recover\n");
#endif
  int i, RUU_index = RUU_tail, LSQ_index = LSQ_tail;
  int RUU_prev_tail = RUU_tail, LSQ_prev_tail = LSQ_tail;

  /* recover from the tail of the RUU towards the head until the branch index
     is reached, this direction ensures that the LSQ can be synchronized with
     the RUU */

  /* go to first element to squash */
  RUU_index = (RUU_index + (RUU_size-1)) % RUU_size;
  LSQ_index = (LSQ_index + (LSQ_size-1)) % LSQ_size;

  /* traverse to older insts until the mispredicted branch is encountered */
  while (RUU_index != branch_index)
    {
      bool isRemote = 0;
      /* the RUU should not drain since the mispredicted branch will remain */
      if (!RUU_num)
	panic("empty RUU");

      /* should meet up with the tail first */
      if (RUU_index == RUU_head)
	panic("RUU head and tail broken");

      /* is this operation an effective addr calc for a load or store? */
      if (RUU[RUU_index].ea_comp) {
	if (mainMemStores.find(RUU[RUU_index].IR) != mainMemStores.end()) {
	  printf("need to squash remote store\n");
	  exit(-1);
	}
	if (mainMemLoads.find(RUU[RUU_index].IR) != mainMemLoads.end()) {
	  mainMemLoads.erase(RUU[RUU_index].IR);
	  condemnedRemotes.insert(RUU[RUU_index].IR);
	  isRemote = 1;
	}

	/* should be at least one load or store in the LSQ */
	if (!LSQ_num)
	  panic("RUU and LSQ out of sync");
	
	/* recover any resources consumed by the load or store operation */
	for (i=0; i<MAX_ODEPS; i++)
	  {
	    rs_free_list.RSLINK_FREE_LIST(LSQ[LSQ_index].odep_list[i]);
	    /* blow away the consuming op list */
	    LSQ[LSQ_index].odep_list[i] = NULL;
	  }
	
	if (IS_MULT_LSQ(LSQ[LSQ_index].IR->op()))
	  lsq_mult--;
	
	/* squash this LSQ entry */
	LSQ[LSQ_index].tag++;
#if PTRACE == 1
	/* indicate in pipetrace that this instruction was squashed */
	ptrace_endinst(LSQ[LSQ_index].ptrace_seq);
#endif
	/* go to next earlier LSQ slot */
	LSQ_prev_tail = LSQ_index;
	LSQ_index = (LSQ_index + (LSQ_size-1)) % LSQ_size;
	LSQ_num--;
      }

      /* recover any resources used by this RUU operation */
      for (i=0; i<MAX_ODEPS; i++) {
	rs_free_list.RSLINK_FREE_LIST(RUU[RUU_index].odep_list[i]);
	/* blow away the consuming op list */
	RUU[RUU_index].odep_list[i] = NULL;
      }
      
      /* squash this RUU entry */
      RUU[RUU_index].tag++;
#if TRACER_DBG == 1
      printf(" squashing %p\n", (char*)RUU[RUU_index].IR->PC());
#endif
      if (isRemote) {	
	thr->condemn(RUU[RUU_index].IR);
      } else {
	thr->squash(RUU[RUU_index].IR);
	extraInstLat.erase(RUU[RUU_index].IR);
      }
#if PTRACE == 1
      /* indicate in pipetrace that this instruction was squashed */
      ptrace_endinst(RUU[RUU_index].ptrace_seq);
#endif
      /* go to next earlier slot in the RUU */
      RUU_prev_tail = RUU_index;
      RUU_index = (RUU_index + (RUU_size-1)) % RUU_size;
      RUU_num--;
    }

  /* reset head/tail pointers to point to the mis-predicted branch */
  RUU_tail = RUU_prev_tail;
  LSQ_tail = LSQ_prev_tail;

  /* revert create vector back to last precise create vector state, NOTE:
     this is accomplished by resetting all the copied-on-write bits in the
     USE_SPEC_CV bit vector */
  BITMAP_CLEAR_MAP(use_spec_cv, CV_BMAP_SZ);
  thr->squashSpec();

  /* FIXME: could reset functional units at squash time */
}

