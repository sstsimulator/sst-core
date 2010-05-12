/*
 * host.h - host-dependent definitions and interfaces
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1998 by Todd M. Austin
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
 * $Id: ssb_host.h,v 1.3 2007-08-07 22:30:50 wcmclen Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2007/07/09 18:37:50  wcmclen
 * Merging files from 64-bit branch into TRUNK.  May require another commit
 * before merge is fully done, will TAG the repository once everything is stable.
 *
 * Revision 1.1.1.1.2.1  2007/05/29 18:58:35  wcmclen
 * Updated type definitions to be hardcoded to the correct bit-width regardless
 * of 64-bit or 32-bit mode of serialProto.
 *
 * Revision 1.1.1.1  2006/01/31 16:35:49  afrodri
 * first entry
 *
 * Revision 1.1  2004/08/05 23:51:44  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.2  2000/04/10 23:46:29  karu
 * converted all quad to qword. NO OTHER change made.
 * the tag specfp95-before-quad-qword is a snapshow before this change was made
 *
 * Revision 1.1.1.1  2000/03/07 05:15:17  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:50  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.1  1998/08/27 08:28:46  taustin
 * Initial revision
 *
 *
 */

#ifndef HOST_H
#define HOST_H
#include <unistd.h>
#include <sst_stdint.h>

/* make sure host compiler supports ANSI-C */
#ifndef __STDC__ /* an ansi C compiler is required */
#error The SimpleScalar simulators must be compiled with an ANSI C compiler.
#endif /* __STDC__ */

/* enable inlining here, if supported by host compiler */
#undef INLINE
#if defined(__GNUC__)
#define INLINE		inline
#else
#define INLINE
#endif

/* bind together two symbols, at preprocess time */
#ifdef __GNUC__
/* this works on all GNU GCC targets (that I've seen...) */
#define SYMCAT(X,Y)	X##Y
#define ANSI_SYMCAT
#else /* !__GNUC__ */
#ifdef OLD_SYMCAT
#define SYMCAT(X,Y)	X/**/Y
#else /* !OLD_SYMCAT */
#define SYMCAT(X,Y)	X##Y
#define ANSI_SYMCAT
#endif /* OLD_SYMCAT */
#endif /* __GNUC__ */

/* host-dependent canonical type definitions */
typedef int bool_t;			/* generic boolean type */
//typedef unsigned char byte_t;		/* byte - 8 bits */
//typedef signed char sbyte_t;
//typedef unsigned short half_t;	/* half - 16 bits */
//typedef signed short shalf_t;
//typedef unsigned int word_t;		/* word - 32 bits */
//typedef signed int sword_t;
typedef uint8_t  byte_t;                /* byte - 8 bits */
typedef int8_t   sbyte_t;                
typedef uint16_t half_t;                /* half - 16 bits */
typedef int16_t  shalf_t;               
typedef uint32_t word_t;                /* word - 32 bits */
typedef int32_t  sword_t;
typedef float    sfloat_t;		/* single-precision float - 32 bits */
typedef double   dfloat_t;		/* double-precision float - 64 bits */

/* quadword defs, note: not all targets support quadword types */
#if defined(__GNUC__) || defined(__SUNPRO_C) || defined(__CC_C89) || defined(__CC_XLC)
#define HOST_HAS_QWORD
#if !defined(__FreeBSD__)
//typedef unsigned long long qword_t;	/* quad - 64 bits */
//typedef signed long long sqword_t;
typedef uint64_t qword_t;               /* quad - 64 bits */
typedef int64_t  sqword_t;
#else /* __FreeBSD__ */
//#define qword_t	unsigned long long
//#define sqword_t	signed long long
#define qword_t         uint64_t
#define sqword_t        int64_t
#endif /* __FreeBSD__ */
#ifdef ANSI_SYMCAT
#define ULL(N)		N##ULL		/* qword_t constant */
#define LL(N)		N##LL		/* sqword_t constant */
#else /* OLD_SYMCAT */
#define ULL(N)		N/**/ULL		/* qword_t constant */
#define LL(N)		N/**/LL		/* sqword_t constant */
#endif
#elif defined(__alpha)
#define HOST_HAS_QWORD
typedef unsigned long qword_t;		/* quad - 64 bits */
typedef signed long sqword_t;
#ifdef ANSI_SYMCAT
#define ULL(N)		N##UL		/* qword_t constant */
#define LL(N)		N##L		/* sqword_t constant */
#else /* OLD_SYMCAT */
#define ULL(N)		N/**/UL		/* qword_t constant */
#define LL(N)		N/**/L		/* sqword_t constant */
#endif
#elif defined(_MSC_VER)
#define HOST_HAS_QWORD
typedef unsigned __int64 qword_t;	/* quad - 64 bits */
typedef signed __int64 sqword_t;
#define ULL(N)		((qword_t)(N))
#define LL(N)		((sqword_t)(N))
#else /* !__GNUC__ && !__alpha */
#undef HOST_HAS_QWORD
#endif

/* statistical counter types, use largest counter type available */
#ifdef HOST_HAS_QWORD
typedef sqword_t counter_t;
typedef sqword_t tick_t;			/* NOTE: unsigned breaks caches */
#else /* !HOST_HAS_QWORD */
typedef dfloat_t counter_t;
typedef dfloat_t tick_t;
#endif /* HOST_HAS_QWORD */

#endif /* HOST_H */
