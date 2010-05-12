/*
 * Copyright (c) 2000-2003 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1992, 1995-1999 Apple Computer, Inc. All Rights Reserved */
/*
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 */

#define	PPC_SYS_syscall	0
#define	PPC_SYS_exit	1
#define	PPC_SYS_fork	2
#define	PPC_SYS_read	3
#define	PPC_SYS_write	4
#define	PPC_SYS_open	5
#define	PPC_SYS_close	6
#define	PPC_SYS_wait4	7
				/* 8 is old creat */
#define	PPC_SYS_link	9
#define	PPC_SYS_unlink	10
				/* 11 is obsolete execv */
#define	PPC_SYS_chdir	12
#define	PPC_SYS_fchdir	13
#define	PPC_SYS_mknod	14
#define	PPC_SYS_chmod	15
#define	PPC_SYS_chown	16
				/* 17 is obsolete sbreak */
#if COMPAT_GETFSSTAT
				/* 18 is old getfsstat */
#else
#define	PPC_SYS_getfsstat	18
#endif
				/* 19 is old lseek */
#define	PPC_SYS_getpid	20
				/* 21 is obsolete mount */
				/* 22 is obsolete umount */
#define	PPC_SYS_setuid	23
#define	PPC_SYS_getuid	24
#define	PPC_SYS_geteuid	25
#define	PPC_SYS_ptrace	26
#define	PPC_SYS_recvmsg	27
#define	PPC_SYS_sendmsg	28
#define	PPC_SYS_recvfrom	29
#define	PPC_SYS_accept	30
#define	PPC_SYS_getpeername	31
#define	PPC_SYS_getsockname	32
#define	PPC_SYS_access	33
#define	PPC_SYS_chflags	34
#define	PPC_SYS_fchflags	35
#define	PPC_SYS_sync	36
#define	PPC_SYS_kill	37
				/* 38 is old stat */
#define	PPC_SYS_getppid	39
				/* 40 is old lstat */
#define	PPC_SYS_dup	41
#define	PPC_SYS_pipe	42
#define	PPC_SYS_getegid	43
#define	PPC_SYS_profil	44
#define	PPC_SYS_ktrace	45
#define	PPC_SYS_sigaction	46
#define	PPC_SYS_getgid	47
#define	PPC_SYS_sigprocmask	48
#define	PPC_SYS_getlogin	49
#define	PPC_SYS_setlogin	50
#define	PPC_SYS_acct	51
#define	PPC_SYS_sigpending	52
#define	PPC_SYS_sigaltstack	53
#define	PPC_SYS_ioctl	54
#define	PPC_SYS_reboot	55
#define	PPC_SYS_revoke	56
#define	PPC_SYS_symlink	57
#define	PPC_SYS_readlink	58
#define	PPC_SYS_execve	59
#define	PPC_SYS_umask	60
#define	PPC_SYS_chroot	61
				/* 62 is old fstat */
				/* 63 is unused */
				/* 64 is old getpagesize */
#define	PPC_SYS_msync	65
#define	PPC_SYS_vfork	66
				/* 67 is obsolete vread */
				/* 68 is obsolete vwrite */
#define	PPC_SYS_sbrk	69
#define	PPC_SYS_sstk	70
				/* 71 is old mmap */
				/* 72 is obsolete vadvise */
#define	PPC_SYS_munmap	73
#define	PPC_SYS_mprotect	74
#define	PPC_SYS_madvise	75
				/* 76 is obsolete vhangup */
				/* 77 is obsolete vlimit */
#define	PPC_SYS_mincore	78
#define	PPC_SYS_getgroups	79
#define	PPC_SYS_setgroups	80
#define	PPC_SYS_getpgrp	81
#define	PPC_SYS_setpgid	82
#define	PPC_SYS_setitimer	83
				/* 84 is old wait */
#define	PPC_SYS_swapon	85
#define	PPC_SYS_getitimer	86
				/* 87 is old gethostname */
				/* 88 is old sethostname */
#define PPC_SYS_getdtablesize 89
#define	PPC_SYS_dup2	90
#define	PPC_SYS_fcntl	92
#define	PPC_SYS_select	93
				/* 94 is obsolete setdopt */
#define	PPC_SYS_fsync	95
#define	PPC_SYS_setpriority	96
#define	PPC_SYS_socket	97
#define	PPC_SYS_connect	98
				/* 99 is old accept */
#define	PPC_SYS_getpriority	100
				/* 101 is old send */
				/* 102 is old recv */
