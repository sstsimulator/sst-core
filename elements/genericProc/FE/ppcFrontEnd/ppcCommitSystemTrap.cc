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

#include "sst_config.h"

#include "global.h"
#include "ppcFront.h"
#include "processor.h"
#include "SCall.h"
#include "sim_syscalls_compat.hpp"
#include "sys/sysctl.h"
#include <sst/cpunicEvent.h>
#include <fcntl.h> // KBW: for open() and others
#include <sys/stat.h> // KBW: for fstat()
#include "sst_stdint.h" // for uint64_t

extern	"C"
{
#include "ppc_syscall.h"
#include "pimSysCallDefs.h"
}

//:System call dispatcher for commit()
//
// A large case statement which dispatches a system call to the
// apporpiate sub-function
bool ppcInstruction::commitSystemTrap(processor* proc,
				      uint32 *AtInstruction,
				      simRegister &nextPC)
{
  simRegister *registers = parent->getRegisters();

  int callNum = ntohl(registers[0]);

  if (callNum == 0) {
    // the syscall syscall
    // shift registers
    registers[0] = registers[3];
    for (int r = 3; r <= 9; ++r) {
      registers[r] = registers[r+1];      
    }
    // make the call
    bool ret = commitSystemTrap(proc, AtInstruction, nextPC);
    return ret;
  }

  if (callNum > 0 && callNum < PPC_highest_syscall) {
    // BSD's syscall's advance the PC by 2 instructions
    // mach (<0) only advances one.

    // Note: if a BSD syscall fails, the case statement below should
    // detect that and subtract 4 from nextPC.
    nextPC = htonl(ntohl(nextPC) + 4);
  }

  if (callNum < 0) {
    return Perform_SYS_mach(proc, registers);
  }

  switch (callNum)
    {
    case PPC_SYS_stat:
      return Perform_SYS_stat(proc, registers, nextPC);
      break;
      
    case PPC_SYS_fstat:
      return Perform_SYS_fstat(proc, registers, nextPC);
      break;

    case PPC_SYS_lstat:
      return Perform_SYS_lstat(proc, registers, nextPC);
      break;

    case	PPC_SYS_write:
      return Perform_SYS_write(proc, registers, nextPC);
      break;

    case	PPC_SYS_writev:
      return Perform_SYS_writev(proc, registers, nextPC);
      break;

    case	PPC_SYS_exit:
      return Perform_SYS_exit(proc, registers);
      break;

    case PPC_SYS_getrlimit:
      return Perform_SYS_getrlimit(proc, registers, nextPC);
      break;

    case PPC_SYS_getrusage:
      return Perform_SYS_getrusage(proc, registers, nextPC);
      break;

    case PPC_SYS___sysctl:
      return Perform_SYS___sysctl(proc, registers, nextPC);
      break;

    case	PPC_SYS_ioctl:
      return Perform_SYS_blank(proc, registers);
      break;

    case	PPC_SYS_open:
      {
	char tmpC;
	string tmpS;
	int off = 0;

	do {
	  parent->CopyFromSIM(&tmpC, registers[3]+off, 1);
	  tmpS += ntohl(tmpC);
	  off++;
	} while (tmpC != 0);

	bool ret = Perform_Str_x<2>(open, registers, nextPC);
	INFO("open \"%s\" %x %x-> fd %d\n", 
		tmpS.c_str(), htonl(registers[4]), htonl(registers[5]), htonl(registers[3]));
	return ret;
      }
      break;

    case	PPC_SYS_chdir:
      return Perform_Str_x<0>(chdir, registers, nextPC);
      break;

    case	PPC_SYS_chmod:
      return Perform_Str_x<1>(chmod, registers, nextPC);
      break;

    case	PPC_SYS_chown:
      return Perform_Str_x<2>(chown, registers, nextPC);
      break;

    case	PPC_SYS_access:
      return Perform_Str_x<1>(access, registers, nextPC);
      break;

    case	PPC_SYS_unlink:
      return Perform_Str_x<0>(unlink, registers, nextPC);
      break;

    case	PPC_SYS_close:
      return Perform_x<1>(close, registers, nextPC);
      break;

    case	PPC_SYS_lseek:
      // lseek has a 64-bit argument passed in two registers
      return Perform_SYS_lseek(proc, registers, nextPC);
      break;

    case	PPC_SYS_fcntl:
      return Perform_x<3>(fcntl, registers, nextPC);
      break;

    case	PPC_SYS_getuid:
      return Perform_x<0>(getuid, registers, nextPC);
      break;

    case	PPC_SYS_getgid:
      return Perform_x<0>(getgid, registers, nextPC);
      break;

    case	PPC_SYS_dup2:
      return Perform_x<2>(dup2, registers, nextPC);
      break;

    case	PPC_SYS_dup:
      return Perform_x<1>(dup, registers, nextPC);
      break;

    case	PPC_SYS_fsync:
      return Perform_x<1>(fsync, registers, nextPC);
      break;

    case	PPC_SYS_gettimeofday:
      return Perform_SYS_gettimeofday(proc, registers, nextPC);
      break;

    case	PPC_SYS_read:
      return Perform_SYS_read(proc, registers, nextPC);
      break;
    case PPC_SYS_sigprocmask:
      //WARN("Signals are unsupported, thus, so is sigprocmask.\n");
      break;

    case PPC_SYS_getpid:
      return Perform_SYS_blank(proc, registers);
      break;

    case PPC_SYS_kill:
      return Perform_SYS_kill(proc, registers, nextPC);
      break;

    case PPC_SYS_issetugid:
      return Perform_SYS_issetugid(proc, registers, nextPC);
      break;


    case SS_PIM_READFF:
    case SS_PIM_READFE:
      return(Perform_PIM_READFX(proc, registers));
    case SS_PIM_WRITEEF:
      return(Perform_PIM_WRITEEF(proc, registers));
    case SS_PIM_FILL_FE:
    case SS_PIM_EMPTY_FE:
      return(Perform_PIM_CHANGE_FE(proc, registers));
    case SS_PIM_BULK_EMPTY_FE:
      return(Perform_PIM_BULK_set_FE(proc, registers,0));
    case SS_PIM_BULK_FILL_FE:
      return(Perform_PIM_BULK_set_FE(proc, registers,1));
    case SS_PIM_IS_FE_FULL:
      return(Perform_PIM_IS_FE_FULL(proc, registers));

    case SS_PIM_MEM_REGION_CREATE:
      return(Perform_PIM_MEM_REGION_CREATE(proc, registers));
    case SS_PIM_MEM_REGION_GET:
      return(Perform_PIM_MEM_REGION_GET(proc, registers));
    
    case SS_PIM_TRYEF:
      return(Perform_PIM_TRYEF(proc, registers));

    case SS_PIM_ATOMIC_INCREMENT:
      return(Perform_PIM_ATOMIC_INCREMENT(proc, registers));

    case SS_PIM_ATOMIC_DECREMENT:
      return(Perform_PIM_ATOMIC_DECREMENT(proc, registers));

    case SS_PIM_FORK:
      return(Perform_PIM_FORK(proc, registers));

    case SS_PIM_RESET:
      return(Perform_PIM_RESET(proc, registers));

    case SS_PIM_EXIT:
      return(Perform_PIM_EXIT(proc, registers));
    case SS_PIM_EXIT_FREE:
      return(Perform_PIM_EXIT_FREE(proc, registers));

    case SS_PIM_LOCK:
      return(Perform_PIM_LOCK(proc, registers));
    case SS_PIM_UNLOCK:
      return(Perform_PIM_UNLOCK(proc, registers));

    case SS_PIM_IS_LOCAL:
    case SS_PIM_ALLOCATE_LOCAL:
    case SS_PIM_MOVE_TO:
    case SS_PIM_MOVE_AWAY:
    case SS_PIM_NUMBER:
    case SS_PIM_REMAP:
    case SS_PIM_REMAP_TO_ADDR:
    case SS_PIM_EST_STATE_SIZE:
    case SS_PIM_IS_PRIVATE:
    case SS_PIM_TID:
    case SS_PIM_REMAP_TO_POLY:
    case SS_PIM_TAG_INSTRUCTIONS:
    case SS_PIM_TAG_SWITCH:
      WARN("PIM syscalls not yet supported");
      break;
    case SS_PIM_FFILE_RD:
      return Perform_PIM_FFILE_RD(proc, registers);
      break;
    case SS_PIM_QUICK_PRINT:
      return Perform_PIM_QUICK_PRINT(proc, registers);
      break;
    case SS_PIM_TRACE:
      return Perform_PIM_TRACE(proc, registers);
      break;
    case SS_PIM_RAND:
      return Perform_PIM_RAND(proc, registers);
      break;
    case SS_PIM_MALLOC:
      return Perform_PIM_MALLOC(proc, registers);
      break;
    case SS_PIM_FREE:
      return Perform_PIM_FREE(proc, registers);
      break;
    case SS_PIM_WRITE_MEM:
      return Perform_PIM_WRITE_MEM(proc, registers);
      break;
    case SS_PIM_SPAWN_TO_COPROC:
      return Perform_PIM_SPAWN_TO_COPROC(proc, registers);
      break;
    case SS_PIM_SPAWN_TO_LOCALE_STACK:
      return Perform_PIM_SPAWN_TO_LOCALE_STACK(proc, registers);
      break;
    case SS_PIM_SPAWN_TO_LOCALE_STACK_STOPPED:
      return Perform_PIM_SPAWN_TO_LOCALE_STACK_STOPPED(proc, registers);
      break;
    case SS_PIM_START_STOPPED_THREAD:
      return Perform_PIM_START_STOPPED_THREAD(proc, registers);
      break;
    case SS_PIM_SWITCH_ADDR_MODE:
      return Perform_PIM_SWITCH_ADDR_MODE(proc, registers);
      break;
    case SS_PIM_WRITE_SPECIAL:
      return Perform_PIM_WRITE_SPECIAL(proc, registers, 1);
      break;
    case SS_PIM_WRITE_SPECIAL2:
      return Perform_PIM_WRITE_SPECIAL(proc, registers, 2);
      break;
    case SS_PIM_WRITE_SPECIAL3:
      return Perform_PIM_WRITE_SPECIAL(proc, registers, 3);
      break;
    case SS_PIM_RW_SPECIAL3:
      WARN("ss_pim_rw_special3 not supported for some reason\n");
      return 0;
      break;
    case SS_PIM_READ_SPECIAL:
      return Perform_PIM_READ_SPECIAL(proc, registers, 0, 1);
    case SS_PIM_READ_SPECIAL1:
      return Perform_PIM_READ_SPECIAL(proc, registers, 1, 1);
    case SS_PIM_READ_SPECIAL2:
      return Perform_PIM_READ_SPECIAL(proc, registers, 2, 1);
    case SS_PIM_READ_SPECIAL3:
      return Perform_PIM_READ_SPECIAL(proc, registers, 3, 1);
    case SS_PIM_READ_SPECIAL4:
      return Perform_PIM_READ_SPECIAL(proc, registers, 4, 1);
    case SS_PIM_READ_SPECIAL1_2:
      return Perform_PIM_READ_SPECIAL(proc, registers, 1, 2);
    case SS_PIM_READ_SPECIAL1_5:
      return Perform_PIM_READ_SPECIAL(proc, registers, 1, 5);
    case SS_PIM_WRITE_SPECIAL5:
      return Perform_PIM_WRITE_SPECIAL(proc, registers, 5);
    case SS_PIM_WRITE_SPECIAL4:
      return Perform_PIM_WRITE_SPECIAL(proc, registers, 4);
    case SS_PIM_WRITE_SPECIAL6:
      return Perform_PIM_WRITE_SPECIAL(proc, registers, 6);
    case SS_PIM_WRITE_SPECIAL7:
      return Perform_PIM_WRITE_SPECIAL(proc, registers, 7);
    case SS_PIM_READ_SPECIAL_2:
      return Perform_PIM_READ_SPECIAL(proc, registers, 0, 2);
    case SS_PIM_READ_SPECIAL1_6:
      return Perform_PIM_READ_SPECIAL(proc, registers, 1, 6);
    case SS_PIM_READ_SPECIAL1_7:
      return Perform_PIM_READ_SPECIAL(proc, registers, 1, 7);
    case NETSIM_SYS_ENTER:
      return Perform_NETSIM_SYS_CALL(proc, registers, nextPC);
    case NETSIM_TX_ENTER:
      return Perform_NETSIM_TX_CALL(proc, registers, nextPC);
    case NETSIM_SYS_PICKUP:
      return Perform_NETSIM_pickup(proc, registers, nextPC);
    default:
      {
	WARN("unrecognized/unsupported systemCall %d pc=%p\n", 
	       ntohl((int)registers[0]), (void*)(size_t)PC());
      }
    }

  return true;
}

