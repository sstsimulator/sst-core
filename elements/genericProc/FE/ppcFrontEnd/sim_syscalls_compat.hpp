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
#ifndef __SIM_SYSCALLS_COMPAT_HPP__
#define __SIM_SYSCALLS_COMPAT_HPP__

#include "global.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>

/*
 * This file is intended as a compatibility file for 32 / 64 bit compatibility
 * modes.  Structs and adapters are provided to enforce proper data structure
 * sizes in simulated memory.
 */

// wcm: 64-bit port: the timeval struct from sys/time.h varies in 
//      size for 32 and 64-bit modes, need to force a 32-bit version
//      for simulated-land.
typedef struct timeval32 {
    int32 tv_sec;
    int32 tv_usec;
} timeval32;


// wcm: convert from system timeval to 32-bit timeval.
void timevalToTimeval32(timeval* s, timeval32* t)
{
    t->tv_sec  = (int32)s->tv_sec;
    t->tv_usec = (int32)s->tv_usec;
};


/* wcm: the stat data struct size changes from 32->64 bit modes 
 *      see /usr/include/sys/stat.h for template 
 */
struct timespec32
{
  int32 tv_sec;       
  int32 tv_nsec;
};

#ifdef __ppc64__
struct stat32 
{
    dev_t       st_dev;
    ino_t       st_ino;
    mode_t      st_mode;
    nlink_t     st_nlink;
    uid_t       st_uid;
    gid_t       st_gid;
    dev_t       st_rdev;
#ifndef _POSIX_C_SOURCE
    struct timespec32 st_atimespec;
    struct timespec32 st_mtimespec;
    struct timespec32 st_ctimespec;
#else
    int32 st_atime;
    int32 st_atimensec;
    int32 st_mtime;
    int32 st_mtimensec;
    int32 st_ctime;
    int32 st_ctimensec;
#endif
    off_t       st_size;
    blkcnt_t    st_blocks;
    blksize_t   st_blksize;
    __uint32_t  st_flags;
    __uint32_t  st_gen;
    __int32_t   st_lspare;
    __int64_t   st_qspare[2];
};
typedef struct stat32 stat32_s;
#elif !defined(__x86_64__)
// since x86_64 will have it's own stat32_s definition 
typedef struct stat stat32_s;
#endif

typedef struct stat stat_s;

#ifdef __x86_64__
// This is what struct stat looks like on a 32-bit ppc

#define ppc_32_dev_t	int32_t
#define ppc_32_ino_t	uint32_t
#define ppc_32_mode_t	uint16_t
#define ppc_32_nlink_t	uint16_t
#define ppc_32_uid_t	uint32_t
#define ppc_32_gid_t	uint32_t
#define ppc_32_off_t	int64_t
#define ppc_32_quad_t	int64_t
#define ppc_32_u_long	uint32_t
typedef struct ppc_32_timespec {
    uint32_t	tv_sec;
    uint32_t	tv_nsec;
} ppc_32_timespec;


typedef struct stat_ppc_32_t {
    ppc_32_dev_t    st_dev;    /* device inode resides on */
    ppc_32_ino_t    st_ino;    /* inode's number */
    ppc_32_mode_t   st_mode;   /* inode protection mode */
    ppc_32_nlink_t  st_nlink;  /* number or hard links to the file */
    ppc_32_uid_t    st_uid;    /* user-id of owner */
    ppc_32_gid_t    st_gid;    /* group-id of owner */
    ppc_32_dev_t    st_rdev;   /* device type, for special file inode */
    struct ppc_32_timespec st_atimespec;  /* time of last access */
    struct ppc_32_timespec st_mtimespec;  /* time of last data modification */
    struct ppc_32_timespec st_ctimespec;  /* time of last file status change */
    ppc_32_off_t    st_size;   /* file size, in bytes */
    ppc_32_quad_t   st_blocks; /* blocks allocated for file */
    ppc_32_u_long   st_blksize;/* optimal file sys I/O ops blocksize */
    ppc_32_u_long   st_flags;  /* user defined flags for file */
    ppc_32_u_long   st_gen;    /* file generation number */
} stat32_s;
#endif


/* conversion function to ensure proper byte-alignment when
 * running 32-bit programs in the 64-bit simulator */
