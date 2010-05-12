/*
 * memory.c - flat memory space routines
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
 * $Id: ssb_memory.cc,v 1.1.1.1 2006-01-31 16:35:50 afrodri Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2005/08/16 21:12:55  arodrig6
 * changes for docs
 *
 * Revision 1.2  2005/06/28 21:21:22  arodrig6
 * changes for GCC 4.0 - mainly making simpleScalar tighter
 *
 * Revision 1.1  2004/08/05 23:51:44  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.3  2000/05/31 01:59:50  karu
 * added support for mis-aligned accesses
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
 * Revision 1.3  1999/02/11 04:24:14  dburger
 * Added the SimpleScalar 3.0 prerelease code to the main trunk.
 *
 * Revision 1.6  1998/08/27 15:38:28  taustin
 * implemented host interface description in host.h
 * added target interface support
 * memory module updated to support 64/32 bit address spaces on 64/32
 *       bit machines, now implemented with a dynamically optimized hashed
 *       page table
 * added support for quadword's
 * added fault support
 *
 * Revision 1.5  1997/03/11  01:15:25  taustin
 * updated copyright
 * mem_valid() added, indicates if an address is bogus, used by DLite!
 * long/int tweaks made for ALPHA target support
 *
 * Revision 1.4  1997/01/06  16:00:51  taustin
 * stat_reg calls now do not initialize stat variable values
 *
 * Revision 1.3  1996/12/27  15:52:46  taustin
 * updated comments
 * integrated support for options and stats packages
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "ssb_host.h"
#include "ssb_misc.h"
#include "ssb_machine.h"
#include "ssb_options.h"
#include "ssb_stats.h"
#include "ssb_memory.h"

int READMASKSLEFT[] = { 0xFFFFFFFF, 0x00FFFFFF, 0x0000FFFF, 0x000000FF };
int WRITEMASKSLEFT[] = { 0xFFFFFFFF, 0xFF000000, 0xFFFF0000, 0xFFFFFF00 };
int WRITEMASKSRIGHT[] = { 0xFFFFFFFF, 0x00FFFFFF, 0x0000FFFF, 0x000000FF };


/* create a flat memory space */
mem_t *
mem_create(char *name)			/* name of the memory space */
{
   mem_t *mem;

  mem = (mem_t*)calloc(1, sizeof(mem_t));
  if (!mem)
    fatal("out of virtual memory");

  mem->name = mystrdup(name);
  return mem;
}

/* translate address ADDR in memory space MEM, returns pointer to host page */
byte_t *
mem_translate( mem_t *mem,	/* memory space to access */
	      md_addr_t addr)		/* virtual address to translate */
{
   mem_pte_t *pte, *prev;

  /* got here via a first level miss in the page tables */
  mem->ptab_misses++; mem->ptab_accesses++;

  /* locate accessed PTE */
  for (prev=NULL, pte=mem->ptab[MEM_PTAB_SET(addr)];
       pte != NULL;
       prev=pte, pte=pte->next)
    {
      if (pte->tag == MEM_PTAB_TAG(addr))
	{
	  /* move this PTE to head of the bucket list */
	  if (prev)
	    {
	      prev->next = pte->next;
	      pte->next = mem->ptab[MEM_PTAB_SET(addr)];
	      mem->ptab[MEM_PTAB_SET(addr)] = pte;
	    }
	  return pte->page;
	}
    }

  /* no translation found, return NULL */
  return NULL;
}

/* allocate a memory page */
void
mem_newpage( mem_t *mem,		/* memory space to allocate in */
	    md_addr_t addr)		/* virtual address to allocate */
{
  byte_t *page;
   mem_pte_t *pte;

  /* see misc.c for details on the getcore() function */
  page = (byte_t*)getcore(MD_PAGE_SIZE);
  if (!page)
    fatal("out of virtual memory");

  /* generate a new PTE */
  pte = (mem_pte_t*)calloc(1, sizeof(mem_pte_t));
  if (!pte)
    fatal("out of virtual memory");
  pte->tag = MEM_PTAB_TAG(addr);
  pte->page = page;

  /* insert PTE into inverted hash table */
  pte->next = mem->ptab[MEM_PTAB_SET(addr)];
  mem->ptab[MEM_PTAB_SET(addr)] = pte;

  /* one more page allocated */
  mem->page_count++;
}

/* generic memory access function, it's safe because alignments and permissions
   are checked, handles any natural transfer sizes; note, faults out if nbytes
   is not a power-of-two or larger then MD_PAGE_SIZE */
