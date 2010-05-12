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


#include "pimSysCallDefs.h"
#include "ppcFront.h"
#include "regs.h"
#include "fe_debug.h"
#include <sim_trace.h>
#include <sst/simulation.h> // for current cycle count (i.e. Simulation)

ppcThread* ppcInstruction::createThreadWithStack(processor* proc,
						 simRegister *regs)
{
    // create new thread
    ppcThread *newT = new ppcThread(proc, pid());
    if (newT) {
	// copy all registers, so we get globals.
	for (int i = 0; i < 32+64; ++i) {
	    newT->packagedRegisters[i] = regs[i];
	}
	memcpy(newT->ppcRegisters, parent->ppcRegisters, sizeof(*parent->ppcRegisters));
	// set up argument
	newT->packagedRegisters[3] = regs[5];
	newT->packagedRegisters[4] = regs[6];
	newT->packagedRegisters[5] = regs[7];
	newT->packagedRegisters[6] = regs[8];
	newT->packagedRegisters[7] = regs[9];
	newT->ppcRegisters->regs_L = 0; // return address
	// set programcounter
	newT->ProgramCounter = regs[4];
	newT->setMigrate(false);

	int loc = regs[3];
	uint stack_size = proc->maxLocalChunk();
	simAddress stack = 0;
	if (loc == -1) {
	    static int locrr = 0;
	    loc = locrr++;
	    locrr *= (locrr < (int)proc->numLocales());
	    INFO("sending thread to loc %i\n", loc);
	    stack = proc->localAllocateAtID(stack_size, loc);
	} else if (loc < 0) {
	    ERROR("specified locale (%i) not valid (too small) (max %u, min 0, auto=-1)!\n", loc, (unsigned)memory::numLocales());
	} else if (loc > (int)proc->numLocales()) {
	    /* according to the documentation, this means we pick somewhere near r5
	     * (which became r3) Note that we aren't paying attention to virtual
	     * addresses here. If we need to, that should be moved into the memory
	     * controller. */
	    //printf("allocating thread stack near 0x%x\n", regs[3]);
	    stack = proc->localAllocateNearAddr(stack_size, regs[3]);
	} else {
	    /* loc is within the appropriate range! */
	    INFO("sending thread to loc %i (as requested)\n", loc);
	    stack = proc->localAllocateAtID(stack_size, loc);
	}
	if (stack == 0) {
	    stack = proc->globalAllocate(stack_size);
	    if (stack == 0) {
		ERROR("Ran out of global memory for thread stacks!\n");
	    }
	}
	stack += stack_size;
	newT->packagedRegisters[1] = stack - 256;
	//newT->setStackLimits(stack, stack - stack_size);
	newT->SequenceNumber = regs[3] = ppcThread::nextThreadID++;
	ppcThread::threadIDMap[regs[3]] = newT;
    } else {
	ERROR("couldn't allocate new thread\n");
    }
    return newT;
}

/* This creates a PIM thread with a stack, but does not schedule the thread to be run
 * r3 specifies which 'locale' to go to.
 * r4 is the PC to start running at (i.e. the address of the function to run)
 * r5-r9 become the arguments to the new thread (r3-r7)
 * Note: new thread is set to non-migrateable
 */
bool ppcInstruction::Perform_PIM_SPAWN_TO_LOCALE_STACK_STOPPED(processor* proc,
							       simRegister *regs)
{
  ERROR("Needs to be fixed");
  if (createThreadWithStack(proc, regs)) {
    return true;
  } else {
    return false;
  }
}

bool ppcInstruction::Perform_PIM_START_STOPPED_THREAD(processor* proc,
						       simRegister *regs)
{
  ERROR("Needs to be fixed");
    // fetch the thread pointer som
    ppcThread *stoppedT = ppcThread::threadIDMap[regs[3]];
    // set it in motion, we use its stack to hint where it should start
    if (proc->spawnToCoProc(PIM_ANY_PIM, stoppedT,
			    stoppedT->packagedRegisters[1])) {
      // return the status
      return true;
    } else {
	regs[3] = 0;
	return false;
    }
}

