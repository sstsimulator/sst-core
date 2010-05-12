/*
 * cache.h - cache module interfaces
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
 * $Id: ssb_cache.h,v 1.2 2006-08-24 19:05:27 afrodri Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2006/01/31 16:35:49  afrodri
 * first entry
 *
 * Revision 1.5  2004/11/16 04:17:46  arodrig6
 * added more documentation
 *
 * Revision 1.4  2004/10/27 21:03:39  arodrig6
 * added SMP support
 *
 * Revision 1.3  2004/08/20 01:17:50  arodrig6
 * began integrating SW2/DRAM with ssBack
 *
 * Revision 1.2  2004/08/10 22:55:35  arodrig6
 * works, except for the speculative stuff, caches, and multiple procs
 *
 * Revision 1.1  2004/08/05 23:51:44  arodrig6
 * grabed files from SS and broke up some of them
 *
 * Revision 1.1.1.1  2000/03/07 05:15:16  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:49  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.5  1998/08/27 08:09:13  taustin
 * implemented host interface description in host.h
 * added target interface support
 *
 * Revision 1.4  1997/03/11  01:09:45  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 *
 * Revision 1.3  1997/01/06  15:57:55  taustin
 * comments updated
 * cache_reg_stats() now works with stats package
 * cp->writebacks stat added to cache
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>

#include "ssb_host.h"
#include "ssb_misc.h"
#include "ssb_machine.h"
#include "ssb_memory.h"
#include "ssb_stats.h"


/* highly associative caches are implemented using a hash table lookup to
   speed block access, this macro decides if a cache is "highly associative" */
#define CACHE_HIGHLY_ASSOC(cp)	((cp)->assoc > 4)

//: cache replacement policy 
//!SEC:ssBack
enum cache_policy {
  LRU,		/* replace least recently used block (perfect LRU) */
  Random,	/* replace a random block */
  FIFO		/* replace the oldest block in the set */
};

/* block status values */
#define CACHE_BLK_VALID		0x00000001	/* block in valid, in use */
#define CACHE_BLK_DIRTY		0x00000002	/* dirty block */

//: cache block (or line) definition
//!SEC:ssBack
struct cache_blk_t
{
  struct cache_blk_t *way_next;	/* next block in the ordered way chain, used
				   to order blocks for replacement */
  struct cache_blk_t *way_prev;	/* previous block in the order way chain */
  struct cache_blk_t *hash_next;/* next block in the hash bucket chain, only
				   used in highly-associative caches */
  /* since hash table lists are typically small, there is no previous
     pointer, deletion requires a trip through the hash table bucket list */
  md_addr_t tag;		/* data block tag value */
  unsigned int status;		/* block status, see CACHE_BLK_* defs above */
  tick_t ready;		/* time when block will be accessible, field
				   is set when a miss fetch is initiated */
  byte_t *user_data;		/* pointer to user defined data, e.g.,
				   pre-decode data or physical page address */
  /* DATA should be pointer-aligned due to preceeding field */
  /* NOTE: this is a variable-size tail array, this must be the LAST field
     defined in this structure! */
  byte_t data[1];		/* actual data block starts here, block size
				   should probably be a multiple of 8 */
};

//: cache set definition 
// cache set definition (one or more blocks sharing the same set index) 
//!SEC:ssBack
struct cache_set_t
{
  struct cache_blk_t **hash;	/* hash table: for fast access w/assoc, NULL
				   for low-assoc caches */
  struct cache_blk_t *way_head;	/* head of way list */
  struct cache_blk_t *way_tail;	/* tail pf way list */
  struct cache_blk_t *blks;	/* cache blocks, allocated sequentially, so
				   this pointer can also be used for random
				   access to cache blocks */
};

class convProc;