enum md_fault_type
mem_access( mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes)			/* number of bytes to access */
{
  byte_t *p = (byte_t*)vp;

  /* check alignments */
  if (/* check size */(nbytes & (nbytes-1)) != 0
      || /* check max size */nbytes > MD_PAGE_SIZE)
    return md_fault_access;

  if (/* check natural alignment */(addr & (nbytes-1)) != 0)
    return md_fault_alignment;

  /* perform the copy */
  switch (nbytes)
    {
    case 1:
      if (cmd == Read)
	*((byte_t *)p) = MEM_READ_BYTE(mem, addr);
      else
	MEM_WRITE_BYTE(mem, addr, *((byte_t *)p));
      break;

    case 2:
      if (cmd == Read)
	*((half_t *)p) = MEM_READ_HALF(mem, addr);
      else
	MEM_WRITE_HALF(mem, addr, *((half_t *)p));
      break;

    case 4:
      if (cmd == Read)
	*((word_t *)p) = MEM_READ_WORD(mem, addr);
      else
	MEM_WRITE_WORD(mem, addr, *((word_t *)p));
      break;

#ifdef HOST_HAS_QWORD
    case 8:
      if (cmd == Read)
	*((qword_t *)p) = MEM_READ_QWORD(mem, addr);
      else
	MEM_WRITE_QWORD(mem, addr, *((qword_t *)p));
      break;
#endif /* HOST_HAS_QWORD */

    default:
      {
	/* nbytes >= 8/16 and power of two */
	unsigned words = nbytes >> 2;

	if (cmd == Read)
	  {
	    while (words-- > 0)
	      {
		*((word_t *)p) = MEM_READ_WORD(mem, addr);
		p += sizeof(word_t);
		addr += sizeof(word_t);
	      }
	  }
	else
	  {
	    while (words-- > 0)
	      {
		MEM_WRITE_WORD(mem, addr, *((word_t *)p));
		p += sizeof(word_t);
		addr += sizeof(word_t);
	      }
	  }
      }
      break;
    }

  /* no fault... */
  return md_fault_none;
}

/* register memory system-specific statistics */
void
mem_reg_stats( mem_t *mem,	/* memory space to declare */
	       stat_sdb_t *sdb)	/* stats data base */
{
  char buf[512], buf1[512];

  sprintf(buf, "%s.page_count", mem->name);
  stat_reg_counter(sdb, buf, "total number of pages allocated",
		   &mem->page_count, mem->page_count, NULL);

  sprintf(buf, "%s.page_mem", mem->name);
  sprintf(buf1, "%s.page_count * %d / 1024", mem->name, MD_PAGE_SIZE);
  stat_reg_formula(sdb, buf, "total size of memory pages allocated",
		   buf1, "%11.0fk");

  sprintf(buf, "%s.ptab_misses", mem->name);
  stat_reg_counter(sdb, buf, "total first level page table misses",
		   &mem->ptab_misses, mem->ptab_misses, NULL);

  sprintf(buf, "%s.ptab_accesses", mem->name);
  stat_reg_counter(sdb, buf, "total page table accesses",
		   &mem->ptab_accesses, mem->ptab_accesses, NULL);

  sprintf(buf, "%s.ptab_miss_rate", mem->name);
  sprintf(buf1, "%s.ptab_misses / %s.ptab_accesses", mem->name, mem->name);
  stat_reg_formula(sdb, buf, "first level page table miss rate", buf1, NULL);
}

/* initialize memory system, call before loader.c */
void
mem_init( mem_t *mem)	/* memory space to initialize */
{
  int i;

  /* initialize the first level page table to all empty */
  for (i=0; i < MEM_PTAB_SIZE; i++)
    mem->ptab[i] = NULL;

  mem->page_count = 0;
  mem->ptab_misses = 0;
  mem->ptab_accesses = 0;
}

/* dump a block of memory, returns any faults encountered */
enum md_fault_type
mem_dump( mem_t *mem,		/* memory space to display */
	 md_addr_t addr,		/* target address to dump */
	 int len,			/* number bytes to dump */
	 FILE *stream)			/* output stream */
{
  int data;
  enum md_fault_type fault;

  if (!stream)
    stream = stderr;

  addr &= ~sizeof(word_t);
  len = (len + (sizeof(word_t) - 1)) & ~sizeof(word_t);
  while (len-- > 0)
    {
      fault = mem_access(mem, Read, addr, &data, sizeof(word_t));
      if (fault != md_fault_none)
	return fault;

      myfprintf(stream, "0x%08p: %08x\n", addr, data);
      addr += sizeof(word_t);
    }

  /* no faults... */
  return md_fault_none;
}