//: This creates a PIM thread with a stack
//
// r3 specifies which 'locale' to go to. 
// r4 is the PC to start running at (i.e. the address of the function to run)
// r5-9 become the arguments to the new thread (r3-r7)
// Note: new thread is set to non-migrateable
bool ppcInstruction::Perform_PIM_SPAWN_TO_LOCALE_STACK(processor* proc,
						       simRegister *regs) {
  // create new thread
  ppcThread *newT = createThreadWithStack(proc, regs);
  if (newT) {
    if (proc->spawnToCoProc(PIM_ANY_PIM, newT, newT->packagedRegisters[1])) {
      return true;
    } else {
      ppcThread::threadIDMap.erase(regs[3]);
      regs[3] = 0;
      delete newT;
      return false;
    }
  } else {
    ERROR("createThreadWithStack failed\n");
    return 0;
  }
}

/* This sets up a new thread.
 * r1 is the stack ptr
 * r3 specifies which processor to go to (2 means "any")
 * r4 is the PC to start running at (i.e. the address of the function to run)
 */

bool ppcInstruction::Perform_PIM_SPAWN_TO_COPROC(processor* proc,
						 simRegister *regs) {
  // create new thread
  ppcThread *newT = new ppcThread(proc, pid());
  if (newT) {
    // copy all registers, so we get globals.
    for (int i = 0; i < 32+64; ++i) {
      newT->packagedRegisters[i] = regs[i];
    }
    memcpy(newT->ppcRegisters, parent->ppcRegisters, 
	   sizeof(*parent->ppcRegisters));
    // set up argument
    newT->packagedRegisters[3] = regs[5];
    newT->packagedRegisters[4] = regs[6];
    newT->packagedRegisters[5] = regs[7];
    newT->packagedRegisters[6] = regs[8];
    newT->packagedRegisters[7] = regs[9];
    // set up stack (if needed)
    if (ppcInstruction::magicStack) 
      newT->packagedRegisters[1] = ppcInitStackBase; //stack pointer
    newT->ppcRegisters->regs_L = 0; // return address
    //set progamcounter
    newT->ProgramCounter = regs[4];
    //we don't know the min stack address
    //newT->setStackLimits(0, 0);
    PIM_coProc coProcTarg = (PIM_coProc)regs[3];
#if 0
    // assign the thread an ID
    newT->SequenceNumber = regs[3] = ppcThread::nextThreadID++;
    ppcThread::threadIDMap[regs[3]] = newT;
#endif
    // use stack as hint
    if (proc->spawnToCoProc(coProcTarg, newT, newT->packagedRegisters[1])) {
	return true;
    } else {
      delete newT;
      ppcThread::threadIDMap.erase(regs[3]);
      regs[3] = 0;
      return false;
    }
  } else {
    ERROR("couldn't allocate new thread\n");
    return 0;
  }
}

bool ppcInstruction::Perform_PIM_SWITCH_ADDR_MODE(processor* proc,
						  simRegister *regs) {
  bool ret = proc->switchAddrMode((PIM_addrMode)regs[3]);

  regs[3] = ret;

  return ret;
}

bool ppcInstruction::Perform_PIM_QUICK_PRINT(processor	*FromProcessor,
					     simRegister	*regs_R)
{
    printf("OUTPUT: %x(%d) %x(%d) %x(%d)\n",
	   (unsigned int)ntohl(regs_R[3]), (unsigned int)ntohl(regs_R[3]),
	   (unsigned int)ntohl(regs_R[4]), (unsigned int)ntohl(regs_R[4]),
	   (unsigned int)ntohl(regs_R[5]), (unsigned int)ntohl(regs_R[5]));
    fflush(stdout);

    // return values
    regs_R[3] = 0;
    return true;
}