//: generic function for performing syscalls
//
// used for syscalls which do not require memory copies. The N
// template parameter indicates the number of arguments
template <int N, class T>
bool ppcInstruction::Perform_x(T func, simRegister *regs, 
			       simRegister &nextPC) {

  SCall<T,int,N-1> sc;
  simRegister org3 = ntohl(regs[3]);
  int retV = sc.doIt(func, ntohl(regs[3]), ntohl(regs[4]), ntohl(regs[5]));

  // detect error
  if (retV == -1) {
    WARN("syscall %d(%x, %x, %x)=%x failed. errno=%d\n", (int)regs[0], 
	   org3, (uint)ntohl(regs[4]), (uint)ntohl(regs[5]), 
	   (uint)ntohl(regs[3]), errno); 
    fflush(stdout);
    nextPC = htonl(ntohl(nextPC) - 4);
  } else {   
    regs[3] = htonl(retV);
  }

  return true;
}

//: generic function for performing syscalls
//
// used for syscalls which have the first argument as a string, and
// then 0-2 of additional arguments.
//
// The N template parameter indicates the number of arguments after
// the first string argument.
template <int N, class T>
bool ppcInstruction::Perform_Str_x(T func, simRegister *regs, 
				   simRegister &nextPC) 
{
    char   tmpC;
    string tmpS;
    int    off = 0;

    do {
      parent->CopyFromSIM(&tmpC, ntohl(regs[3])+off, 1);
      tmpS += tmpC;
      off++;
    } while (tmpC != 0);

    SCall<T,const char*,N> sc;
    int retV = sc.doIt(func, tmpS.c_str(), ntohl(regs[4]), ntohl(regs[5]));
   
    // detect failure for BSD calls
    if (retV == -1) {
      WARN("syscall %d() failed. errno=%d\n", (int)regs[0], errno); 
      nextPC = htonl(ntohl(nextPC) - 4);
    } else {
      regs[3] = htonl(retV);
    }

    return true;
}


