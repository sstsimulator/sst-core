/*
 * memory.h - flat memory space interfaces
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
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
 * $Id: ssb_memory.h,v 1.1.1.1 2006-01-31 16:35:50 afrodri Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2005/08/16 21:12:55  arodrig6
 * changes for docs
 *
 * Revision 1.4  2004/11/16 04:17:46  arodrig6
 * added more documentation
 *
 * Revision 1.3  2004/08/24 15:43:30  arodrig6
 * added 'condem()' to thread. Integrated off-chip dram with conventional processor
 *
 * Revision 1.2  2004/08/10 22:55:35  arodrig6
 * works, except for the speculative stuff, caches, and multiple procs
 *
 * Revision 1.1  2004/08/05 23:51:44  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.4  2000/05/31 01:59:51  karu
 * added support for mis-aligned accesses
 *
 * Revision 1.3  2000/04/11 00:55:56  karu
 * made source platform independent. compiles on linux, solaris and ofcourse aix.
 * powerpc-nonnative.def is the def file with no inline assemble code.
 * manually create the machine.def symlink if compiling on non-aix OS.
 * also syscall.c does not compile because the syscalls haven't been
 * resourced with diff. flags and names for different operating systems.
 *
 * Revision 1.2  2000/04/10 23:46:30  karu
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
 * Revision 1.6  1998/08/27 15:39:47  taustin
 * implemented host interface description in host.h
 * added target interface support
 * memory module updated to support 64/32 bit address spaces on 64/32
 *       bit machines, now implemented with a dynamically optimized hashed
 *       page table
 * added support for quadword's
 * added fault support
 * added support for register and memory contexts
 *
 * Revision 1.5  1997/03/11  01:16:23  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 * major macro reorganization to support CC portability
 * mem_valid() added, indicates if an address is bogus, used by DLite!
 *
 * Revision 1.4  1997/01/06  16:01:24  taustin
 * HIDE_MEM_TABLE_DEF added to help with sim-fast.c compilation
 *
 * Revision 1.3  1996/12/27  15:53:15  taustin
 * updated comments
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef SSB_MEMORY_H
#define SSB_MEMORY_H

#include <stdio.h>

#include "ssb_host.h"
#include "ssb_misc.h"
#include "ssb_machine.h"
#include "ssb_options.h"
#include "ssb_stats.h"

/*
int READMASKSLEFT[] = { 0xFFFFFFFF, 0x00FFFFFF, 0x0000FFFF, 0x000000FF };
int WRITEMASKSLEFT[] = { 0xFFFFFFFF, 0xFF000000, 0xFFFF0000, 0xFFFFFF00 };
int WRITEMASKSRIGHT[] = { 0xFFFFFFFF, 0x00FFFFFF, 0x0000FFFF, 0x000000FF };
*/

extern int READMASKSLEFT[];
extern int WRITEMASKSLEFT[];
extern int WRITEMASKSRIGHT[];

/* number of entries in page translation hash table (must be power-of-two) */
#define MEM_PTAB_SIZE		(32*1024)
#define MEM_LOG_PTAB_SIZE	15

//: page table entry 
//!SEC:ssBack
struct mem_pte_t {
  //: next translation in this bucket 
   mem_pte_t *next;
  //: virtual page number tag 
  md_addr_t tag;		
  //: page pointer 
  byte_t *page;			
};

//: memory object 
//!SEC:ssBack
struct mem_t {
  /* memory object state */
  //: name of this memory space 
  char *name;				
  //: inverted page table 
   mem_pte_t *ptab[MEM_PTAB_SIZE];

  /* memory object stats */
  //: total number of pages allocated 
  counter_t page_count;		
  //: total first level page tbl misses 
  counter_t ptab_misses;		
  //: total page table accesses 
  counter_t ptab_accesses;		
};

/* memory access command */
enum mem_cmd {
  Read,			/* read memory from target (simulated prog) to host */
  Write,		/* write memory from host (simulator) to target */
  Inject                /* Inject memory into cache */
};

/* memory access function type, this is a generic function exported for the
   purpose of access the simulated vitual memory space */
typedef enum md_fault_type
(*mem_access_fn)( mem_t *mem,	/* memory space to access */
		 enum mem_cmd cmd,	/* Read or Write */
		 md_addr_t addr,	/* target memory address to access */
		 void *p,		/* where to copy to/from */
		 int nbytes);		/* transfer length in bytes */

