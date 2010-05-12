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


#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "global.h"
#include "exceptions.h"
#include "sst/boost.h"

//:Instruction Types
//
// A JMP is unconditional, a Branch is conditional.
//!SEC:Framework
typedef enum{ NOOP, 
	      QUIT,
	      UNKNOWN,
	      TRAP,  
	      ALU,    
	      FP,    /* floating point */
	      LOAD,
	      STORE,
	      JMP,     /* unconditional */
	      BRANCH,   /* conditional */
	      VEC,      /* vector or VIS */
	      BUBBLE,
	      ISDEAD,
	      LASTINST /* NOTE: when adding new instructions, be
			  sure to also add to the array of strings
			  in tt7.cc in the correct order */
} instType;

extern const char *instNames[];

typedef enum {NEW,
	      FETCHED,
	      ISSUED,
	      COMMITTED,
	      RETIRED,
	      SQUASHED,
	      NOT_SUPP} instState;

class processor;
//: Simulated Instruction
//
// A key part of the processor/thread interface. 
//
// Instructions are retrieved from threads. They then go through
// several stages of completion (fetch(), issue(), commit()). The
// instruction can be queried for various information, such as its
// state (fetched, issued, etc...), program counter, required
// functional units, memory address accessed, operation type, etc...
//
// For backends which which to model out-of-order execution or branch
// prediction, things are a bit more complex. The back end it required
// to track the program counter. The back end must also detect when it
// has mispredicted, and squash instructions as appropriate. If it
// wishes to model speculative execution, it must supply a speculative
// mode to instructions which are issue()ing or
// commit()ing. Misspeculation can be detected with the use of the
// NPC() function.
//
//!SEC:Framework
class instruction 
{
 public:
  virtual ~instruction() {;}
  //: Fetch Instruction
  //
  // This "tells" the instruction that it needs to be fetched from
  // memory. It should be the first thing called upon a new
  // instruction after that instruction has been received from a
  // thread.
  //
  // After fetch completes, the instruction should be able to give its
  // PC (instruction address) so the backend can simulate instruction
  // fetch from a cache.
  //
  // Returns true upon success.
  //
  //!NOTE: I'm not sure when fetch would need to fail. Possibly in the
  //!OOO case?
  virtual bool fetch(processor*)=0;
  //: Issue Instruction
  //
  // This tells the instruction that it needs to be "issued", such as
  // to a functional unit. It should be called after instruction::fetch().
  //
  // after the issue(), the instruction should be able to give its
  // opcode type, and what memory address (memEA) it needs to access
  // (if it is a memory access).
  //
  // Returns true upon success. Should never fail in the serial
  // in-order case. It may fail in the OOO case if the instruction
  // cannot issue because of a data dependance. ex: Cannot compute its
  // memEA because a source register has not been filled in.
  virtual bool issue(processor*)=0;
  //: Commit Instruction
  //
  // This tells the thread it can complete its computation and commit
  // its results to the thread's state.
  //
  // After an instrucion successfully commits, it can be retired back
  // to its issueing thread.
  //
  // Returns true upon success. It may fail, if it does so, it should
  // be able to give the exception which caused its failure. (thru
  // instruction::exception()).
  virtual bool commit(processor*)=0; 

  //: return Next PC
  //
  // returns the program counter which should follow this
  // instruction. Available after commit. Useful to detect
  // mispredicted instructions.
  virtual simRegister NPC() const =0;
  //: return TargetPC
  virtual simRegister TPC() const =0;
  //: Issue, possibly speculatively
  //
  // For the out-of-order case, backends can fetch "incorrect"
  // instructions. The backend must detect when this has occured and
  // supply the correct speculation mode to the instruction.
  virtual bool issue(processor *p, const bool)=0; 
  // Commit, possibly speculatively
  //
  // For the out-of-order case, backends can fetch "incorrect"
  // instructions. The backend must detect when this has occured and
  // supply the correct speculation mode to the instruction.
  virtual bool commit(processor *p, const bool)=0;
  //: Functional unit requirements
  //
  // return the type of functional unit required for this
  // instruction. Follows simpleScalar FU designations.
  virtual int fu() const =0;
  //: Specific Operation
  //
  // follows simpleScalar semantics for more detailed opcode flags
  virtual int specificOp() const =0;
  //: Output dependancies
  //
  // returns a -1 terminated list of output dependancies. The
  // dependancies are register numbers, starting at 1. First integer
  // registers, then FP registers, then any special registers as
  // required.
  virtual const int* outDeps() const =0;
  //: Input dependancies
  //
  // returns a -1 terminated list of input register dependancies. The
  // dependancies are register numbers, starting at 1. First integer
  // registers, then FP registers, then any special registers as
  // required.
  virtual const int* inDeps() const =0;
  //: Is this a function return
  //
  // is this instruction returning from an function call. Used for
  // branch predictors.
  virtual bool isReturn() const =0;

  //: Instruction state
  virtual instState state() const =0;
  //: Accessor for opcode type
  virtual instType op() const =0;
  //: Accessor for program counter (instruction address)
  virtual simAddress PC() const =0;
  //: Accessor for effective address
  // For a load or store
  virtual simAddress memEA() const =0;
  //: Offset for memory instructions
  virtual int memOffset(bool &valid) const {valid=0; return -1;}
  //: Accessor for execption type
  // of an instruction which failed to commit.
  virtual exceptType exception() const =0;

  virtual simPID pid() const =0;
  //: Accessor for target destination
  //
  // If a thread issues an instruction directing it to move (migrate)
  // to another LWP location, it should give the MOVE_TO_EXCEPTION and
  // then set the location it wishes to move
  // to. instruction::exception() accesses that target address.
  virtual simAddress moveToTarget() const =0;
  //: Accessor for FEB address
  //
  // If an instruction cannot complete a load or store instruction due
  // to Full/Empty Bit complications, it should raise the
  // FEB_EXCEPTION and instruction::febTarget() should return the
  // address of the data item it was attempting to access.
  virtual simAddress febTarget() const =0;
};

#endif
