// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, 2009, 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "smpProc.h"
#include "ssb_cache.h"
#include "ssb.h"

smpProc::smpProc(string cfgstr, smpMemory *sm, component *mc, genericNetwork *net, int sysNum, prefetchMC *pMC) 
  : mainProc(cfgstr,mc, net, sysNum, sm->getBaseMem(), pMC), sharedMem(sm) {
  static bool inited = 0;
  if (!inited) {
    inited = 1;
    requestFullMemCopy(sharedMem);
  }

  sm->registerProcessor(this, this);
  invalidates[0] = invalidates[1] = 0;
  busMemAccess = 0;
}

//: Process bus write hit message
//
// We implement the H&P algorithm, so a write hit is the same as a
// write miss.
void smpProc::busWriteHit(simAddress sa) {busWriteMiss(sa);}

void smpProc::writeBackBlock(simAddress sa) {
  if (sa) {
    parcel *p = parcel::newParcel();
    p->inst() = 0;
    p->data() = (void*)sa;
#warning this should be the cache line length in bits...
    p->sizeBits() = ssb::memReqSizeBits;
    sendParcel(p, memCtrl, TimeStamp()+1);
  }
}

void smpProc::busReadMiss(simAddress sa) {
  //printf("%d: saw read miss for %x\n", getMainProcID(), sa);
  if (sa) {
    tagMap::iterator it = blkTags.find(sa);
    if ((it != blkTags.end()) && (it->second == EXCLUSIVE)) {
      //printf("exclu->shared %x\n", sa);
      // we should have it exclusively
      // write back block
      writeBackBlock(sa);
      // change to shared
      it->second = SHARED;
    }
  }
}

// perform write invalidate
void smpProc::busWriteMiss(simAddress sa) {
  //printf("%d: erasing tag for %x\n", getMainProcID(), sa);
  if (sa) {
    // remove the tag
    blkTags.erase(sa);
    // invalidate from cache(s)
    if (cache_dl1) {
      bool writtenBack = 0;
      if (cache_probe(cache_dl1, sa)) {
	writeBackBlock(sa);
	writtenBack = 1;
	invalidates[0]++;
	//printf(" inv l1\n");
	cache_invalidate_addr(cache_dl1, sa, TimeStamp());
      }
      if (cache_dl2) {
	if (cache_probe(cache_dl2, sa)) {
	  if (writtenBack == 0) {writeBackBlock(sa);}
	  invalidates[1]++;
	  cache_invalidate_addr(cache_dl2, sa, TimeStamp());
	}
      }
    }
  }
}

void smpProc::setup() {
  mainProc::setup();
}

void smpProc::preTic() {
  if (thr) {
    mainProc::preTic();
  }
}

void smpProc::finish() {
  mainProc::finish();
  for (int i = 0; i < 2; ++i) 
    printf("%llu L%d Invalidations\n", invalidates[i], i+1);
  printf("%llu busMemAccess\n", busMemAccess);
}

void smpProc::handleParcel(parcel *p) {
  thread *newT = p->travThread();
  if (newT != 0) {
    newT->assimilate(this);
    if (thr == 0) {
      thr = newT;
      fetch_pred_PC = thr->getStartPC();
      // This means that a thread was successfully spawned to a processor
      INFO("proc %d got thread PC:%x\n", mainProcID, (uint)fetch_pred_PC);
    } else {
      WARN("trying to replace a thread in smpProc\n");
    }
    parcel::deleteParcel(p);
  } else {
    mainProc::handleParcel(p);
  }
}

bool smpProc::spawnToCoProc(const PIM_coProc where, thread* t, 
			    simRegister hint) {
  // to do the "hetero" experiements, we treat PIM_ANY_PIM like SMPPROC
  if (where != PIM_SMPPROC && where != PIM_ANY_PIM) {
    return mainProc::spawnToCoProc(where, t, hint);
  }

  const smpMemory::smpVec &smps = sharedMem->getSMPs();
  for (uint i = 0; i < smps.size(); ++i) {
    smpProc *proc = smps[i];
    if ((proc != 0) && (proc->thr == 0)) {
      parcel *p = parcel::newParcel();
      p->travThread() = t;
      sendParcel(p, proc, TimeStamp()+1);      
      return 1;
    }
  }

  ERROR("smpProc couldn't find free processor! (where:%i max:%i)\n", where, smps.size());
  return 0;
}

void smpProc::noteWrite(simAddress a) {
  if (a) {
    //place write miss on bus
    md_addr_t baddr = a;
    if (cache_dl2) baddr = cache_get_blkAddr(cache_dl2, a);
    else if (cache_dl1) baddr = cache_get_blkAddr(cache_dl1, a);
    blkTag &bTag = blkTags[baddr];
    if (bTag != EXCLUSIVE) {
      //printf("%d: note write %x\n", getMainProcID(), baddr);
      sharedMem->postMessage(sharedMemory::WRITE_MISS, baddr, this);
      blkTags[baddr] = EXCLUSIVE; //change tag to Exclusive    
    }
  }
}

unsigned int smpProc::cplx_mem_access_latency(const enum mem_cmd cmd,
					      const md_addr_t baddr,
					      const int bsize,
					      bool &needMM) {
  blkTag &bTag = blkTags[baddr];
  //printf("%d: smp cplx mem access baddr:%x(tag %d) cmd: %d\n", getMainProcID(), baddr, bTag, cmd);

  busMemAccess++;
  unsigned int ret = convProc::cplx_mem_access_latency(cmd, baddr, bsize, 
						       needMM);

  if (cmd == Read) { // read miss
    //place read miss on bus
    sharedMem->postMessage(sharedMemory::READ_MISS, baddr, this);
    bTag = SHARED; //change tag to SHARED    
  } else { // write miss, but taken care of in noteWrite()
    //place write miss on bus
    //sharedMem->postMessage(sharedMemory::WRITE_MISS, baddr, this);
    //bTag = EXCLUSIVE; //change tag to Exclusive    
  }

  return ret;
}

uint8 smpProc::getFE(const simAddress sa) {
  return sharedMem->getFE(sa);
}

void smpProc::setFE(const simAddress sa, const uint8 FEValue) {
  sharedMem->setFE(sa, FEValue);
}

void smpProc::squashSpec() {
  mainProc::squashSpec();
}

#define SMP_MEM_WRAP_GEN(S)						\
  uint##S smpProc::ReadMemory##S(const simAddress sa, const bool s) {   \
    if (s) {								\
      return mainProc::ReadMemory##S(sa, s);				\
    } else {								\
      return sharedMem->ReadMemory##S(sa, s);				\
    }									\
  }									\
  bool smpProc::WriteMemory##S(const simAddress sa, const uint##S d, const bool s) { \
    if (s) {								\
      return mainProc::WriteMemory##S(sa, d, s);			\
    } else {								\
      return sharedMem->WriteMemory##S(sa, d, s);			\
    }									\
  }

SMP_MEM_WRAP_GEN(8)
SMP_MEM_WRAP_GEN(16)
SMP_MEM_WRAP_GEN(32)