/*
 * virtual to host page translation macros
 */

/* compute page table set */
#define MEM_PTAB_SET(ADDR)						\
  (((ADDR) >> MD_LOG_PAGE_SIZE) & (MEM_PTAB_SIZE - 1))

/* compute page table tag */
#define MEM_PTAB_TAG(ADDR)						\
  ((ADDR) >> (MD_LOG_PAGE_SIZE + MEM_LOG_PTAB_SIZE))

/* convert a pte entry at idx to a block address */
#define MEM_PTE_ADDR(PTE, IDX)						\
  (((PTE)->tag << (MD_LOG_PAGE_SIZE + MEM_LOG_PTAB_SIZE))		\
   | ((IDX) << MD_LOG_PAGE_SIZE))

#define MEM_OFFSET(ADDR)	((ADDR) & (MD_PAGE_SIZE - 1))
/* locate host page for virtual address ADDR, returns NULL if unallocated */
#define MEM_PAGE(MEM, ADDR)						\
  (/* first attempt to hit in first entry, otherwise call xlation fn */	\
   ((MEM)->ptab[MEM_PTAB_SET(ADDR)]					\
    && (MEM)->ptab[MEM_PTAB_SET(ADDR)]->tag == MEM_PTAB_TAG(ADDR))	\
   ? (/* hit - return the page address on host */			\
      (MEM)->ptab_accesses++, \
      (MEM)->ptab[MEM_PTAB_SET(ADDR)]->page)				\
   : (/* first level miss - call the translation helper function */	\
      mem_translate((MEM), (ADDR))))

/* compute address of access within a host page */

/* memory tickle function, allocates pages when they are first written */
#define MEM_TICKLE(MEM, ADDR)						\
  (!MEM_PAGE(MEM, ADDR)							\
   ? (/* allocate page at address ADDR */				\
      mem_newpage(MEM, ADDR))						\
   : (/* nada... */ (void)0))

/* memory page iterator */
#define MEM_FORALL(MEM, ITER, PTE)					\
  for ((ITER)=0; (ITER) < MEM_PTAB_SIZE; (ITER)++)			\
    for ((PTE)=(MEM)->ptab[i]; (PTE) != NULL; (PTE)=(PTE)->next)


/*
 * memory accessors macros, fast but difficult to debug...
 */

/* safe version, works only with scalar types */
/* FIXME: write a more efficient GNU C expression for this... */
#define MEM_READ(MEM, ADDR, TYPE)					\
  (MEM_PAGE(MEM, (md_addr_t)(ADDR))					\
   ? *((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR)) + MEM_OFFSET(ADDR)))	\
   : /* page not yet allocated, return zero value */ 0)

/* unsafe version, works with any type */
#define __UNCHK_MEM_READ(MEM, ADDR, TYPE)				\
  (*((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR)) + MEM_OFFSET(ADDR))))

/* safe version, works only with scalar types */
/* FIXME: write a more efficient GNU C expression for this... */
#define MEM_WRITE(MEM, ADDR, TYPE, VAL)					\
  (MEM_TICKLE(MEM, (md_addr_t)(ADDR)),					\
   *((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR)) + MEM_OFFSET(ADDR))) = (VAL))
      
/* unsafe version, works with any type */
#define __UNCHK_MEM_WRITE(MEM, ADDR, TYPE, VAL)				\
  (*((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR)) + MEM_OFFSET(ADDR))) = (VAL))


/* fast memory accessor macros, typed versions */
#if 0
#define MEM_READ_BYTE(MEM, ADDR)	MEM_READ(MEM, ADDR, byte_t)
#define MEM_READ_SBYTE(MEM, ADDR)	MEM_READ(MEM, ADDR, sbyte_t)
#define MEM_READ_HALF(MEM, ADDR)	MEM_READ(MEM, ADDR, half_t)
#define MEM_READ_SHALF(MEM, ADDR)	MEM_READ(MEM, ADDR, shalf_t)
#define MEM_READ_WORD(MEM, ADDR)	MEM_READ(MEM, ADDR, word_t)
#define MEM_READ_SWORD(MEM, ADDR)	MEM_READ(MEM, ADDR, sword_t)
#define MEM_READ_SFLOAT(MEM, ADDR)	MEM_READ(MEM, ADDR, sfloat_t)
#define MEM_READ_DFLOAT(MEM, ADDR)	MEM_READ(MEM, ADDR, dfloat_t)