static void statToStat32(stat_s* source, stat32_s* target)
{
    target->st_dev   = source->st_dev;
    target->st_ino   = source->st_ino;
    target->st_mode  = source->st_mode;
    target->st_nlink = source->st_nlink;
    target->st_uid   = source->st_uid;
    target->st_gid   = source->st_gid;
    target->st_rdev  = source->st_rdev;
    target->st_size      = source->st_size;
    target->st_blocks    = source->st_blocks;
    target->st_blksize   = source->st_blksize;

#ifdef HAVE_STAT_ST_FLAGS
    target->st_flags     = 0; // fake this
#endif
#ifdef HAVE_STAT_ST_GEN
    target->st_gen       = 0; // fake this
#endif

#ifdef _POSIX_C_SOURCE
# ifdef HAVE_STAT_ST_ATIMESPEC
    target->st_atimespec.tv_sec  = source->st_atime;
    target->st_atimespec.tv_nsec = 0;
# endif
# ifdef HAVE_STAT_ST_MTIMESPEC
    target->st_mtimespec.tv_sec  = source->st_mtime;
    target->st_mtimespec.tv_nsec = 0;
# endif
# ifdef HAVE_STAT_ST_CTIMESPEC
    target->st_ctimespec.tv_sec  = source->st_ctime;
    target->st_ctimespec.tv_nsec = 0;
# endif
#else //  NOT _POSIX_C_SOURCE
    target->st_atimespec.tv_sec  = source->st_atimespec.tv_sec;
    target->st_atimespec.tv_nsec = source->st_atimespec.tv_nsec;
    target->st_mtimespec.tv_sec  = source->st_mtimespec.tv_sec;
    target->st_mtimespec.tv_nsec = source->st_mtimespec.tv_nsec;
    target->st_ctimespec.tv_sec  = source->st_ctimespec.tv_sec;
    target->st_ctimespec.tv_nsec = source->st_ctimespec.tv_nsec;
#endif // _POSIX_C_SOURCE
};


// the system rusage struct varies from 32 to 64bit in size,
// must hardwire it for simulated-space.
typedef struct rusage32 
{
    timeval32 ru_utime;   /* user time used */
    timeval32 ru_stime;   /* system time used */
    int32 ru_maxrss;      /* integral max resident set size */
    int32 ru_ixrss;       /* integral shared text size */
    int32 ru_idrss;       /* integral unshared data size */
    int32 ru_isrss;       /* integral unshared stack size */
    int32 ru_minflt;      /* page reclaims */
    int32 ru_majflt;      /* page faults */
    int32 ru_nswap;       /* swaps */
    int32 ru_inblock;     /* block input operations */
    int32 ru_oublock;     /* block output operations */
    int32 ru_msgsnd;      /* messages sent */
    int32 ru_msgrcv;      /* messages received */
    int32 ru_nsignals;    /* signals received */
    int32 ru_nvcsw;       /* voluntary context switches */
    int32 ru_nivcsw;      /* involuntary context switches */
} rusage32;


void rusageToRusage32(rusage* s, rusage32* t)
{
    t->ru_utime.tv_sec  = s->ru_utime.tv_sec;
    t->ru_utime.tv_usec = s->ru_utime.tv_usec;

    t->ru_stime.tv_sec  = s->ru_stime.tv_sec;
    t->ru_stime.tv_usec = s->ru_stime.tv_usec;

    t->ru_maxrss   = (int32)s->ru_maxrss;
    t->ru_ixrss    = (int32)s->ru_ixrss;
    t->ru_idrss    = (int32)s->ru_idrss;
    t->ru_isrss    = (int32)s->ru_isrss;
    t->ru_minflt   = (int32)s->ru_minflt;
    t->ru_majflt   = (int32)s->ru_majflt;
    t->ru_nswap    = (int32)s->ru_nswap;
    t->ru_inblock  = (int32)s->ru_inblock;
    t->ru_oublock  = (int32)s->ru_oublock;
    t->ru_msgsnd   = (int32)s->ru_msgsnd;
    t->ru_msgrcv   = (int32)s->ru_msgrcv;
    t->ru_nsignals = (int32)s->ru_nsignals;
    t->ru_nvcsw    = (int32)s->ru_nvcsw;
    t->ru_nivcsw   = (int32)s->ru_nivcsw;
}






#endif
// EOF