//: cache definition 
//
// This module contains code to implement various cache-like
// structures. The user instantiates caches using cache_new(). When
// instantiated, the user may specify the geometry of the cache (i.e.,
// number of set, line size, associativity), and supply a block access
// function. The block access function indicates the latency to access
// lines when the cache misses, accounting for any component of miss
// latency, e.g., bus acquire latency, bus transfer latency, memory
// access latency, etc... In addition, the user may allocate the cache
// with or without lines allocated in the cache. Caches without tags
// are useful when implementing structures that map data other than
// the address space, e.g., TLBs which map the virtual address space
// to physical page address, or BTBs which map text addresses to
// branch prediction state. Tags are always allocated. User data may
// also be optionally attached to cache lines, this space is useful to
// storing auxilliary or additional cache line information, such as
// predecode data, physical page address information, etc...
//
// The caches implemented by this module provide efficient storage
// management and fast access for all cache geometries. When sets
// become highly associative, a hash table (indexed by address) is
// allocated for each set in the cache.
//
// This module also tracks latency of accessing the data cache, each
// cache has a hit latency defined when instantiated, miss latency is
// returned by the cache's block access function, the caches may
// service any number of hits under any number of misses, the calling
// simulator should limit the number of outstanding misses or the
// number of hits under misses as per the limitations of the
// particular microarchitecture being simulated.
//
// Due to the organization of this cache implementation, the latency
// of a request cannot be affected by a later request to this
// module. As a result, reordering of requests in the memory hierarchy
// is not possible.
//
//!SEC:ssBack
struct cache_t
{
  /* parameters */
  char *name;			/* cache name */
  convProc *proc;
  int nsets;			/* number of sets */
  int bsize;			/* block size in bytes */
  int balloc;			/* maintain cache contents? */
  int usize;			/* user allocated data size */
  int assoc;			/* cache associativity */
  enum cache_policy policy;	/* cache replacement policy */
  unsigned int hit_latency;	/* cache hit latency */

  /* miss/replacement handler, read/write BSIZE bytes starting at BADDR
     from/into cache block BLK, returns the latency of the operation
     if initiated at NOW, returned latencies indicate how long it takes
     for the cache access to continue (e.g., fill a write buffer), the
     miss/repl functions are required to track how this operation will
     effect the latency of later operations (e.g., write buffer fills),
     if !BALLOC, then just return the latency; BLK_ACCESS_FN is also
     responsible for generating any user data and incorporating the latency
     of that operation */
  unsigned int					/* latency of block access */
  (convProc::*blk_access_fn)(enum mem_cmd cmd,	/* block access command */
			     md_addr_t baddr,	/* program address to access */
			     int bsize,		/* size of the cache block */
			     cache_blk_t *blk,	/* ptr to cache block struct */
			     tick_t now,	/* when fetch was initiated */
			     bool &needMM);

  /* derived data, for fast decoding */
  int hsize;			/* cache set hash table size */
  md_addr_t blk_mask;
  int set_shift;
  md_addr_t set_mask;		/* use *after* shift */
  int tag_shift;
  md_addr_t tag_mask;		/* use *after* shift */
  md_addr_t tagset_mask;	/* used for fast hit detection */

  /* bus resource */
  tick_t bus_free;		/* time when bus to next level of cache is
				   free, NOTE: the bus model assumes only a
				   single, fully-pipelined port to the next
 				   level of memory that requires the bus only
 				   one cycle for cache line transfer (the
 				   latency of the access to the lower level
 				   may be more than one cycle, as specified
 				   by the miss handler */

  /* per-cache stats */
  counter_t hits;		/* total number of hits */
  counter_t misses;		/* total number of misses */
  counter_t replacements;	/* total number of replacements at misses */
  counter_t writebacks;		/* total number of writebacks at misses */
  counter_t invalidations;	/* total number of external invalidations */

  /* last block to hit, used to optimize cache hit processing */
  md_addr_t last_tagset;	/* tag of last line accessed */
  struct cache_blk_t *last_blk;	/* cache block last accessed */

  /* data blocks */
  byte_t *data;			/* pointer to data blocks allocation */

  /* NOTE: this is a variable-size tail array, this must be the LAST field
     defined in this structure! */
  struct cache_set_t sets[1];	/* each entry is a set */
};