bool ppcInstruction::Perform_PIM_TRACE(processor	*FromProcessor,
					     simRegister	*regs_R) {
  
#if 0
  SIM_trace( simulation::cycle, 
	FromProcessor->getProcNum()/2, 
	(FromProcessor->getProcNum()%2) ? "PPC" : "Opteron",
	regs_R[3], regs_R[4], regs_R[5] );
#endif

  // return values
  regs_R[3] = 0;

  return true;
}

// This is the implementation of PIM_fastFileRead()
bool ppcInstruction::Perform_PIM_FFILE_RD(processor* proc,
					  simRegister *regs) {
  simAddress cstr = ntohl(regs[3]);
  unsigned int len = 0;
  char buf[1024];
  char c;

  while ((len < 1023) && ( (c = proc->ReadMemory8(cstr + len,0)) != 0)) {
    buf[len] = c;
    len++;
  }
  buf[len] = 0;

  FILE *fp = fopen (buf, "r");

  //printf ("Opening file %s fp %p first char %c\n", buf, fp, fgetc(fp));

  if (fp == NULL) {
    fprintf (stderr, "Error trying fast page in of file %s\n", buf);
    regs[3] = 0;
    return true;
  }

  simAddress oBuf = ntohl(regs[4]);
  unsigned int maxB = ntohl(regs[5]);
  unsigned int offS = ntohl(regs[6]);

  //printf ("Seeking forward %d bytes, max read %d bytes buffer %x\n",
  //  offS, maxB, oBuf);

  if (fseek (fp, (long)offS, SEEK_SET) != 0) {
    fprintf (stderr, "Error trying to seek to %d in file %s\n", offS, buf);
    regs[3] = 0;
    return true;
  }

  len = 0;
  for (uint i = 0; i < maxB; i++) {
    if ( (c = fgetc(fp)) == EOF ) {
      i = maxB;
    } else {
      proc->WriteMemory8(oBuf + len, c, 0);
      //printf ("Write to memory %x, c = %c\n", oBuf + len, c);
      len++;
    }
  }

  regs[3] = ntohl(len);

  fclose(fp);
  return true;
}

#include "rand.h"
bool ppcInstruction::Perform_PIM_RAND(processor *proc,
				      simRegister *regs) {
  unsigned int r = SimRand::rand();
  regs[3] = ntohl(r);
  return true;
}

#define ALLOC_GLOBAL 0
#define ALLOC_LOCAL_ADDR 1
#define ALLOC_LOCAL_ID 2

bool ppcInstruction::Perform_PIM_MALLOC(processor *proc,
					simRegister *regs) {
  unsigned int size = ntohl(regs[3]);
  unsigned int allocType = ntohl(regs[4]);
  unsigned int opt = ntohl(regs[5]);
  unsigned int addr;

  INFO ("PIM_MALLOC: size %d type %d PC %x)\n", size, allocType, PC());

  switch (allocType) {
  case ALLOC_GLOBAL:
    addr = proc->globalAllocate(size);
    break;
  default:
  case ALLOC_LOCAL_ADDR:
    addr = proc->localAllocateNearAddr(size, opt);
    break;
  case ALLOC_LOCAL_ID:
    addr = proc->localAllocateAtID(size, opt);
    break;
  }

  regs[3] = ntohl(addr);

  return true;
}