// timezone is 8 bytes in 32 & 64 bit.
typedef struct timezone timezone_s;


// lseek has a 64-bit argument, and returs a 64-bit number
bool ppcInstruction::Perform_SYS_lseek(processor* proc, 
				       simRegister *regs,
				       simRegister &nextPC) 
{
    // KBW: off_t is NOT 8-bytes on all systems
  uint64_t offset = (((uint64_t)(ntohl(regs[4])))<<32) + ntohl(regs[5]);
  uint64_t res = lseek(ntohl(regs[3]), offset, ntohl(regs[6]));
  
  // detect failure
  if (res == (uint64_t)-1) {
    WARN("lseek(%d %lld %d) failed\n", (int)ntohl(regs[3]), (long long)offset, 
	   (int)ntohl(regs[6]));
    WARN(" ret %lld\n", (long long)res);
    nextPC = htonl(ntohl(nextPC) - 4);
  }

  regs[3] = htonl(res >> 32);
  regs[4] = htonl(res & 0xffffffff);

  return true;
}


bool ppcInstruction::Perform_SYS_gettimeofday(processor* proc, 
					      simRegister *regs,
					      simRegister &nextPC) 
{
    timeval    tp;
    timeval32  tp32;
    int retV=0;

    if (ppcThread::realGettimeofday) {
	timezone_s tzp;
	retV = gettimeofday(&tp, &tzp);
    } else {
	tp.tv_sec = 0;
	tp.tv_usec = parent->home->getCurrentSimTimeMicro();
	if (tp.tv_usec > 1e6) {
	    tp.tv_sec = tp.tv_usec * 1e-6;
	    tp.tv_usec -= (tp.tv_sec * 1e6);
	}
    }
    timevalToTimeval32(&tp, &tp32);

    INFO("gettimeofday: sec(%d,%d), usec(%d,%d)\n", (int)tp.tv_sec, (int)tp32.tv_sec, (int)tp.tv_usec,(int)tp32.tv_usec);

    tp32.tv_sec  = htonl(tp32.tv_sec);
    tp32.tv_usec = htonl(tp32.tv_usec);

    const simAddress tp_addr = ntohl(regs[3]);
    if ( tp_addr ) {
	parent->CopyToSIM(tp_addr, &tp32, sizeof(tp32));
    }

    regs[3] = htonl(retV);

    // detect error
    if (regs[3] == htonl(-1)) {
	nextPC = htonl(ntohl(nextPC) - 4);
    }

    return true;
}

