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


#include "global.h"
#include "ppcFront.h"
#include "processor.h"
#include "ppcMachine.h"

extern	"C"
{
#include "ppc_syscall.h"
#include "pimSysCallDefs.h"
}

//:System call dispatcher for issue()
//
// A large case statement which dispatches a system call to the
// apporpiate sub-function
void ppcInstruction::issueSystemTrap(processor* proc,
				     uint32 *AtInstruction)
{
  simRegister *registers = parent->getRegisters();

  if ((int)ntohl(registers[0]) < 0) {
    // mach call
    return;
  }

  switch ( (int)ntohl(registers[0]) )
    {
    case 0:
      break;
    case	PPC_SYS_exit:
    case	PPC_SYS_fork:
    case	PPC_SYS_vfork:
    case	PPC_SYS_read:
    case	PPC_SYS_write:
    case	PPC_SYS_open:
    case	PPC_SYS_close:
    case	PPC_SYS_unlink:
    case	PPC_SYS_chdir:
    case	PPC_SYS_chmod:
    case	PPC_SYS_chown:
    case	PPC_SYS_getuid:
    case	PPC_SYS_lseek:
    case	PPC_SYS_access:
    case	PPC_SYS_dup2:
    case	PPC_SYS_dup:
    case	PPC_SYS_fcntl:
    case	PPC_SYS_fstat:
    case	PPC_SYS_fsync:
    case	PPC_SYS_getgid:
    case	PPC_SYS_gettimeofday:
    case	PPC_SYS_ioctl:
    case	PPC_SYS_lstat:
    case	PPC_SYS_pipe:
    case	PPC_SYS_select:
    case	PPC_SYS_stat:
    case	PPC_SYS_writev:
    case PPC_SYS_sigprocmask:
    case PPC_SYS_getpid:
    case PPC_SYS_kill:
    case PPC_SYS_getrusage:
    case PPC_SYS_getrlimit:
    case PPC_SYS___sysctl:
    case PPC_SYS_issetugid:
    case SS_PIM_FORK:
    case SS_PIM_EXIT:
    case SS_PIM_EXIT_FREE:
    case SS_PIM_LOCK:
    case SS_PIM_UNLOCK:
    case SS_PIM_IS_LOCAL:
    case SS_PIM_ALLOCATE_LOCAL:
    case SS_PIM_MOVE_TO:
    case SS_PIM_MOVE_AWAY:
    case SS_PIM_QUICK_PRINT:
    case SS_PIM_TRACE:
    case SS_PIM_RAND:
    case SS_PIM_MALLOC:
    case SS_PIM_FREE:
    case SS_PIM_RESET:
    case SS_PIM_NUMBER:
    case SS_PIM_REMAP:
    case SS_PIM_REMAP_TO_ADDR:
    case SS_PIM_MEM_REGION_CREATE:
    case NETSIM_SYS_ENTER:
    case NETSIM_TX_ENTER:
    case NETSIM_SYS_PICKUP:
      break;
    case SS_PIM_FFILE_RD:
      _memEA = registers[4];
      _op = STORE;  // so the back end knows to check if it needs to migrate/remote
      break;
    case SS_PIM_MEM_REGION_GET:
      break;
    case SS_PIM_ATOMIC_INCREMENT:
    case SS_PIM_ATOMIC_DECREMENT:
      _memEA = registers[3];
      _op = STORE;  // so the back end knows to check if it needs to migrate/remote
      _fu = WrPort;
      break;
    case SS_PIM_EST_STATE_SIZE:
      break;
    case SS_PIM_WRITE_MEM:
    case SS_PIM_WRITEEF:
    case SS_PIM_FILL_FE:
    case SS_PIM_EMPTY_FE:
    case SS_PIM_BULK_EMPTY_FE:
    case SS_PIM_BULK_FILL_FE:
    case SS_PIM_TRYEF:
      _memEA = registers[3];
      _op = STORE;
      _fu = WrPort;
      break;
    case SS_PIM_READFF:
    case SS_PIM_READFE: // should ReadFE be a store?
    case SS_PIM_IS_FE_FULL:
      _memEA = registers[3];
      _op = LOAD;
      _fu = RdPort;
      break;
    case SS_PIM_IS_PRIVATE:
    case SS_PIM_TID:
    case SS_PIM_REMAP_TO_POLY:
    case SS_PIM_TAG_INSTRUCTIONS:
    case SS_PIM_TAG_SWITCH:
    case SS_PIM_SPAWN_TO_COPROC:
    case SS_PIM_SPAWN_TO_LOCALE_STACK:
    case SS_PIM_SPAWN_TO_LOCALE_STACK_STOPPED:
    case SS_PIM_START_STOPPED_THREAD:
    case SS_PIM_SWITCH_ADDR_MODE:
      break;
    case SS_PIM_WRITE_SPECIAL:
    case SS_PIM_WRITE_SPECIAL2:
    case SS_PIM_WRITE_SPECIAL3:
      _fu = WrPort;
      break;
    case SS_PIM_RW_SPECIAL3:
    case SS_PIM_READ_SPECIAL:
    case SS_PIM_READ_SPECIAL1:
    case SS_PIM_READ_SPECIAL2:
    case SS_PIM_READ_SPECIAL3:
    case SS_PIM_READ_SPECIAL1_2:  
    case SS_PIM_READ_SPECIAL1_5:
    case SS_PIM_WRITE_SPECIAL5:
    case SS_PIM_WRITE_SPECIAL4:
    case SS_PIM_READ_SPECIAL4:
    case SS_PIM_READ_SPECIAL_2:
    case SS_PIM_READ_SPECIAL1_6:
    case SS_PIM_READ_SPECIAL1_7:
    case SS_PIM_WRITE_SPECIAL7:
    case SS_PIM_WRITE_SPECIAL6:
      _fu = RdPort;
      break;
    default:
      WARN("unrecognized systemCall %d in %s\n", ntohl(registers[0]), __FILE__);
    }
}
	