bool ppcInstruction::Perform_PIM_FREE(processor *proc,
				      simRegister *regs) {
  unsigned int addr = ntohl(regs[3]);
  unsigned int size = ntohl(regs[4]);

  //XXX: This probably does not interact well with mapped memory, but just
  //     using getPhysAddr won't fix it (KBW)
  unsigned int result = memory::memFree(addr, size);

  if (result == 0) {
    ERROR("Fast Free failed \n");
#if 0
    printf("%p@%d: Commit %6s %p %p(%lx) %8p %8p %llu\n", 
	   this->parent, proc->getProcNum(), MD_OP_NAME(simOp), 
	   (void*)ProgramCounter, (void*)_memEA, 
	   proc->ReadMemory32(_memEA,0), (void*)(parent->getRegisters())[0], 
	   (void*)(parent->getRegisters())[31], 
	   component::cycle()); 
    printf ("Memory at %x: %x\n", (uint)ProgramCounter, (uint)proc->ReadMemory32(ProgramCounter, 0));
    printf ("Op returned from %x: %x, simOp %x\n", (uint)ProgramCounter, (uint)getOp(ProgramCounter), simOp);

    this->parent->printCallStack();
    fflush(stdout);
#endif
  }

  regs[3] = ntohl(result);

  return true;
}

//: Write directly to memory
//
// bypass cache, FU, etc...
bool ppcInstruction::Perform_PIM_WRITE_MEM(processor* proc, 
					   simRegister *regs) {
  simAddress addr = ntohl(regs[3]);
  simRegister data = regs[4];

  proc->WriteMemory32(ntohl(addr), data, false); // false means
						 // "non-speculatively"

  return true;
}

bool ppcInstruction::Perform_PIM_WRITE_SPECIAL(processor* proc, 
					       simRegister *regs,
					       const int num) {
  exceptType ret = NO_EXCEPTION;
  switch ((PIM_cmd)ntohl(regs[3])) {
  case PIM_CMD_SET_EVICT:
    parent->setEvict(ntohl(regs[4]) > 0);
    break;
  case PIM_CMD_SET_MIGRATE:
    parent->setMigrate(ntohl(regs[4]) > 0);
    break;
  case PIM_CMD_SET_THREAD_ID:
    //printf("set TID: thread %p -> %x (seq %d)\n", parent, (uint)regs[4], myS);
    parent->ThreadID = ntohl(regs[4]);
    break;
  case PIM_CMD_SET_FUTURE:
    parent->isFuture = (ntohl(regs[4]) != 0);
    break;

  default:
    ret = proc->writeSpecial((PIM_cmd)ntohl(regs[3]), // pim_cmd
			     num, // number of arguments
			     (uint*)&regs[4]);
  }

  if (ret == NO_EXCEPTION) {
    return true;
  } else {
    return false;
  }
}