static inline uint64_t hton64(uint64_t in)
{
#ifndef WORDS_BIGENDIAN
    uint32_t high = in >> 32;
    uint32_t low = in & 0xffffffff;
    return (((uint64_t)htonl(low)) << 32) | htonl(high);
#else
    return in;
#endif
}

typedef struct stat stat_s;

void
htonl_stat32_t(struct stat *ss_sbuf, void *target)
{
#ifndef __x86_64__
    stat32_s *ss_sbuf32= (stat32_s *)target;

    ss_sbuf32->st_dev = htonl(ss_sbuf->st_dev);
    ss_sbuf32->st_ino = htonl(ss_sbuf->st_ino);
    ss_sbuf32->st_mode = htons(ss_sbuf->st_mode);
    ss_sbuf32->st_nlink = htons(ss_sbuf->st_nlink);
    ss_sbuf32->st_uid = htonl(ss_sbuf->st_uid);
    ss_sbuf32->st_gid = htonl(ss_sbuf->st_gid);
    ss_sbuf32->st_rdev = htonl(ss_sbuf->st_rdev );
    ss_sbuf32->st_size = hton64(ss_sbuf->st_size);
    ss_sbuf32->st_blocks = hton64(ss_sbuf->st_blocks);
    ss_sbuf32->st_blksize = htonl(ss_sbuf->st_blksize);
    ss_sbuf32->st_flags = 0;
    ss_sbuf32->st_gen = 0;
#ifdef HAVE_STAT_ST_ATIMESPEC
    ss_sbuf32->st_atimespec.tv_sec = htonl(ss_sbuf->st_atime);
    ss_sbuf32->st_atimespec.tv_nsec = 0;
#endif
#ifdef HAVE_STAT_ST_MTIMESPEC
    ss_sbuf32->st_mtimespec.tv_sec = htonl(ss_sbuf->st_mtime);
    ss_sbuf32->st_mtimespec.tv_nsec = 0;
#endif
#ifdef HAVE_STAT_ST_CTIMESPEC
    ss_sbuf32->st_ctimespec.tv_sec = htonl(ss_sbuf->st_ctime);
    ss_sbuf32->st_ctimespec.tv_nsec = 0;
#endif

#else // 64-bit X86 system
    // Copy the data in case we're simulating a 32-bit system
    // on a 64-bit system, and then do endian conversion
stat_ppc_32_t *ss_sbuf32= (stat_ppc_32_t *)target;
    statToStat32(ss_sbuf, ss_sbuf32);
    ss_sbuf32->st_dev = htonl(ss_sbuf32->st_dev);
    ss_sbuf32->st_ino = htonl(ss_sbuf32->st_ino);
    ss_sbuf32->st_mode = htons(ss_sbuf32->st_mode);
    ss_sbuf32->st_nlink = htons(ss_sbuf32->st_nlink);
    ss_sbuf32->st_uid = htonl(ss_sbuf32->st_uid);
    ss_sbuf32->st_gid = htonl(ss_sbuf32->st_gid);
    ss_sbuf32->st_rdev = htonl(ss_sbuf32->st_rdev );
    ss_sbuf32->st_atimespec.tv_sec = htonl(ss_sbuf32->st_atimespec.tv_sec);
    ss_sbuf32->st_atimespec.tv_nsec = htonl(ss_sbuf32->st_atimespec.tv_nsec);
    ss_sbuf32->st_mtimespec.tv_sec = htonl(ss_sbuf32->st_mtimespec.tv_sec);
    ss_sbuf32->st_mtimespec.tv_nsec = htonl(ss_sbuf32->st_mtimespec.tv_nsec);
    ss_sbuf32->st_ctimespec.tv_sec = htonl(ss_sbuf32->st_ctimespec.tv_sec);
    ss_sbuf32->st_ctimespec.tv_nsec = htonl(ss_sbuf32->st_ctimespec.tv_nsec);
    ss_sbuf32->st_size = hton64(ss_sbuf32->st_size);
    ss_sbuf32->st_blocks = hton64(ss_sbuf32->st_blocks);
    ss_sbuf32->st_blksize = htonl(ss_sbuf32->st_blksize);
    ss_sbuf32->st_flags = htonl(ss_sbuf32->st_flags);
    ss_sbuf32->st_gen = htonl(ss_sbuf32->st_gen);
#endif
}  // end of ntohl_stat32()



bool ppcInstruction::Perform_SYS_fstat(processor* proc, simRegister *regs,
				       simRegister &nextPC) 
{
    struct stat ss_sbuf;
#ifdef __x86_64__
    stat_ppc_32_t ss_sbuf32;	// The simulated machine expects this
#else
    stat32_s    ss_sbuf32;    /* wcm: for 32-bit compat */
#endif

    int retV = fstat(ntohl(regs[3]), &ss_sbuf);

    htonl_stat32_t(&ss_sbuf, &ss_sbuf32);
    regs[3] = htonl(retV);

    // detect error
    if (ntohl(regs[3]) == -1) {
	nextPC = htonl(ntohl(nextPC) - 4);
    } else {
	parent->CopyToSIM(ntohl(regs[4]), &ss_sbuf32, sizeof(ss_sbuf32));
    }

    return true;
}


bool ppcInstruction::Perform_SYS_lstat(processor*   proc, 
	                              simRegister* regs,
				      simRegister& nextPC) 
{
    stat_s   ss_sbuf;
#ifdef __x86_64__
    stat_ppc_32_t ss_sbuf32;	// The simulated machine expects this
#else
    stat32_s    ss_sbuf32;    /* wcm: for 32-bit compat */
#endif
    
    char   tmpC;
    string tmpS;
    int    off = 0;

    do {
	parent->CopyFromSIM(&tmpC, ntohl(regs[3])+off, 1);
	tmpS += ntohl(tmpC);
	off++;
    } while (tmpC != 0);

    fprintf(stderr, "--- lstat path is \"%s\"\n", tmpS.c_str());
    int retV = lstat(tmpS.c_str(), &ss_sbuf);
    htonl_stat32_t(&ss_sbuf, &ss_sbuf32);
    regs[3] = htonl(retV);

    // detect error
    if (regs[3] == htonl(-1)) {
	nextPC = htonl(ntohl(nextPC) - 4);
    } else {
	parent->CopyToSIM(ntohl(regs[4]), &ss_sbuf32, sizeof(ss_sbuf32));
    }

    return true;
}