/* copy a '\0' terminated string to/from simulated memory space, returns
   the number of bytes copied, returns any fault encountered */
enum md_fault_type
mem_strcpy(mem_access_fn mem_fn,	/* user-specified memory accessor */
	    mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   char *s)
{
  int n = 0;
  char c;
  enum md_fault_type fault;

  switch (cmd)
    {
    case Read:
      /* copy until string terminator ('\0') is encountered */
      do {
	fault = mem_fn(mem, Read, addr++, &c, 1);
	if (fault != md_fault_none)
	  return fault;
	*s++ = c;
	n++;
      } while (c);
      break;

    case Write:
      /* copy until string terminator ('\0') is encountered */
      do {
	c = *s++;
	fault = mem_fn(mem, Write, addr++, &c, 1);
	if (fault != md_fault_none)
	  return fault;
	n++;
      } while (c);
      break;

    default:
      return md_fault_internal;
  }

  /* no faults... */
  return md_fault_none;
}

/* copy NBYTES to/from simulated memory space, returns any faults */
enum md_fault_type
mem_bcopy(mem_access_fn mem_fn,		/* user-specified memory accessor */
	   mem_t *mem,		/* memory space to access */
	  enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  md_addr_t addr,		/* target address to access */
	  void *vp,			/* host memory address to access */
	  int nbytes)
{
  byte_t *p = (byte_t*)vp;
  enum md_fault_type fault;

  /* copy NBYTES bytes to/from simulator memory */
  while (nbytes-- > 0)
    {
      fault = mem_fn(mem, cmd, addr++, p++, 1);
      if (fault != md_fault_none)
	return fault;
    }

  /* no faults... */
  return md_fault_none;
}

/* copy NBYTES to/from simulated memory space, NBYTES must be a multiple
   of 4 bytes, this function is faster than mem_bcopy(), returns any
   faults encountered */
enum md_fault_type
mem_bcopy4(mem_access_fn mem_fn,	/* user-specified memory accessor */
	    mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes)
{
  byte_t *p = (byte_t*)vp;
  int words = nbytes >> 2;		/* note: nbytes % 2 == 0 is assumed */
  enum md_fault_type fault;

  while (words-- > 0)
    {
      fault = mem_fn(mem, cmd, addr, p, sizeof(word_t));
      if (fault != md_fault_none)
	return fault;

      addr += sizeof(word_t);
      p += sizeof(word_t);
    }

  /* no faults... */
  return md_fault_none;
}

/* zero out NBYTES of simulated memory, returns any faults encountered */
enum md_fault_type
mem_bzero(mem_access_fn mem_fn,		/* user-specified memory accessor */
	   mem_t *mem,		/* memory space to access */
	  md_addr_t addr,		/* target address to access */
	  int nbytes)
{
  byte_t c = 0;
  enum md_fault_type fault;

  /* zero out NBYTES of simulator memory */
  while (nbytes-- > 0)
    {
      fault = mem_fn(mem, Write, addr++, &c, 1);
      if (fault != md_fault_none)
	return fault;
    }

  /* no faults... */
  return md_fault_none;
}









#if 0