bool ppcInstruction::Perform_PIM_READ_SPECIAL(processor *proc,
					      simRegister *regs, 
					      const int numIn,
					      const int numOut) {
  simRegister rets[numOut];
  exceptType ret = NO_EXCEPTION;

  static simAddress *local_ctrl;

  switch((PIM_cmd)ntohl(regs[3])) {
  case PIM_CMD_LOCAL_CTRL:
    if (local_ctrl == NULL) {
      local_ctrl = new simAddress[memory::numLocales()];
      for (unsigned int i = 0; i < memory::numLocales(); i++) 
	local_ctrl[i] = memory::localAllocateAtID(memory::maxLocalChunk(), i);
    }
    if ((unsigned)ntohl(regs[4]) > (unsigned)memory::numLocales())
      rets[0] = ntohl(local_ctrl[memory::getLocalID(proc)]);
    else
      rets[0] = ntohl(local_ctrl[regs[4]]);
    break;
  case PIM_CMD_LOC_COUNT:
    /* the number of different memory timing regions */
    rets[0] = ntohl(memory::numLocales());
    break;
  case PIM_CMD_MAX_LOCAL_ALLOC:
    /* the maximum size of contiguous memory that can be allocated that will
     * all be within the same timing region */
    rets[0] = ntohl(memory::maxLocalChunk());
    break;
  case PIM_CMD_CYCLE:
    /* the number of cycles executed thus far */
    //rets[0] = htonl((simRegister)parent->home->getCurrentSimTime());
    rets[0] = htonl((simRegister)Simulation::getSimulation()->getCurrentSimCycle());
    break;
  case PIM_CMD_ICOUNT:
    /* the number of committed instructions */
    rets[0] = ntohl((unsigned)ppcInstruction::totalCommitted);
    break;
  case PIM_CMD_THREAD_ID:
    /* the thread ID (this must be set by the thread, defaults to 0) */
    processor::checkNumArgs((PIM_cmd)ntohl(regs[3]), numIn, numOut, 0, 1); //XXX: What is this for?
    rets[0] = ntohl(parent->ThreadID);
    //printf("get TID: thread %p = %x\n", parent, rets[0]);
    break;
  case PIM_CMD_THREAD_SEQ:
    /* the thread sequence number (this can NOT be set by the thread) */
    processor::checkNumArgs((PIM_cmd)ntohl(regs[3]), numIn, numOut, 0, 1); //XXX: What is this for?
    rets[0] = ntohl(parent->SequenceNumber);
    //rets[0] = ppcThread::threadSeq[parent];
    break;
  case PIM_CMD_SET_FUTURE:
    /* a boolean mark on a thread; whether it is a future (i.e. subject to
     * futurelib bookkeeping) or not */
    rets[0] = ntohl(parent->isFuture);
    break;
  case PIM_CMD_GET_NUM_CORE:
    /* the number of cores per chip (why does anyone care?) */
    rets[0] = ntohl(proc->getNumCores());
    break;
  case PIM_CMD_GET_CORE_NUM:
    /* the number of the core we're currently executing on */
    rets[0] = ntohl(proc->getCoreNum());
    break;
  case PIM_CMD_GET_MHZ:
    //rets[0] = proc->ClockMhz();
    WARN("Get_Mhz not supported\n");
    break;
  case PIM_CMD_GET_CTOR:
    rets[0] = ntohl(parent->loadInfo.constrSize);
    rets[1] = ntohl(parent->loadInfo.constrLoc);
    break;
  default:
    ret = proc->readSpecial((PIM_cmd)ntohl(regs[3]), //pim_cmd
			    numIn, // number of Input arguments
			    numOut, // number of Output arguments
			    &regs[4], 
			    rets);
  }

  if (ret != NO_EXCEPTION) {
    _exception = ret;
    return false;
  } else {
    _exception = ret;
    for (int i = 0; i < numOut; ++i) 
      regs[3+i] = rets[i];
    return true;
  }
}

bool ppcInstruction::Perform_PIM_READFX(processor	*proc,
					  simRegister	*regs) 
{
  simAddress addr = ntohl(regs[3]);
  //printf ("readfx %x\n", addr); fflush(stdout);

  if (proc->getFE(addr) == 1) {
    // it is full, do the read, and set the bit as needed
    if (regs[0] == ntohl(SS_PIM_READFE)) {
      // if we need to, empty the FE bit
      proc->setFE(addr, 0);
    } 
    regs[3] = ntohl(proc->ReadMemory32(addr,0));

    return true;  
  } else {
    _exception = FEB_EXCEPTION;
    _febTarget  = addr;

    return false;  
  }
  
}

bool ppcInstruction::Perform_PIM_ATOMIC_INCREMENT(processor	*proc,
					  simRegister	*regs) 
{
  simAddress addr = ntohl(regs[3]);
  simRegister sum = ntohl(proc->ReadMemory32(addr,0));
  //return the original value
  regs[3] = ntohl(sum);

  sum += ntohl(regs[4]);
  //store the incremented value
  proc->WriteMemory32(addr, ntohl(sum), 0);

  //set feb to full
  //proc->setFE(addr,1);

  return true;
}

bool ppcInstruction::Perform_PIM_ATOMIC_DECREMENT(processor	*proc,
					  simRegister	*regs) 
{
  simAddress addr = ntohl(regs[3]);
  simRegister sum = ntohl(proc->ReadMemory32(addr,0));
  //return the original value
  regs[3] = ntohl(sum);

  sum -= ntohl(regs[4]);
  //store the incremented value
  proc->WriteMemory32(addr, ntohl(sum), 0);

  //set feb to full
  //proc->setFE(addr,1);

  return true;
}