bool ppcInstruction::Perform_SYS_stat(processor*   proc, 
	                              simRegister* regs,
				      simRegister& nextPC) 
{
    stat_s   ss_sbuf;
#ifdef __x86_64__
    stat_ppc_32_t ss_sbuf32;	// The simulated machine expects this
#else
    stat32_s    ss_sbuf32;    /* wcm: for 32-bit compat */
#endif
    
    char   tmpC;
    string tmpS;
    int    off = 0;

    do {
	parent->CopyFromSIM(&tmpC, ntohl(regs[3])+off, 1);
	tmpS += htonl(tmpC);
	off++;
    } while (tmpC != 0);

    fprintf(stderr, "--- stat path is \"%s\"\n", tmpS.c_str());
    int retV = stat(tmpS.c_str(), &ss_sbuf);
    htonl_stat32_t(&ss_sbuf, &ss_sbuf32);

    regs[3] = htonl(retV);

    // detect error
    if (regs[3] == htonl(-1)) {
	nextPC = htonl(ntohl(nextPC) - 4);
    } else {
	parent->CopyToSIM(ntohl(regs[4]), &ss_sbuf32, sizeof(ss_sbuf32));
    }

    return true;
}


//: fake syscall
//
// place holder for any syscalls we don't do. returns a 0
bool ppcInstruction::Perform_SYS_blank(processor* proc, simRegister *regs) {
  regs[3] = 0;
  return true;
}


bool ppcInstruction::Perform_SYS_kill(processor* proc, simRegister *regs,
				      simRegister &nextPC)
{
    if (regs[3] == 0) { /* this is a special case: the thread is killing itself */
	parent->_isDead = 1;
	WARN("*** Thread Committed Seppuku ***\n");
	fflush(stdout); fflush(stderr);
	//component::keepAlive() = 0;
	return true;
    } else {
      WARN("Killing other threads is currently "
	      "unsupported (tried to kill %i).\n",(int)ntohl(regs[3]));
	regs[3] = htonl(-1);
	nextPC = htonl(ntohl(nextPC) - 4);
	return true;
    }
}


bool ppcInstruction::Perform_SYS_read(processor   *proc, 
	                              simRegister *regs,
				      simRegister &nextPC) 
{
    char *TemporaryBuffer;

    if ( !(TemporaryBuffer = new char[ntohl(regs[5])]) ) 
    {
	WARN("out of memory in SYS_read\n");
	regs[3] = htonl(-1);
	return ( true );
    }

    //INFO("read fd %ld\n", regs[3]);

    int retV = read(ntohl(regs[3]), TemporaryBuffer, ntohl(regs[5]));
    parent->CopyToSIM(ntohl(regs[4]), TemporaryBuffer, ntohl(regs[5]));

    regs[3] = htonl(retV);

    // detect error
    if (regs[3] == htonl(-1)) {
	nextPC = htonl(ntohl(nextPC) - 4);
    }

    return true;
}


/* regs[3] => file descriptor to write to
 * regs[4] => buffer to write
 * regs[5] => how many bytes in that buffer */
bool ppcInstruction::Perform_SYS_write(processor   *proc, 
				       simRegister *regs,
				       simRegister &nextPC) 
{
  char* TemporaryBuffer = NULL;
  const int fd = ntohl(regs[3]);
  const simAddress userBuffer = ntohl(regs[4]);
  const size_t numBytes = ntohl(regs[5]);

  if(numBytes < 0) {
    WARN("Invalid, asking to write %lu bytes\n", (unsigned long)numBytes);
    regs[3] = htonl((uint32_t)-1);
    return true;
  }

  if (!(TemporaryBuffer = new char[numBytes])) {
    WARN("out of memory in SYS_write\n");
    regs[3] = htonl((uint32_t)-1);
    return ( true );
  }

  fflush(stdout);   
  fflush(stderr);

  parent->CopyFromSIM(TemporaryBuffer, userBuffer, numBytes);
  if (fd == 2) {
    // move stderr to stdout
    regs[3] = htonl(write(1, TemporaryBuffer, numBytes));
  } else {
    regs[3] = htonl(write(fd, TemporaryBuffer, numBytes));
  }

#if 0
  if (strncmp(TemporaryBuffer, " sp 2", 5) == 0) {
    INFO("found MAK\n");
    component::keepAlive() = -1;
  }
#endif

  if (size_t(ntohl(regs[3])) != numBytes) {
    WARN("write err 1\n");
    regs[3] = htonl((uint32_t)-1);
  } else if (ntohl(regs[3]) == -1) {
    WARN("write err 2\n");
    nextPC = htonl(ntohl(nextPC) - 4);
  }

  delete TemporaryBuffer;
  return true;
}


