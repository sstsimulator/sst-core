// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <assert.h>
#include "ssb.h"
#include "ssb_sim-outorder.h"
#include "ssb_cache.h"

/* speculative memory hash table address hash function */
#define HASH_ADDR(ADDR)							\
  ((((ADDR) >> 24)^((ADDR) >> 16)^((ADDR) >> 8)^(ADDR)) & (STORE_HASH_SIZE-1))


//: Simple memory access
//
// returns total latency of access 
unsigned int		
convProc::mem_access_latency(int blk_sz) /* block size accessed */
{ 
  int chunks = (blk_sz + (mem_bus_width - 1)) / mem_bus_width;
  
  assert(chunks > 0);

  return (/* first chunk latency */mem_lat[0] +
	  (/* remainder chunk latency */mem_lat[1] * (chunks - 1)));
}

/*
 * cache miss handlers
 */

//: l1 data cache miss handler 
unsigned int			/* latency of block access */
convProc::dl1_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
			md_addr_t baddr,	/* block address to access */
			int bsize,		/* size of block to access */
			 cache_blk_t *blk,/* ptr to block in
						   upper level */
			tick_t now, 		/* time of access */
			bool& needMM)
{
  unsigned int lat;

  if (cache_dl2)
    {
      /* access next level of data cache hierarchy */
      md_addr_t bumped = 0;
      lat = cache_access(cache_dl2, cmd, baddr, NULL, bsize,
			 /* now */now, /* pudata */NULL, 
			 /* repl addr */NULL,
			 needMM, &bumped);
      if (bumped != 0) {
	if (pref) pref->reportCacheEject(bumped);
      }
      if (cmd == Read)
	return lat;
      else
	{
	  /* FIXME: unlimited write buffers */
	  return 0;
	}
    }
  else
    {
      /* access main memory */
      if (simpleMemory) {	
	if (cmd == Read)
	  return mem_access_latency(bsize);
	else {
	  /* FIXME: unlimited write buffers */
	  return 0;
	}
      } else {
	return cplx_mem_access_latency(cmd, baddr, bsize, needMM);
      }
    }
}

//: l2 data cache miss handler
unsigned int			/* latency of block access */
convProc::dl2_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	       cache_blk_t *blk,	/* ptr to block in upper level */
			tick_t now,		/* time of access */
			bool& needMM)
{
  if (simpleMemory) {
    /* this is a miss to the lowest level, so access main memory */
    if (cmd == Read)
      return mem_access_latency(bsize);
    else {
      /* FIXME: unlimited write buffers */
      return 0;
    }
  } else {
    return cplx_mem_access_latency(cmd, baddr, bsize, needMM);
  }
}

//: l1 inst cache miss handler function
unsigned int			/* latency of block access */
convProc::il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	       cache_blk_t *blk,	/* ptr to block in upper level */
			tick_t now,		/* time of access */
			bool& needMM)
{
  unsigned int lat;

if (cache_il2)
    {
      /* access next level of inst cache hierarchy */
      lat = cache_access(cache_il2, cmd, baddr, NULL, bsize,
			 /* now */now, /* pudata */NULL, /* repl addr */NULL,
			 needMM, NULL);
      if (cmd == Read)
	return lat;
      else
	panic("writes to instruction memory not supported");
    }
  else
    {
      /* access main memory */
      if (simpleMemory) {
	if (cmd == Read)
	  return mem_access_latency(bsize);
	else
	  panic("writes to instruction memory not supported");
      } else {
	return cplx_mem_access_latency(cmd, baddr, bsize, needMM);
      }
    }
}

//: l2 inst cache miss handler
unsigned int			/* latency of block access */
convProc::il2_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	       cache_blk_t *blk,	/* ptr to block in upper level */
			tick_t now,		/* time of access */
			bool &needMM)
{
  /* this is a miss to the lowest level, so access main memory */
  if (simpleMemory) {
    if (cmd == Read)
      return mem_access_latency(bsize);
    else
      panic("writes to instruction memory not supported");
  } else {
    return cplx_mem_access_latency(cmd, baddr, bsize, needMM);
  }
}


/*
 * TLB miss handlers
 */

//: iTLB miss handler
// 
// note: does not access main memory
unsigned int			/* latency of block access */
convProc::itlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       md_addr_t baddr,		/* block address to access */
	       int bsize,		/* size of block to access */
	        cache_blk_t *blk,	/* ptr to block in upper level */
			 tick_t now,		/* time of access */
			 bool &needMM)
{
  md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  /* return tlb miss latency */
  return tlb_miss_lat;
}

//:dTLB miss handler
// 
// note: does not access main memory
unsigned int			/* latency of block access */
convProc::dtlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       md_addr_t baddr,	/* block address to access */
	       int bsize,		/* size of block to access */
	        cache_blk_t *blk,	/* ptr to block in upper level */
			 tick_t now,		/* time of access */
			 bool &needMM)
{
  md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  /* return tlb miss latency */
  return tlb_miss_lat;
}

//: Create send an instruction to main memoy
void convProc::mmSendParcel(instruction *inst) {
  instType iType = LOAD;
  uint64_t address = 0;

  if (inst->state() <= FETCHED) {
    address = ntohl(inst->PC());
  } else {
    iType = inst->op();
    
    address = (uint64_t)inst->memEA();
  }

  // All instructions are loading into a cache, so they are all LOADS,
  // even store instructions, load into the cache. The only time the
  // memory controller sees a 'store' is on a writeback.
  iType = LOAD;

  myProc->sendMemoryReq( iType, address, inst, myCoreID);
}

//: Send an instruction to main memory
void convProc::mainMemAccess(instruction* inst) {
  //printf("%s!\n", __FUNCTION__);

  if (pref) {
    if ((inst->op() == LOAD) && pref->isPreFetching(inst->memEA())) {
      //printf("filtered req for %p from %p %d\n", (void*)inst->memEA(), inst, inst->op());
      pref->setWakeUp(inst, inst->memEA());
      return;
    }
  }
  mmSendParcel(inst);
}

//: Check off chip access
//
// sends writebacks and informs the caller of a main memory access is
// required.
unsigned int convProc::cplx_mem_access_latency(const enum mem_cmd cmd,
					       const md_addr_t baddr,
					       const int bsize,
					       bool &needMM) {
  if (cmd == Read) {
    needMM = 1;
  } else { // writeback, so send as such
    //printf("sent writeback\n");
    needMM = 0;
    myProc->sendMemoryReq( STORE, baddr, (instruction*)~0, myCoreID);
    if (pref) pref->reportCacheEject(baddr);
  }
  return 1;
}