bool ppcInstruction::Perform_PIM_BULK_set_FE(processor	*proc,
					     simRegister	*regs,
					     simRegister val) 
{
  simAddress addr = ntohl(regs[3]);
  simRegister len = ntohl(regs[4]);

  for (int i = 0; i < len; ++i) {
    proc->setFE(addr+i, val);
  }

  regs[3] = 0;

  return true;
}

bool ppcInstruction::Perform_PIM_CHANGE_FE(processor	*proc,
					     simRegister	*regs) 
{
  simAddress addr = ntohl(regs[3]);

  switch (ntohl(regs[0])) {
  case SS_PIM_FILL_FE:
    proc->setFE(addr, 1);
    break;
  case SS_PIM_EMPTY_FE:
    proc->setFE(addr, 0);
    break;
  default:
    WARN("unknown: %x %d\n", ntohl(regs[0]), ntohl(regs[3]));
    ERROR("Unrecognized PIM_CHANGE_FE type.\n");
    break;
  }

  regs[3] = 0;

  return true;
}

bool ppcInstruction::Perform_PIM_UNLOCK(processor *proc, simRegister *regs)
{
  int current_state = proc->getFE(ntohl(regs[3]));
  if (base_memory::getDefaultFEB() == 1) {
    regs[0] = ntohl(SS_PIM_FILL_FE);
    if (current_state != 0) {
      INFO("unlocking a currently unlocked address: %p\n", 
	   (void*)(size_t)ntohl(regs[3]));
    }
  } else {
    if (current_state == 0) {
      INFO("unlocking a currently unlocked address: %p\n", 
	   (void*)(size_t)ntohl(regs[3]));
    }
    regs[0] = ntohl(SS_PIM_EMPTY_FE);
  }
  return Perform_PIM_CHANGE_FE(proc, regs);
}

bool ppcInstruction::Perform_PIM_LOCK(processor *proc, simRegister *regs)
{
    if (base_memory::getDefaultFEB() == 1) {
      regs[0] = ntohl(SS_PIM_READFE);
      return Perform_PIM_READFX(proc, regs);
    } else {
      // regs[4] = 1;
      return Perform_PIM_WRITEEF(proc, regs);
    }
}

bool ppcInstruction::Perform_PIM_WRITEEF(processor	*proc,
					 simRegister	*regs) {
  simAddress addr = ntohl(regs[3]);
  //printf ("writeef %x (%d)\n", addr, proc->ReadMemory32(addr,0)); fflush(stdout);

  if (proc->getFE(addr) == 0) {
    // its empty to write and fill
    proc->WriteMemory32(addr, htonl(regs[4]), 0);
    proc->setFE(addr, 1);

    regs[3] = 0;
    return true;
  } else {
    // fail, raise exception
    _exception = FEB_EXCEPTION;
    _febTarget  = addr;

    return false;  
  }
}

bool ppcInstruction::Perform_PIM_IS_FE_FULL(processor	*proc,
					    simRegister	*regs) {
  simAddress addr = ntohl(regs[3]);
  //printf ("isfull %x\n", addr); fflush(stdout);

  regs[3] = ntohl(proc->getFE(addr));

  return true;
}

// NOTE: returns 0 for success, just like mutex_trylock
bool ppcInstruction::Perform_PIM_TRYEF(processor	*proc,
				       simRegister	*regs) {
  simAddress addr = ntohl(regs[3]);
  //printf ("tryef %x\n", addr); fflush(stdout);

  if (proc->getFE(addr) == 0) {
    // its empty, so we can fill it
    proc->setFE(addr, 1);
    regs[3] = 0;
  } else {
    // the FEB is already full, so return 1
    regs[3] = ntohl(1);
  }

  return true;
}

