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

#ifndef SSB_RUU_H
#define SSB_RUU_H

#include "instruction.h"
#include "ssb_bpred.h"
#include "ssb_machine.h"

/* total input dependencies possible */
#define MAX_IDEPS               5

/* total output dependencies possible */
#define MAX_ODEPS               5

//: register update unit (RUU) station
//
// A register update unit (RUU) station, this record is contained in
// the processors RUU, which serves as a collection of ordered
// reservations stations. The reservation stations capture register
// results and await the time when all operands are ready, at which
// time the instruction is issued to the functional units; the RUU is
// an order circular queue, in which instructions are inserted in
// fetch (program) order, results are stored in the RUU buffers, and
// later when an RUU entry is the oldest entry in the machines, it and
// its instruction's value is retired to the architectural register
// file in program order, NOTE: the RUU and LSQ share the same
// structure, this is useful because loads and stores are split into
// two operations: an effective address add and a load/store, the add
// is inserted into the RUU and the load/store inserted into the LSQ,
// allowing the add to wake up the load/store when effective address
// computation has finished
//
//!SEC:ssBack
struct RUU_station {
  /* inst info */
  instruction *IR;			/* instruction bits */
  instType op;
  md_addr_t PC, next_PC, pred_PC;	/* inst PC, next PC, predicted PC */
  int in_LSQ;				/* non-zero if op is in LSQ */
  int ea_comp;				/* non-zero if op is an addr comp */
  int recover_inst;			/* start of mis-speculation? */
  int stack_recover_idx;		/* non-speculative TOS for RSB pred */
  struct bpred_update_t dir_update;	/* bpred direction update info */
  int spec_mode;			/* non-zero if issued in spec_mode */
  md_addr_t addr;			/* effective address for ld/st's */
  INST_TAG_TYPE tag;			/* RUU slot tag, increment to
					   squash operation */
  INST_SEQ_TYPE seq;			/* instruction sequence, used to
					   sort the ready list and tag inst */
  unsigned int ptrace_seq;		/* pipetrace sequence number */

  /* instruction status */
  int queued;				/* operands ready and queued */
  int issued;				/* operation is/was executing */
  int completed;			/* operation has completed execution */

  //: ouput dependency list
  // output operand dependency list, these lists are used to limit the
  // number of associative searches into the RUU when instructions
  // complete and need to wake up dependent insts
  int onames[MAX_ODEPS];		/* output logical names (NA=unused) */
  struct RS_link *odep_list[MAX_ODEPS];	/* chains to consuming operations */

  //: input dep list
  //
  // input dependent links, the output chains rooted above use these
  // fields to mark input operands as ready, when all these fields
  // have been set non-zero, the RUU operation has all of its register
  // operands, it may commence execution as soon as all of its memory
  // operands are known to be read (see lsq_refresh() for details on
  // enforcing memory dependencies)
  int idep_ready[MAX_IDEPS];		/* input operand ready? */

  int lsq_count;         /* Number of memory ops */
};

#endif