#ifdef HOST_HAS_QWORD
#define MEM_READ_QWORD(MEM, ADDR)	MEM_READ(MEM, ADDR, qword_t)
#define MEM_READ_SQWORD(MEM, ADDR)	MEM_READ(MEM, ADDR, sqword_t)
#endif /* HOST_HAS_QWORD */

#define MEM_WRITE_BYTE(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, byte_t, VAL)
#define MEM_WRITE_SBYTE(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, sbyte_t, VAL)
#define MEM_WRITE_HALF(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, half_t, VAL)
#define MEM_WRITE_SHALF(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, shalf_t, VAL)
#define MEM_WRITE_WORD(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, word_t, VAL)
#define MEM_WRITE_SWORD(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, sword_t, VAL)
#define MEM_WRITE_SFLOAT(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, sfloat_t, VAL)
#define MEM_WRITE_DFLOAT(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, dfloat_t, VAL)

#ifdef HOST_HAS_QWORD
#define MEM_WRITE_QWORD(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, qword_t, VAL)
#define MEM_WRITE_SQWORD(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, sqword_t, VAL)
#endif /* HOST_HAS_QWORD */
#endif

#ifdef PPC_SWAP

#define SWAP_HALF(X)                                                    \
  (((((half_t)(X)) & 0xff) << 8) | ((((half_t)(X)) & 0xff00) >> 8))
#define SWAP_WORD(X)    (((word_t)(X) << 24) |                          \
                         (((word_t)(X) << 8)  & 0x00ff0000) |           \
                         (((word_t)(X) >> 8)  & 0x0000ff00) |           \
                         (((word_t)(X) >> 24) & 0x000000ff))
#define SWAP_QWORD(X)   (((qword_t)(X) << 56) |                         \
                         (((qword_t)(X) << 40) & ULL(0x00ff000000000000)) |\
                         (((qword_t)(X) << 24) & ULL(0x0000ff0000000000)) |\
                         (((qword_t)(X) << 8)  & ULL(0x000000ff00000000)) |\
                         (((qword_t)(X) >> 8)  & ULL(0x00000000ff000000)) |\
                         (((qword_t)(X) >> 24) & ULL(0x0000000000ff0000)) |\
                         (((qword_t)(X) >> 40) & ULL(0x000000000000ff00)) |\
                         (((qword_t)(X) >> 56) & ULL(0x00000000000000ff)))
#else 

#define SWAP_HALF(X) (X)
#define SWAP_WORD(X) (X)
#define SWAP_QWORD(X) (X)

#endif

#define MD_SWAPH(X)             SWAP_HALF(X)
#define MD_SWAPW(X)             SWAP_WORD(X)
#define MD_SWAPQ(X)             SWAP_QWORD(X)
#define MD_SWAPI(X)             SWAP_WORD(X)

#define ADDR_REM(ADDR) ((ADDR)%4)
#define LEFTWORD(MEM, ADDR) (MEM_READ(MEM, (ADDR-((ADDR)%4)), word_t) )
#define RIGHTWORD(MEM,ADDR) (MEM_READ(MEM, ((ADDR-((ADDR)%4))+4), word_t) )

#define MEM_READ_BYTE(MEM, ADDR)        MEM_READ(MEM, ADDR, byte_t)
#define MEM_READ_SBYTE(MEM, ADDR)       MEM_READ(MEM, ADDR, sbyte_t)
/*
#define MEM_READ_HALF(MEM, ADDR)        MD_SWAPH(MEM_READ(MEM, ADDR, half_t))
#define MEM_READ_SHALF(MEM, ADDR)       MD_SWAPH(MEM_READ(MEM, ADDR, shalf_t))
#define MEM_READ_WORD(MEM, ADDR)        MD_SWAPW(MEM_READ(MEM, ADDR, word_t))
#define MEM_READ_SWORD(MEM, ADDR)       MD_SWAPW(MEM_READ(MEM, ADDR, sword_t))
*/

#define MEM_READ_HALF(MEM, ADDR) 	((ADDR%4)==0)?(MEM_READ(MEM, ADDR, half_t)):\
								((half_t) ( ( (MEM_READ_BYTE(MEM,ADDR) << 8) | \
									 (MEM_READ_BYTE(MEM,ADDR+1)) )\
								))
#define MEM_READ_SHALF(MEM, ADDR)    ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, shalf_t)):\
                        ((shalf_t) ( ( (MEM_READ_BYTE(MEM,ADDR) << 8) | \
                            (MEM_READ_BYTE(MEM,ADDR+1)) )\
                        ))