#ifndef __ppc__
#define	PPC_SYS_sigreturn	103
#endif
#define	PPC_SYS_bind	104
#define	PPC_SYS_setsockopt	105
#define	PPC_SYS_listen	106
				/* 107 is obsolete vtimes */
				/* 108 is old sigvec */
				/* 109 is old sigblock */
				/* 110 is old sigsetmask */
#define	PPC_SYS_sigsuspend	111
				/* 112 is old sigstack */
				/* 113 is old recvmsg */
				/* 114 is old sendmsg */
				/* 115 is obsolete vtrace */
#define	PPC_SYS_gettimeofday	116
#define	PPC_SYS_getrusage	117
#define	PPC_SYS_getsockopt	118
				/* 119 is obsolete resuba */
#define	PPC_SYS_readv	120
#define	PPC_SYS_writev	121
#define	PPC_SYS_settimeofday	122
#define	PPC_SYS_fchown	123
#define	PPC_SYS_fchmod	124
				/* 125 is old recvfrom */
				/* 126 is old setreuid */
				/* 127 is old setregid */
#define	PPC_SYS_rename	128
				/* 129 is old truncate */
				/* 130 is old ftruncate */
#define	PPC_SYS_flock	131
#define	PPC_SYS_mkfifo	132
#define	PPC_SYS_sendto	133
#define	PPC_SYS_shutdown	134
#define	PPC_SYS_socketpair	135
#define	PPC_SYS_mkdir	136
#define	PPC_SYS_rmdir	137
#define	PPC_SYS_utimes	138
#define	PPC_SYS_futimes	139
#define	PPC_SYS_adjtime	140
				/* 141 is old getpeername */
				/* 142 is old gethostid */
				/* 143 is old sethostid */
				/* 144 is old getrlimit */
				/* 145 is old setrlimit */
				/* 146 is old killpg */
#define	PPC_SYS_setsid	147
				/* 148 is obsolete setquota */
				/* 149 is obsolete quota */
				/* 150 is old getsockname */
#define	PPC_SYS_getpgid	151
#define PPC_SYS_setprivexec 152
#define	PPC_SYS_pread	153
#define	PPC_SYS_pwrite	154
#define	PPC_SYS_nfssvc	155
				/* 156 is old getdirentries */
#define	PPC_SYS_statfs	157
#define	PPC_SYS_fstatfs	158
#define PPC_SYS_unmount	159
				/* 160 is obsolete async_daemon */
#define	PPC_SYS_getfh	161
				/* 162 is old getdomainname */
				/* 163 is old setdomainname */
				/* 164 is obsolete pcfs_mount */
#define PPC_SYS_quotactl	165
				/* 166 is obsolete exportfs	*/
#define PPC_SYS_mount	167
				/* 168 is obsolete ustat */
				/* 169 is unused */
#define PPC_SYS_table	170
				/* 171 is old wait_3 */
				/* 172 is obsolete rpause */
				/* 173 is unused */
				/* 174 is obsolete getdents */
#define PPC_SYS_gc_control	175
#define PPC_SYS_add_profil	176
				/* 177 is unused */
				/* 178 is unused */
				/* 179 is unused */
#define PPC_SYS_kdebug_trace 180       
#define	PPC_SYS_setgid	181
#define	PPC_SYS_setegid	182
#define	PPC_SYS_seteuid	183
#ifdef __ppc__
#define	PPC_SYS_sigreturn	184
#endif
				/* 185 is unused */
				/* 186 is unused */
				/* 187 is unused */
#define	PPC_SYS_stat	188
#define	PPC_SYS_fstat	189
#define	PPC_SYS_lstat	190
#define	PPC_SYS_pathconf	191
#define	PPC_SYS_fpathconf	192
#if COMPAT_GETFSSTAT
#define	PPC_SYS_getfsstat	193
#endif
#define	PPC_SYS_getrlimit	194
#define	PPC_SYS_setrlimit	195
#define PPC_SYS_getdirentries	196
#define	PPC_SYS_mmap	197
#define	PPC_SYS___syscall	198
#define	PPC_SYS_lseek	199
#define	PPC_SYS_truncate	200
#define	PPC_SYS_ftruncate	201
#define	PPC_SYS___sysctl	202
#define PPC_SYS_mlock	203
#define PPC_SYS_munlock 	204
#define	PPC_SYS_undelete	205
#define	PPC_SYS_ATsocket	206
#define	PPC_SYS_ATgetmsg	207
#define	PPC_SYS_ATputmsg	208
#define	PPC_SYS_ATPsndreq	209
#define	PPC_SYS_ATPsndrsp	210
#define	PPC_SYS_ATPgetreq	211
#define	PPC_SYS_ATPgetrsp	212
				/* 213 is reserved for AppleTalk */