bool ppcInstruction::Perform_SYS_writev(processor   *proc, 
	                                simRegister *regs,
					simRegister &nextPC)
{
    char *TemporaryBuffer;

    if (ntohl(regs[5]) < 0) {
      WARN("Invalid, asking to write %d iovec struct\n", ntohl((int)regs[5]));
	regs[3] = htonl(-1);
	return true;
    }

    for (int i = 0; i < ntohl(regs[5]); i++) 
    {	
	struct myiovec {
	    simAddress iov_base;
	    uint32_t iov_len;	    // 64b: was size_t, incompat w/ 64-bit
	} iov;

	/* fetch the iovec struct from the program */
	parent->CopyFromSIM((void*)&iov, 
		            ntohl(regs[4])+(sizeof(struct myiovec)*i), 
			    sizeof(struct myiovec) );
	
	/* allocate space to copy the vector to be written from the program */
	if (!(TemporaryBuffer = new char[ntohl(iov.iov_len)])) {
	    WARN("out of memory in SYS_writev\n");
	    regs[3] = htonl(-1);
	    return true;
	}
	fflush(stdout); 
	fflush(stderr);
	
	/* fetch the buffer to be written from the program */
	parent->CopyFromSIM((void*)TemporaryBuffer, 
		            ntohl(iov.iov_base), 
			    ntohl(iov.iov_len));
	int retV, die=0;
	if (regs[3] == htonl(2)) { /* i.e. it should be written to stderr */
	    retV = write(1, TemporaryBuffer, ntohl(iov.iov_len));
	} else {
	    retV = write(ntohl(regs[3]), TemporaryBuffer, ntohl(iov.iov_len));
	}
	
	if ((unsigned int)retV != ntohl(iov.iov_len)) { /* short write */
	    WARN("write err 1\n");
	    regs[3] = htonl(-1);
	    die = 1;
	}
	
	if (retV == -1) { /* write error */
	    WARN("write err 2\n");
	    nextPC = htonl(ntohl(nextPC) - 4);
	    die = 1;
	}

	delete TemporaryBuffer;
	if (die == 1) 
	    return true;
    }
    return true;
}


#include "fe_debug.h"
bool ppcInstruction::Perform_SYS_exit(processor* proc, simRegister *regs) 
{
    parent->_isDead = 1;
//     INFO("SysExit Called @ %f\n", proc->CurrentTime()*1000000.0);
    fflush(stdout);   
    fflush(stderr);
    proc->procExit();
#if 0
    if (ppcThread::exitSysCallExitsAll) {
      //component::keepAlive() = 0;
    } else if ( parent->ShouldExit ) {
      //--component::keepAlive();
    }
#endif
    return ( true );
}


bool ppcInstruction::Perform_SYS_getrusage(processor* proc, 
					   simRegister *regs,
					   simRegister &nextPC) {
    //rusage   ruse;
    //rusage32 ruse32;	// system-based rusage struct differs in 32/64bit 

    /* realGetrusage */
#if 0
    int ret = getrusage(regs[3], &ruse);
    rusageToRusage32(&ruse, &ruse32);
#endif
    if (regs[3] != htonl(RUSAGE_SELF)) {
	regs[3] = htonl(-1);
    } else {
#if 0
	// wcm: long long are ok in 32/64bit, each are 8 bytes.
	unsigned long long ts = proc->TimeStamp(); 
	memset(&ruse32, 0, sizeof(ruse32));
	ruse32.ru_utime.tv_sec  = int32((ts / proc->ClockMhz()) / 1000000);
	ruse32.ru_utime.tv_usec = int32((ts / proc->ClockMhz()) % 1000000);
	/* XXX: technically, this is probably bad, but because there's no OS,
	 * there's no system, and thus no system time */
	ruse32.ru_stime.tv_sec  = 0;
	ruse32.ru_stime.tv_usec = 1;

	parent->CopyToSIM(regs[4], &ruse32, sizeof(ruse32));
#endif
	regs[3] = 0;
    }

    // detect error
    if (regs[3] == htonl(-1)) {
	nextPC = htonl(ntohl(nextPC) - 4); /* BSD-ism; we're backtracking 1 word */
    }

    return true;
}


// regs[3] => resource
// regs[4] => pointer to rlimit struct
bool
ppcInstruction::Perform_SYS_getrlimit(processor* proc, 
                                      simRegister *regs,
                                      simRegister &nextPC)
{
    const int resource = ntohl(regs[3]);
    struct rlimit rl;

    switch (resource) {
	case RLIMIT_STACK:
	    regs[3] = 0; // no need for byte-swapping
	    rl.rlim_cur = hton64(0x800000);
	    rl.rlim_max = hton64(0x3fff000); // just copying ppc behavior
	    break;
	default:
	    regs[3] = htonl(getrlimit(resource, &rl));
	    INFO("%i = getrlimit(%i,0x%x) => { %x, %x}\n", ntohl(regs[3]),
		    resource, ntohl(regs[4]), (unsigned int)rl.rlim_cur,
		    (unsigned int)rl.rlim_max);
	    rl.rlim_cur = hton64(rl.rlim_cur);
	    rl.rlim_max = hton64(rl.rlim_max);
	    if (regs[3] == htonl(-1)) regs[3] = htonl(errno);
	    break;
    }

    assert(sizeof(rlim_t) == sizeof(uint64_t));
    // Copy the rlimit struct to application memory.  rlimit is two
    // 64 bit fields, so should be no 32/64 problems.
    parent->CopyToSIM(ntohl(regs[4]), &rl.rlim_cur, sizeof(uint64_t));
    parent->CopyToSIM(ntohl(regs[4])+sizeof(uint64_t), &rl.rlim_max, sizeof(uint64_t));

    // detect error
    if (regs[3] != 0) {
	nextPC = htonl(ntohl(nextPC) - 4); /* BSD-ism; we're backtracking 1 word */
    }

    return true;
}

bool ppcInstruction::Perform_SYS_issetugid(processor* proc, simRegister *regs, simRegister &nextPC)
{
    // FIXME: For now we are just faking this.
    regs[3] = 0;
    return true;
}


void
print_regs(simRegister *regs)
{

int i, j;

    for (i= 0; i < 8; i++)   {
	for (j= 0; j < 4; j++)   {
	    INFO("r[%2d] %12d   ", i * 4 + j, ntohl(regs[i*4+j]));
	}
	INFO("\n");
    }
}

