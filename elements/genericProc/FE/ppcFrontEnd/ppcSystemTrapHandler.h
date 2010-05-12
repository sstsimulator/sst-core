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


#ifndef SYSTEMTRAPHANDLER_H
#define SYSTEMTRAPHANDLER_H

  template <int N, class T>
  bool Perform_Str_x(T func, simRegister *, simRegister &nextPC);

  template <int N, class T>
  bool Perform_x(T func, simRegister *, simRegister &nextPC);

  bool Perform_SYS_mach_msg_trap(processor*, simRegister *);
  void do_host_info(processor*, simRegister *);
  void do_clock_get_time(processor*, simRegister *);
  bool Perform_SYS_mach(processor*, simRegister *);

ppcThread* createThreadWithStack(processor* proc, simRegister *regs);

  bool  Perform_SYS_blank(processor*, simRegister *);
  bool	Perform_SYS_exit(processor*, simRegister *);
  bool  Perform_SYS_getrusage(processor*, simRegister *, simRegister &);
  bool  Perform_SYS_getrlimit(processor*, simRegister *, simRegister &);
  bool	Perform_SYS_fork(processor*, simRegister *);
  bool	Perform_SYS_vfork(processor*, simRegister *);
  bool	Perform_SYS_read(processor*, simRegister *, simRegister &);
  bool	Perform_SYS_open(processor*, simRegister *);
  bool	Perform_SYS_write(processor*, simRegister *, simRegister &);
  bool	Perform_SYS_writev(processor*, simRegister *, simRegister &);
  bool	Perform_SYS_close(processor*, simRegister *);
  bool	Perform_SYS_creat(processor*, simRegister *);
  bool	Perform_SYS_unlink(processor*, simRegister *);
  bool	Perform_SYS_chdir(processor*, simRegister *);
  bool	Perform_SYS_chmod(processor*, simRegister *);
  bool	Perform_SYS_chown(processor*, simRegister *);
  bool	Perform_SYS_brk(processor*, simRegister *);
  bool	Perform_SYS_lseek(processor*, simRegister *, simRegister &);
  bool	Perform_SYS_getuid(processor*, simRegister *);
  bool	Perform_SYS_access(processor*, simRegister *);
  bool	Perform_SYS_stat(processor*, simRegister *, simRegister &);
  bool	Perform_SYS_lstat(processor*, simRegister *, simRegister &);
  bool  Perform_SYS_kill(processor*, simRegister *, simRegister &);
  bool	Perform_SYS_dup(processor*, simRegister *);
  bool	Perform_SYS_pipe(processor*, simRegister *);
  bool	Perform_SYS_getgid(processor*, simRegister *);
  bool	Perform_SYS_ioctl(processor*, simRegister *);
  bool	Perform_SYS_fstat(processor*, simRegister *, simRegister &nextPC);
  bool	Perform_SYS_getpagesize(processor*, simRegister *);
  bool	Perform_SYS_fsync(processor*, simRegister *);
  bool	Perform_SYS_dup2(processor*, simRegister *);
  bool	Perform_SYS_fcntl(processor*, simRegister *);
  bool	Perform_SYS_select(processor*, simRegister *);
  bool	Perform_SYS_gettimeofday(processor*, simRegister*,simRegister &nextPC);
  bool	Perform_SYS___sysctl(processor*, simRegister*,simRegister &nextPC);
  bool	Perform_SYS_issetugid(processor* proc, simRegister *regs, simRegister &nextPC);

  bool	Perform_PIM_FORK(processor*, simRegister *);
  bool	Perform_PIM_EXIT(processor*, simRegister *);
  bool	Perform_PIM_EXIT_FREE(processor*, simRegister *);
  bool	Perform_PIM_LOCK(processor*, simRegister *);
  bool	Perform_PIM_UNLOCK(processor*, simRegister *);
  bool	Perform_PIM_IS_LOCAL(processor*, simRegister *);
  bool	Perform_PIM_ALLOCATE_LOCAL(processor*, simRegister *);
  bool	Perform_PIM_MOVE_TO(processor*, simRegister *);
  bool	Perform_PIM_MOVE_AWAY(processor*, simRegister *);
  bool	Perform_PIM_QUICK_PRINT(processor*, simRegister *);
  bool	Perform_PIM_TRACE(processor*, simRegister *);
  bool  Perform_PIM_FFILE_RD(processor*, simRegister *);
  bool	Perform_PIM_RAND(processor*, simRegister *);
  bool	Perform_PIM_MALLOC(processor*, simRegister *);
  bool	Perform_PIM_FREE(processor*, simRegister *);
  bool	Perform_PIM_WRITE_MEM(processor*, simRegister *);
  bool  Perform_PIM_RESET(processor* thread, simRegister *);
  bool  Perform_PIM_NUMBER(processor* thread, simRegister *);
  bool  Perform_PIM_REMAP(processor* thread, simRegister *);
  bool  Perform_PIM_REMAP_TO_ADDR(processor* thread, simRegister *);
  bool  Perform_PIM_ATOMIC_INCREMENT(processor* thread, simRegister *);
  bool  Perform_PIM_ATOMIC_DECREMENT(processor* thread, simRegister *);
  bool  Perform_PIM_EST_STATE_SIZE(processor* thread, simRegister *);
  bool  Perform_PIM_READFX(processor* thread, simRegister *);
  bool  Perform_PIM_WRITEEF(processor* thread, simRegister *);
  bool  Perform_PIM_CHANGE_FE(processor* thread, simRegister *);
bool  Perform_PIM_BULK_set_FE(processor* thread, simRegister *, simRegister);
  bool  Perform_PIM_IS_FE_FULL(processor* thread, simRegister *);
  bool  Perform_PIM_IS_PRIVATE(processor* thread, simRegister *);
  bool  Perform_PIM_TID(processor* thread, simRegister *);
  bool  Perform_PIM_REMAP_TO_POLY(processor* thread, simRegister *);
  bool  Perform_PIM_TAG_INSTRUCTIONS(processor* thread, simRegister *);
  bool  Perform_PIM_TAG_SWITCH(processor* thread, simRegister *);
  bool  Perform_PIM_SPAWN_TO_COPROC(processor*, simRegister *);
  bool  Perform_PIM_SPAWN_TO_LOCALE_STACK(processor*, simRegister *);
  bool  Perform_PIM_SPAWN_TO_LOCALE_STACK_STOPPED(processor*, simRegister *);
  bool  Perform_PIM_START_STOPPED_THREAD(processor*, simRegister *);
  bool  Perform_PIM_SWITCH_ADDR_MODE(processor*, simRegister *);
  bool  Perform_PIM_WRITE_SPECIAL(processor*, simRegister *, const int);
  bool  Perform_PIM_READ_SPECIAL(processor*, simRegister *, const int,
				 const int);
  bool  Perform_PIM_TRYEF(processor*, simRegister *);

  bool  Perform_PIM_MEM_REGION_CREATE(processor* thread, simRegister *);
  bool  Perform_PIM_MEM_REGION_GET(processor* thread, simRegister *);

  bool	Perform_Unimplemented(processor*, simRegister *);

  bool	Perform_NETSIM_pickup(processor*, simRegister *, simRegister &);
  bool	Perform_NETSIM_SYS_CALL(processor*, simRegister *, simRegister &);
  bool	Perform_NETSIM_TX_CALL(processor*, simRegister *, simRegister &);

#endif