#define PPC_SYS_kqueue_from_portset_np 214
#define PPC_SYS_kqueue_portset_np	215
#define PPC_SYS_mkcomplex	216 
#define PPC_SYS_statv	217		
#define PPC_SYS_lstatv	218 			
#define PPC_SYS_fstatv	219 			
#define PPC_SYS_getattrlist	220 		
#define PPC_SYS_setattrlist	221		
#define PPC_SYS_getdirentriesattr	222 	
#define PPC_SYS_exchangedata	223 				
#define PPC_SYS_checkuseraccess	224 
#define PPC_SYS_searchfs	 225

				/* 226 - 230 are reserved for HFS expansion */
				/* 231 - 241 are reserved  */
#define	PPC_SYS_fsctl	242
				/* 243 - 246 are reserved  */
#define	PPC_SYS_nfsclnt	247	/* from freebsd, for lockd */
#define	PPC_SYS_fhopen	248	/* from freebsd, for lockd */
				/* 249 is reserved  */
#define PPC_SYS_minherit	 250
#define	PPC_SYS_semsys	251
#define	PPC_SYS_msgsys	252
#define	PPC_SYS_shmsys	253
#define	PPC_SYS_semctl	254
#define	PPC_SYS_semget	255
#define	PPC_SYS_semop	256
#define	PPC_SYS_semconfig	257
#define	PPC_SYS_msgctl	258
#define	PPC_SYS_msgget	259
#define	PPC_SYS_msgsnd	260
#define	PPC_SYS_msgrcv	261
#define	PPC_SYS_shmat	262
#define	PPC_SYS_shmctl	263
#define	PPC_SYS_shmdt	264
#define	PPC_SYS_shmget	265
#define	PPC_SYS_shm_open	266
#define	PPC_SYS_shm_unlink	267
#define	PPC_SYS_sem_open	268
#define	PPC_SYS_sem_close	269
#define	PPC_SYS_sem_unlink	270
#define	PPC_SYS_sem_wait	271
#define	PPC_SYS_sem_trywait	272
#define	PPC_SYS_sem_post	273
#define	PPC_SYS_sem_getvalue 274
#define	PPC_SYS_sem_init	275
#define	PPC_SYS_sem_destroy	276
				/* 277 - 295 are reserved  */
#define PPC_SYS_load_shared_file 296
#define PPC_SYS_reset_shared_file 297
#define PPC_SYS_new_system_shared_regions 298
				/* 299 - 309 are reserved  */
#define	PPC_SYS_getsid	310
				/* 311 - 312 are reserved */
#define	PPC_SYS_aio_fsync	313
#define	PPC_SYS_aio_return	314
#define	PPC_SYS_aio_suspend	315
#define	PPC_SYS_aio_cancel	316
#define	PPC_SYS_aio_error	317
#define	PPC_SYS_aio_read	318
#define	PPC_SYS_aio_write	319
#define	PPC_SYS_lio_listio	320
				/* 321 - 323 are reserved */
#define PPC_SYS_mlockall	 324
#define PPC_SYS_munlockall	 325
				/* 326 is reserved */
#define PPC_SYS_issetugid    327
#define PPC_SYS___pthread_kill    328
#define PPC_SYS_pthread_sigmask    329
#define PPC_SYS_sigwait    330

#define PPC_SYS_audit		350	/* submit user space audit records */
#define PPC_SYS_auditon		351	/* audit subsystem control */
#define PPC_SYS_auditsvc		352	/* audit file control */
#define PPC_SYS_getauid		353
#define PPC_SYS_setauid		354
#define PPC_SYS_getaudit		355
#define PPC_SYS_setaudit		356
#define PPC_SYS_getaudit_addr	357
#define PPC_SYS_setaudit_addr	358
#define PPC_SYS_auditctl		359	/* audit control */

#define PPC_SYS_kqueue    362
#define PPC_SYS_kevent    363

#define NETSIM_SYS_ENTER 397		/* Instructions for the netsim NIC */
#define NETSIM_TX_ENTER 398		/* Send instruction for the netsim NIC */
#define NETSIM_SYS_PICKUP 399		/* Pickup data from the netsim NIC */


#define PPC_highest_syscall 400