//
// If the CPU has received an event from the NIC, then this
// call picks it up and returns it to user level
// We hard-code the maximum size of the parameters structure so we
// don't have to include user_includes/netsim/netsim_internal.h
#define MAX_NICPARAMS_SIZE	(64)
bool ppcInstruction::Perform_NETSIM_pickup(processor* proc, simRegister *regs, simRegister &nextPC)
{
    bool rc;
    const simAddress params = ntohl(regs[4]);
    int params_length = ntohl(regs[5]);
    char data[MAX_NICPARAMS_SIZE];
    uint32_t *user_rc= (uint32_t *)(data);
    int payload_len;
    simAddress user_buf;
    CPUNicEvent *e;


    INFO("--------------------------------------------------------- Perform_NETSIM_pickup\n");
    // INFO("Registers passed to us:\n");
    // print_regs(regs);


    if (params_length > MAX_NICPARAMS_SIZE)   {
	WARN("Params_length too large! %d > %d\n", params_length, MAX_NICPARAMS_SIZE);
	// Clear the user level rc in parameters (to indicate nothing returned)
	*user_rc= ntohl(0);
	parent->CopyToSIM(params, data, sizeof(int));
	regs[3] = htonl((uint32_t)-1);
	nextPC = htonl(ntohl(nextPC) - 4);
	return true;
    }

    rc= proc->pickupNetsimNIC(&e);

    if (rc == true)    {
	e->DetachParams(static_cast<void *>(data), &params_length);
	INFO("--------------------------------------------------------- Unpacked %d bytes of params\n", params_length);

	// copy params to user space
	parent->CopyToSIM(params, data, params_length);

	// Is there payload data?
	payload_len= e->GetPayloadLen();
	if (payload_len)   {
	    char *tmp_buf;
	    uint64_t lower32, upper32;

	    tmp_buf= new char[payload_len];
	    if (!tmp_buf)   {
		WARN("Out of memory!\n");
		regs[3] = htonl((uint32_t)-1);
		nextPC = htonl(ntohl(nextPC) - 4);
		delete tmp_buf;
		delete e;
		return true;
	    }

	    e->DetachPayload(tmp_buf, &payload_len);
	    INFO("--------------------------------------------------------- Unpacked %d bytes of payload data\n", payload_len);
	    // fprintf(stderr, "%s() payload is \"%s\"\n", __func__, tmp_buf);
	    lower32= htonl(e->buf >> 32);
	    upper32= htonl(e->buf);
	    user_buf= (upper32 << 32) | lower32;

	    // fprintf(stderr, "%s() e->buf is %p, user_buf is %p\n", __func__, e->buf, user_buf);
	    parent->CopyToSIM(user_buf, tmp_buf, payload_len);
	    delete tmp_buf;
	}


	regs[3]= ntohl(0);
	delete e;
    } else   {
	// Clear the user level rc in parameters (to indicate nothing returned)
	*user_rc= ntohl(0);
	parent->CopyToSIM(params, data, sizeof(int));
	regs[3] = htonl((uint32_t)-1);
	nextPC = htonl(ntohl(nextPC) - 4);
    }

    return true;
}

bool ppcInstruction::Perform_NETSIM_SYS_CALL(processor* proc, simRegister *regs, simRegister &nextPC)
{
    bool rc;
    int call_num= ntohl(regs[3]);
    const simAddress params = ntohl(regs[4]);
    const size_t params_length = ntohl(regs[5]);
    char *tmp;
    uint32_t *user_rc;


    INFO("--------------------------------------------------------- Perform_NETSIM_SYS_CALL, call %d\n", call_num);
    // INFO("Registers passed to us:\n");
    // print_regs(regs);


    tmp= new char[params_length];
    if (!tmp)   {
	WARN("Out of memory!\n");
	regs[3] = htonl((uint32_t)-1);
	nextPC = htonl(ntohl(nextPC) - 4);
	return true;
    }
    user_rc= (uint32_t *)(tmp);

    parent->CopyFromSIM(tmp, params, params_length);
    rc= proc->forwardToNetsimNIC(call_num, tmp, params_length, NULL, 0);

    if (rc == false)    {
	*user_rc= ntohl(1);
	parent->CopyToSIM(params, tmp, sizeof(int));
	regs[3]= htonl(0);
    } else   {
	*user_rc= ntohl(0);
	parent->CopyToSIM(params, tmp, sizeof(int));
	regs[3] = htonl((uint32_t)-1);
	nextPC = htonl(ntohl(nextPC) - 4);
    }

    delete tmp;
    return true;
}


bool ppcInstruction::Perform_NETSIM_TX_CALL(processor* proc, simRegister *regs, simRegister &nextPC)
{
    bool rc;
    int call_num= ntohl(regs[3]);
    const simAddress params = ntohl(regs[4]);
    const size_t params_length = ntohl(regs[5]);
    char *tmp;
    uint32_t *user_rc;


    INFO("--------------------------------------------------------- Perform_NETSIM_TX_CALL, call %d\n", call_num);
    // INFO("Registers passed to us:\n");
    // print_regs(regs);


    tmp= new char[params_length];
    if (!tmp)   {
	WARN("Out of memory!\n");
	regs[3] = htonl((uint32_t)-1);
	nextPC = htonl(ntohl(nextPC) - 4);
	return true;
    }
    user_rc= (uint32_t *)(tmp);

    parent->CopyFromSIM(tmp, params, params_length);


    // This is a send, so we also have to copy the buffer
    const simAddress buf = ntohl(regs[6]);
    const size_t buf_length = ntohl(regs[7]);
    char *tmp_buf;

    tmp_buf= new char[buf_length];
    if (!tmp_buf)   {
	WARN("Out of memory!\n");
	regs[3] = htonl((uint32_t)-1);
	nextPC = htonl(ntohl(nextPC) - 4);
	return true;
    }

    parent->CopyFromSIM(tmp_buf, buf, buf_length);


    // Now send it to the local NIC
    rc= proc->forwardToNetsimNIC(call_num, tmp, params_length, tmp_buf, buf_length);

    if (rc == false)    {
	*user_rc= ntohl(1);
	parent->CopyToSIM(params, tmp, sizeof(int));
	regs[3]= htonl(0);
    } else   {
	*user_rc= ntohl(0);
	parent->CopyToSIM(params, tmp, sizeof(int));
	regs[3] = htonl((uint32_t)-1);
	nextPC = htonl(ntohl(nextPC) - 4);
    }

    delete tmp;
    delete tmp_buf;
    return true;
}