bool ppcInstruction::Perform_PIM_FORK(processor	*proc,
				      simRegister	*regs) {
  ERROR("Needs to be fixed");
  ppcThread *newT = new ppcThread(proc, pid());
  bool ret = proc->insertThread(newT);
  if (ret) {
    // init new thread
#if 0
    simRegister *newR = proc->getFrame(newT->registers);
#else
    simRegister *newR = newT->getRegisters();
#endif
    // copy all registers, so we get globals.
    for (int i = 0; i < 32; ++i) {
      newR[i] = regs[i];
    }
    // set up argument
    newR[3] = regs[4];

    // set up stack
    //newR[1] = 0x7fff0000; 
    if (magicStack) newR[1] = ppcInitStackBase; //stack pointer 

    newT->ppcRegisters->regs_L = 0; // return address
    newT->ProgramCounter = regs[3]; // set PC

    //we don't know the min stack address
    //newT->setStackLimits (0,0);

    //return values
    regs[3] = 0;
  } else {
    regs[3] = 0;
    delete newT;
  }

  return ret;
}

bool ppcInstruction::Perform_PIM_RESET(processor	*proc,
				      simRegister	*regs) {
  proc->resetCounters();
  return true;
}


bool ppcInstruction::Perform_PIM_EXIT(processor	*proc,
				      simRegister *regs) {
  //printf("%p: PIM_EXIT\n", parent);
  parent->_isDead = 1;
  return true;
}

bool ppcInstruction::Perform_PIM_EXIT_FREE(processor	*proc,
					   simRegister *regs) {
  //printf("%p: PIM_EXIT_FREE\n", parent);
  
  //parent->freeStack();
  parent->_isDead = 1;
  return true;
}

bool ppcInstruction::Perform_PIM_MEM_REGION_CREATE(processor    *proc,
                                             simRegister        *regs )
{
  int region         = ntohl(regs[3]);
  simAddress vaddr   = ntohl(regs[4]);
  unsigned long size = ntohl(regs[5]);
  simAddress kaddr   = ntohl(regs[6]);

  //DPRINT(0,"region=%d vaddr=%#lx size=%d kaddr=%#lx\n",
  //region,(long unsigned int)vaddr,(int)size,(long unsigned int)kaddr,regs[7]);

  memType_t type;
  switch( ntohl(regs[7]) ) {
    case PIM_REGION_CACHED:
	type = CACHED;
	break;
    default:
    case PIM_REGION_UNCACHED:
	type = UNCACHED;
	break;
    case PIM_REGION_WC:
	type = WC;
	break;
  }
  regs[3] = ntohl(proc->CreateMemRegion(region, vaddr, size, kaddr, type ));

  return true;
}

bool ppcInstruction::Perform_PIM_MEM_REGION_GET(processor       *proc,
                                             simRegister        *regs )
{
  int region           = ntohl(regs[3]);
  unsigned long addr  = (unsigned long)ntohl(regs[4]);
  unsigned long len   = (unsigned long)ntohl(regs[5]);

  DPRINT(0,"region=%d addr=%#lx len=%#lx\n", region, addr, len);

  switch( region ) {
     case PIM_REGION_DATA:
       proc->WriteMemory32(addr, ntohl(parent->loadInfo.data_addr), 0);
       proc->WriteMemory32(len, ntohl(parent->loadInfo.data_len), 0);
       break;
     case PIM_REGION_STACK:
       proc->WriteMemory32(addr, ntohl(parent->loadInfo.stack_addr), 0);
       proc->WriteMemory32(len, ntohl(parent->loadInfo.stack_len), 0);
       break;
     case PIM_REGION_TEXT:
       proc->WriteMemory32(addr, ntohl(parent->loadInfo.text_addr), 0);
       proc->WriteMemory32(len, ntohl(parent->loadInfo.text_len), 0);
       break;
     case PIM_REGION_HEAP:
       proc->WriteMemory32(addr, ntohl(parent->loadInfo.heap_addr), 0);
       proc->WriteMemory32(len, ntohl(parent->loadInfo.heap_len), 0);
       break;
  }

  return true;
}