/*
 * The SimpleScalar virtual memory address space is 2^31 bytes mapped from
 * 0x00000000 to 0x7fffffff.  The upper 2^31 bytes are currently reserved for
 * future developments.  The address space from 0x00000000 to 0x00400000 is
 * currently unused.  The address space from 0x00400000 to 0x10000000 is used
 * to map the program text (code), although accessing any memory outside of
 * the defined program space causes an error to be declared.  The address
 * space from 0x10000000 to "mem_brk_point" is used for the program data
 * segment.  This section of the address space is initially set to contain the
 * initialized data segment and then the uninitialized data segment.
 * "mem_brk_point" then grows to higher memory when sbrk() is called to
 * service heap growth.  The data segment can continue to expand until it
 * collides with the stack segment.  The stack segment starts at 0x7fffc000
 * and grows to lower memory as more stack space is allocated.  Initially,
 * the stack contains program arguments and environment variables (see
 * loader.c for details on initial stack layout).  The stack may continue to
 * expand to lower memory until it collides with the data segment.
 *
 * The SimpleScalar virtual memory address space is implemented with a
 * one level page table, where the first level table contains MEM_TABLE_SIZE
 * pointers to MEM_BLOCK_SIZE byte pages in the second level table.  Pages
 * are allocated in MEM_BLOCK_SIZE size chunks when first accessed, the initial
 * value of page memory is all zero.
 *
 * Graphically, it all looks like this:
 *
 *                 Virtual        Level 1    Host Memory Pages
 *                 Address        Page       (allocated as needed)
 *                 Space          Table
 * 0x00000000    +----------+      +-+      +-------------------+
 *               | unused   |      | |----->| memory page (64k) |
 * 0x00400000    +----------+      +-+      +-------------------+
 *               |          |      | |
 *               | text     |      +-+
 *               |          |      | |
 * 0x10000000    +----------+      +-+
 *               |          |      | |
 *               | data seg |      +-+      +-------------------+
 *               |          |      | |----->| memory page (64k) |
 * mem_brk_point +----------+      +-+      +-------------------+
 *               |          |      | |
 *               |          |      +-+
 *               |          |      | |
 * regs_R[29]    +----------+      +-+
 * (stack ptr)   |          |      | |
 *               | stack    |      +-+
 *               |          |      | |
 * 0x7fffc000    +----------+      +-+      +-------------------+
 *               | unsed    |      | |----->| memory page (64k) |
 * 0x7fffffff    +----------+      +-+      +-------------------+

 */

/* top of the data segment, sbrk() moves this to higher memory */
extern SS_ADDR_TYPE mem_brk_point;

/* lowest address accessed on the stack */
extern SS_ADDR_TYPE mem_stack_min;

/*
 * memory page table defs
 */

/* memory indirect table size (upper mem is not used) */
#define MEM_TABLE_SIZE		0x8000 /* was: 0x7fff */

#ifndef HIDE_MEM_TABLE_DEF	/* used by sim-fast.c */
/* the level 1 page table map */
extern char *mem_table[MEM_TABLE_SIZE];
#endif /* HIDE_MEM_TABLE_DEF */

/* memory block size, in bytes */
#define MEM_BLOCK_SIZE		0x10000

  /* check permissions, no probes allowed into undefined segment regions */
  if (!(/* text access and a read */
	(addr >= ld_text_base && addr < (ld_text_base+ld_text_size)
	 && cmd == Read)
	/* data access within bounds */
	|| (addr >= ld_data_base && addr < ld_stack_base)))
    fatal("access error: segmentation violation, addr 0x%08p", addr);

  /* track the minimum SP for memory access stats */
  if (addr > mem_brk_point && addr < mem_stack_min)
    mem_stack_min = addr;

/* determines if the memory access is valid, returns error str or NULL */
char *					/* error string, or NULL */
mem_valid( mem_t *mem,		/* memory space to probe */
	  enum mem_cmd cmd,		/* Read (from sim'ed mem) or Write */
	  md_addr_t addr,		/* target address to access */
	  int nbytes,			/* number of bytes to access */
	  int declare);			/* declare any detected error? */

/* determines if the memory access is valid, returns error str or NULL */
char *					/* error string, or NULL */
mem_valid(enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  SS_ADDR_TYPE addr,		/* target address to access */
	  int nbytes,			/* number of bytes to access */
	  int declare)			/* declare the error if detected? */
{
  char *err_str = NULL;

  /* check alignments */
  if ((nbytes & (nbytes-1)) != 0 || (addr & (nbytes-1)) != 0)
    {
      err_str = "bad size or alignment";
    }
  /* check permissions, no probes allowed into undefined segment regions */
  else if (!(/* text access and a read */
	   (addr >= ld_text_base && addr < (ld_text_base+ld_text_size)
	    && cmd == Read)
	   /* data access within bounds */
	   || (addr >= ld_data_base && addr < ld_stack_base)))
    {
      err_str = "segmentation violation";
    }

  /* track the minimum SP for memory access stats */
  if (addr > mem_brk_point && addr < mem_stack_min)
    mem_stack_min = addr;

  if (!declare)
    return err_str;
  else if (err_str != NULL)
    fatal(err_str);
  else /* no error */
    return NULL;
}

/* initialize memory system, call after loader.c */
void
mem_init1(void)
{

  /* initialize the bottom of heap to top of data segment */
  mem_brk_point = ROUND_UP(ld_data_base + ld_data_size, SS_PAGE_SIZE);

  /* set initial minimum stack pointer value to initial stack value */
  mem_stack_min = regs_R[SS_STACK_REGNO];
}

#endif