#include <sys/sysctl.h>

// regs[3] => name
// regs[5] => namelen
// regs[5] => oldp
// regs[6] => oldlenp
// regs[7] => newp
// regs[8] => newlen
bool
ppcInstruction::Perform_SYS___sysctl(processor* proc, 
                                     simRegister *regs,
                                     simRegister &nextPC)
{
    const simAddress user_namep = ntohl(regs[3]);
    unsigned int namelen = ntohl(regs[4]);
    simAddress user_oldp = ntohl(regs[5]);
    simAddress user_oldlenp = ntohl(regs[6]);
    const simAddress user_newp = ntohl(regs[7]);
    size_t oldlen, orig_oldlen, newlen = ntohl(regs[8]);
    int32_t tmp;
    void *oldp, *newp;
    int *mib;
    char str[] = "Simulator";
    
    mib = new int[namelen];
    if (NULL == mib) {
        regs[3] = htonl(ENOMEM);
        goto cleanup;
    }
    parent->CopyFromSIM(mib, user_namep, sizeof(int) * namelen);
    for (unsigned int i=0;i<namelen; i++) {
	mib[i] = ntohl(mib[i]);
    }

    if (0 != user_oldlenp) {
        parent->CopyFromSIM(&tmp, user_oldlenp, sizeof(int32_t));
        orig_oldlen = oldlen = ntohl(tmp);
    } else {
        orig_oldlen = oldlen = 0;
    }

    switch (mib[0]) {
    case CTL_HW:
        switch (mib[1]) {
        case HW_MACHINE:
        case HW_MODEL:
        case HW_MACHINE_ARCH:
            if (0 == user_oldp) {
                if (strlen(str) > oldlen) {
                    regs[3] = htonl(ENOMEM);
                    goto cleanup;
                } else {
                    parent->CopyToSIM(user_oldp, str, strlen(str) + 1);
                    tmp = strlen(str) + 1;
                    parent->CopyToSIM(user_oldlenp, &tmp, sizeof(int32_t));
                }
            } else {
                tmp = strlen(str) + 1;
                parent->CopyToSIM(user_oldlenp, &tmp, sizeof(int32_t));
            }
            break;
        case HW_NCPU:
            /* lie */
            tmp = htonl(1);
            parent->CopyToSIM(user_oldp, &tmp, sizeof(int32_t));
            tmp = htonl(sizeof(int32_t));
            parent->CopyToSIM(user_oldlenp, &tmp, sizeof(int32_t));
            break;
        case HW_BYTEORDER:
            tmp = htonl(4321);
            parent->CopyToSIM(user_oldp, &tmp, sizeof(int32_t));
            tmp = htonl(sizeof(int32_t));
            parent->CopyToSIM(user_oldlenp, &tmp, sizeof(int32_t));
            break;
        case HW_PHYSMEM:
            /* lie */
            tmp = htonl(1 * 1024 * 1024 * 1024);
            parent->CopyToSIM(user_oldp, &tmp, sizeof(int32_t));
            tmp = htonl(sizeof(int32_t));
            parent->CopyToSIM(user_oldlenp, &tmp, sizeof(int32_t));
            break;
        case HW_USERMEM:
            /* lie */
            tmp = htonl(1 * 1024 * 1024 * 1024);
            parent->CopyToSIM(user_oldp, &tmp, sizeof(int32_t));
            tmp = htonl(sizeof(int32_t));
            parent->CopyToSIM(user_oldlenp, &tmp, sizeof(int32_t));
            break;
        case HW_PAGESIZE:
            /* lie */
            tmp = htonl(4 * 1024);
            parent->CopyToSIM(user_oldp, &tmp, sizeof(int32_t));
            tmp = htonl(sizeof(int32_t));
            parent->CopyToSIM(user_oldlenp, &tmp, sizeof(int32_t));
            break;
        }
        regs[3] = 0;
        break;
    case CTL_MACHDEP:
    case CTL_NET:
    case CTL_VM:
        regs[3] = htonl(ENOSYS);
        goto cleanup;
        break;
    default:
        if (0 != user_oldp) {
            oldp = new char[oldlen];
            if (NULL == oldp) {
                regs[3] = htonl(ENOMEM);
                goto cleanup;
            }
        } else {
            oldp = NULL;
        }

        if (0 != newlen) {
            newp = new char[newlen];
            if (NULL == newp) {
                regs[3] = htonl(ENOMEM);
                goto cleanup;
            }

            parent->CopyFromSIM(newp, user_newp, newlen);
        } else {
            newp = NULL;
        }

        regs[3] = htonl(sysctl(mib, namelen, oldp, &oldlen, newp, newlen));
        if (0 != regs[3]) {
            regs[3] = htonl(errno);
            goto cleanup;
        }

        if (oldp != NULL) {
            parent->CopyToSIM(user_oldp, oldp, orig_oldlen);
        }
        tmp = oldlen;
        parent->CopyToSIM(user_oldlenp, &tmp, sizeof(int32_t));
    }

 cleanup:
    // detect error
    if (regs[3] != 0) {
	nextPC = htonl(ntohl(nextPC) - 4); /* BSD-ism; we're backtracking 1 word */
    }

    return true;
}

// EOF