#define MEM_READ_WORD(MEM, ADDR) ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, word_t)):\
						((word_t) (( (LEFTWORD(MEM,ADDR) & READMASKSLEFT[ADDR_REM(ADDR)]) << (ADDR_REM(ADDR)*8)) |\
							(RIGHTWORD(MEM,ADDR) >> ((4-ADDR_REM(ADDR))*8)) ))
#define MEM_READ_SWORD(MEM, ADDR) ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, sword_t)):\
                  ((sword_t) (( (LEFTWORD(MEM,ADDR) & READMASKSLEFT[ADDR_REM(ADDR)]) << (ADDR_REM(ADDR)*8)) |\
                     (RIGHTWORD(MEM, ADDR) >> ((4-ADDR_REM(ADDR))*8)) ))


#ifdef HOST_HAS_QWORD
#define MEM_READ_QWORD(MEM, ADDR)       MD_SWAPQ( ( (qword_t) (\
			( ((qword_t) (MEM_READ(MEM, ADDR, word_t))) << 32) |					\
			((MEM_READ(MEM, ADDR+4, word_t)))	\
			) ) )
#define MEM_READ_SQWORD(MEM, ADDR)      MD_SWAPQ( ( (qword_t) (\
         ( ((qword_t) (MEM_READ(MEM, ADDR, word_t))) << 32) |              \
         ((MEM_READ(MEM, ADDR+4, word_t)))   \
         ) ) )

#endif /* HOST_HAS_QWORD */

#define MEM_WRITE_BYTE(MEM, ADDR, VAL)  MEM_WRITE(MEM, ADDR, byte_t, VAL)
#define MEM_WRITE_SBYTE(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, sbyte_t, VAL)
/*
#define MEM_WRITE_HALF(MEM, ADDR, VAL)                                  \
                                MEM_WRITE(MEM, ADDR, half_t, MD_SWAPH(VAL))
#define MEM_WRITE_SHALF(MEM, ADDR, VAL)                                 \
                                MEM_WRITE(MEM, ADDR, shalf_t, MD_SWAPH(VAL))
*/
#define MEM_WRITE_HALF(MEM, ADDR, VAL) (((ADDR)%4)==0)?MEM_WRITE(MEM, ADDR, half_t, MD_SWAPH(VAL)):\
									(\
										(MEM_WRITE_BYTE(MEM, ADDR, ((VAL)>>8)) ),\
										(MEM_WRITE_BYTE(MEM, ((ADDR)+1), ((VAL)&0xFF)) )\
									)
#define MEM_WRITE_SHALF(MEM, ADDR, VAL) (((ADDR)%4)==0)?MEM_WRITE(MEM, ADDR, shalf_t, MD_SWAPH(VAL)):\
                           (\
                              (MEM_WRITE_BYTE(MEM, ADDR, ((VAL)>>8)) ),\
                              (MEM_WRITE_BYTE(MEM, ((ADDR)+1), ((VAL)&0xFF)) )\
                           )

/*
#define MEM_WRITE_WORD(MEM, ADDR, VAL)                                  \
                                MEM_WRITE(MEM, ADDR, word_t, MD_SWAPW(VAL))
#define MEM_WRITE_SWORD(MEM, ADDR, VAL)                                 \
                                MEM_WRITE(MEM, ADDR, sword_t, MD_SWAPW(VAL))
*/
#define MEM_WRITE_WORD(MEM, ADDR, VAL) (((ADDR)%4)==0)?\
										MEM_WRITE(MEM, (ADDR), word_t, MD_SWAPW(VAL)):\
										(\
										(\
										MEM_WRITE(MEM, ((ADDR)-((ADDR)%4)), word_t, ((LEFTWORD(MEM, (ADDR) ) &  WRITEMASKSLEFT[ADDR_REM(ADDR)]) | (VAL >> (ADDR_REM(ADDR)*8))) )	\
										),	\
										(\
										MEM_WRITE(MEM, ((ADDR)-((ADDR)%4)+4), word_t, ((RIGHTWORD(MEM, (ADDR)) &  WRITEMASKSRIGHT[ADDR_REM(ADDR)]) | (VAL << ((4-ADDR_REM(ADDR))*8))) )									\
										)		\
										)