//: create and initialize a general cache structure 
struct cache_t *			/* pointer to cache created */
cache_create(char *name,		/* name of the cache */
	     convProc *pr,
	     int nsets,			/* total number of sets in cache */
	     int bsize,			/* block (line) size of cache */
	     int balloc,		/* allocate data space for blocks? */
	     int usize,			/* size of user data to alloc w/blks */
	     int assoc,			/* associativity of cache */
	     enum cache_policy policy,	/* replacement policy w/in sets */
	     /* block access function, see description w/in struct cache def */
	     unsigned int (convProc::*blk_access_fn)(enum mem_cmd cmd,
					   md_addr_t baddr, int bsize,
					   struct cache_blk_t *blk,
						     tick_t now, bool&),
	     unsigned int hit_latency);/* latency in cycles for a hit */

//: parse policy 
enum cache_policy			/* replacement policy enum */
cache_char2policy(char c);		/* replacement policy as a char */

//: print cache configuration 
void
cache_config(struct cache_t *cp,	/* cache instance */
	     FILE *stream);		/* output stream */

//: register cache stats 
void
cache_reg_stats(struct cache_t *cp,	/* cache instance */
		struct stat_sdb_t *sdb);/* stats database */

//: print cache stats 
void
cache_stats(struct cache_t *cp,		/* cache instance */
	    FILE *stream);		/* output stream */

//: print cache stats 
void cache_stats(struct cache_t *cp, FILE *stream);

//: Access a cache
//
// access a cache, perform a CMD operation on cache CP at address
// ADDR, places NBYTES of data at *P, returns latency of operation if
// initiated at NOW, places pointer to block user data in *UDATA, *P
// is untouched if cache blocks are not allocated (!CP->BALLOC), UDATA
// should be NULL if no user data is attached to blocks
unsigned int				/* latency of access in cycles */
cache_access(struct cache_t *cp,	/* cache to access */
	     enum mem_cmd cmd,		/* access type, Read or Write */
	     md_addr_t addr,		/* address of access */
	     void *vp,			/* ptr to buffer for input/output */
	     int nbytes,		/* number of bytes to access */
	     tick_t now,		/* time of access */
	     byte_t **udata,		/* for return of user data ptr */
	     md_addr_t *repl_addr,	/* for address of replaced block */
	     bool &needMM,              /* does this need a main me access */
	     md_addr_t *bumped);        /* Was an address ejected from cache */

/* cache access functions, these are safe, they check alignment and
   permissions */
/*#define cache_double(cp, cmd, addr, p, now, udata)		\
  cache_access(cp, cmd, addr, p, sizeof(double), now, udata)
#define cache_float(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(float), now, udata)
#define cache_dword(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(long long), now, udata)
#define cache_word(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(int), now, udata)
#define cache_half(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(short), now, udata)
#define cache_byte(cp, cmd, addr, p, now, udata)	\
cache_access(cp, cmd, addr, p, sizeof(char), now, udata)*/

/* return non-zero if block containing address ADDR is contained in cache
   CP, this interface is used primarily for debugging and asserting cache
   invariants */
int					/* non-zero if access would hit */
cache_probe(struct cache_t *cp,		/* cache instance to probe */
	    md_addr_t addr);		/* address of block to probe */

/* flush the entire cache, returns latency of the operation */
unsigned int				/* latency of the flush operation */
cache_flush(struct cache_t *cp,		/* cache instance to flush */
	    tick_t now);		/* time of cache flush */

/* flush the block containing ADDR from the cache CP, returns the latency of
   the block flush operation */
unsigned int				/* latency of flush operation */
cache_flush_addr(struct cache_t *cp,	/* cache instance to flush */
		 md_addr_t addr,	/* address of block to flush */
		 tick_t now);		/* time of cache flush */
/* invalidate the block containing ADDR from the cache CP, returns the
   latency of the operation. does not flush to backing store */
unsigned int				/* latency of flush operation */
cache_invalidate_addr(struct cache_t *cp,	/* cache instance to flush */
		      md_addr_t addr,	/* address of block to flush */
		      tick_t now);		/* time of cache flush */

md_addr_t cache_get_blkAddr(struct cache_t *cp, md_addr_t addr);
#endif /* CACHE_H */