#define MEM_WRITE_SWORD(MEM, ADDR, VAL) (((ADDR)%4)==0)?\
                              MEM_WRITE(MEM, ADDR, sword_t, MD_SWAPW(VAL)):\
                              MEM_WRITE_WORD(MEM, ADDR, VAL)
 

											

#define MEM_WRITE_SFLOAT(MEM, ADDR, VAL)                                \
                                MEM_WRITE(MEM, ADDR, sfloat_t, MD_SWAPW(VAL))
#define MEM_WRITE_DFLOAT(MEM, ADDR, VAL)                                \
                                MEM_WRITE(MEM, ADDR, dfloat_t, MD_SWAPQ(VAL))

#ifdef HOST_HAS_QWORD
/*
#define MEM_WRITE_QWORD(MEM, ADDR, VAL)                                 \
                                MEM_WRITE(MEM, ADDR, qword_t, MD_SWAPQ(VAL))
#define MEM_WRITE_SQWORD(MEM, ADDR, VAL)                                \
                                MEM_WRITE(MEM, ADDR, sqword_t, MD_SWAPQ(VAL))
*/
#define MEM_WRITE_QWORD(MEM, ADDR, VAL) \
									(MEM_WRITE_WORD(MEM, ADDR, (((qword_t) (VAL))>>32))  , MEM_WRITE_WORD(MEM, ADDR, ((VAL)&0xFFFFFFFF)))
#define MEM_WRITE_SQWORD(MEM, ADDR, VAL) \
									MEM_WRITE_QWORD(MEM, ADDR, VAL)

#endif /* HOST_HAS_QWORD */

/* create a flat memory space */
struct mem_t *
mem_create(char *name);			/* name of the memory space */
	   
/* translate address ADDR in memory space MEM, returns pointer to host page */
byte_t *
mem_translate( mem_t *mem,	/* memory space to access */
	      md_addr_t addr);		/* virtual address to translate */

/* allocate a memory page */
void
mem_newpage( mem_t *mem,		/* memory space to allocate in */
	    md_addr_t addr);		/* virtual address to allocate */

/* generic memory access function, it's safe because alignments and permissions
   are checked, handles any natural transfer sizes; note, faults out if nbytes
   is not a power-of-two or larger then MD_PAGE_SIZE */
enum md_fault_type
mem_access( mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes);			/* number of bytes to access */

/* register memory system-specific statistics */
void
mem_reg_stats( mem_t *mem,	/* memory space to declare */
	       stat_sdb_t *sdb);	/* stats data base */

/* initialize memory system, call before loader.c */
void
mem_init( mem_t *mem);	/* memory space to initialize */

/* dump a block of memory, returns any faults encountered */
enum md_fault_type
mem_dump( mem_t *mem,		/* memory space to display */
	 md_addr_t addr,		/* target address to dump */
	 int len,			/* number bytes to dump */
	 FILE *stream);			/* output stream */


/*
 * memory accessor routines, these routines require a memory access function
 * definition to access memory, the memory access function provides a "hook"
 * for programs to instrument memory accesses, this is used by various
 * simulators for various reasons; for the default operation - direct access
 * to the memory system, pass mem_access() as the memory access function
 */

/* copy a '\0' terminated string to/from simulated memory space, returns
   the number of bytes copied, returns any fault encountered */
enum md_fault_type
mem_strcpy(mem_access_fn mem_fn,	/* user-specified memory accessor */
	    mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   char *s);			/* host memory string buffer */

/* copy NBYTES to/from simulated memory space, returns any faults */
enum md_fault_type
mem_bcopy(mem_access_fn mem_fn,		/* user-specified memory accessor */
	   mem_t *mem,		/* memory space to access */
	  enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  md_addr_t addr,		/* target address to access */
	  void *vp,			/* host memory address to access */
	  int nbytes);			/* number of bytes to access */

/* copy NBYTES to/from simulated memory space, NBYTES must be a multiple
   of 4 bytes, this function is faster than mem_bcopy(), returns any
   faults encountered */
enum md_fault_type
mem_bcopy4(mem_access_fn mem_fn,	/* user-specified memory accessor */
	    mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes);			/* number of bytes to access */

/* zero out NBYTES of simulated memory, returns any faults encountered */
enum md_fault_type
mem_bzero(mem_access_fn mem_fn,		/* user-specified memory accessor */
	   mem_t *mem,		/* memory space to access */
	  md_addr_t addr,		/* target address to access */
	  int nbytes);			/* number of bytes to clear */

#endif /* MEMORY_H */
