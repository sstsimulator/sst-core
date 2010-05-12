// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "global.h"
#include "prefetch.h"
//#include "configuration.h"
#include "ssb_DMA_fakeInst.h"
#include "FE/fe_debug.h"

#warning this should be a config var
const int cacheShift = 6;

//: Pool of Fake insrtuctions 
pool<fakeDMAInstruction> prefetcher::fakeInst;

//: Constructor
prefetcher::prefetcher(string nm, prefetchProc *p, prefetchMC *_mc) : 
  prename(nm), proc(p), requestsIssued(0), requestsHit(0),
  overPage(0), tooLate(0), adaptions(0), subTotalReq(0),
  subRequestsHit(0), streamReq(0), streamsDetected(0), mc(_mc), streamRequestsHit(0), rr(0) {
#if 1
  ERROR("Prefetcher not supported");
#else  

  inWin = 0;

  if (mc == 0) {
    WARN("Prefetch: memory controller is _not_ prefetch aware\n");
  }

  stats = configuration::getValue(prename+":stats");
  if (stats <= 0) stats = 0;
  else stats = 1;

  string pretype = configuration::getStrValue(prename);
  if (pretype == "obl" || pretype == "obls") {
    if (configuration::getValue(":simpleMemory") == 1) {
      printf("Prefetcher not supported with simpleMemory\n");
      exit(-1);
    }

    if (pretype == "obls") {
      printf("Using Stream Prefetcher.\n");
      streams = configuration::getValue(prename+":stream:streams");
      windowL = configuration::getValue(prename+":stream:window");
      detLeng = configuration::getValue(prename+":stream:length");
      if (streams <= 0 || detLeng < 2 || windowL <= detLeng) {
	printf("stream prefetcher misconfigured\n");
	exit(-1);
      } else {
	printf("  streams %d\n  window %d\n  stream length %d\n", streams, 
	       windowL, detLeng);
      }
    } else {
      streams = 0;
    }

    printf("Using OBL (One Block Lookahead) Prefetcher.\n");
    tagged = configuration::getValue(prename+":obl:tagged");
    degree = configuration::getValue(prename+":obl:degree");
    pageShift = configuration::getValue(prename+":pageShift");
    adaptive = configuration::getValue(prename+":obl:adaptive");
    loadAware = configuration::getValue(prename+":loadAware");
    if (loadAware >= 1) {loadAware = 1; printf("  load aware\n");}
    else {loadAware = 0;}
    if (tagged >= 1) {tagged = 1; printf("  tagged\n");}
    else {tagged = 0; printf("  untagged\n");}
    if (degree >= 0) {printf("  degree %d\n", degree);}
    else {degree = 1; printf("  degree 1\n");}
    if (adaptive >= 1) {
      adaptive = 1; stats=1;printf("  adaptive %d\n", adaptive);
    }
    else {adaptive = 0; printf("  adaptive 0\n");}
    if (pageShift >= 1) {printf("  pageShift %d\n", pageShift);}
    else {pageShift = 12; printf("  pageShift 12 (default)\n");}

    if (adaptive) {
      int q = configuration::getValue(prename+":obl:adaptive:quantaPow2");
      if (q < 1) {
	printf("adaptive prefetcher quanta invalid\n"); exit(-1);
      } else {
	adaptQuantaMask = 0-1;
	adaptQuantaMask >>= q;
	adaptQuantaMask <<= q;
	adaptQuantaMask = ~adaptQuantaMask;
	printf("  adapt quanta mask 0x%llx\n", adaptQuantaMask);
      }

      adaptMax = configuration::getValue(prename+":obl:adaptive:maxDegree");
      decDeg = configuration::getValue(prename+":obl:adaptive:decDeg100");
      incDeg = configuration::getValue(prename+":obl:adaptive:incDeg100");
      if (adaptMax < 2 || decDeg < 0 || incDeg < 0 || decDeg > incDeg) {
	printf("misconfigured adaptive OBL\n");
	exit(-1);
      }
    }
  } else {
    printf("%s prefetcher not supported\n", pretype.c_str());
    exit(-1);
  }
#endif
}

//: Detects if a given address is being prefetched
//
// aka. there is an active prefetch out for this address
bool prefetcher::isPreFetching(const simAddress sa) {
  simAddress ea = (sa >> cacheShift) << cacheShift;
  bool r = (addrs.find(ea) != addrs.end());
  //if (r) printf("already fetching %p\n", (void*)ea);
  return r;
}

//: record an instruction to wake up later
//
// this is for instruction which missed caches, but the prefetcher was
// already fetching. When the prefetch arrives we will need to tell
// the processor to restart this instruction.
void prefetcher::setWakeUp(instruction *inst, simAddress sa) {
  simAddress ea = (sa >> cacheShift) << cacheShift;
  wakeUpMap[ea].push_back(inst);
  //printf("setting wakeup for %p for %p\n", inst, (void*)ea);
}

void prefetcher::detectStream(const simAddress addr) {
  const simAddress ea = (addr >> cacheShift) << cacheShift;
  if (ea == lastBlock) return;
  lastBlock = ea;

  //printf("%x %x %llu\n", ea>>cacheShift, addr, component::Cycle());
  const simAddress eaCache = ea>>cacheShift;

  contigCount.insert(eaCache);
  bool found = 1;
  for (int i = 1; i < detLeng; ++i) {
    if (contigCount.find((eaCache) - i) == contigCount.end()) {
      found = 0;
      break;
    }
  }

  if (found && (streamSet.size() < uint(streams))) {    
    //printf(" %x %x\n", ea>>cacheShift, ea>>pageShift);

    bool good = 1;
    const simAddress eaPage = (ea>>pageShift);
    // check current streams
    for (set<simAddress>::iterator si = streamSet.begin();
	 si != streamSet.end(); ++si) {      
      if (((*si) << cacheShift) >> pageShift == eaPage) {
	good = 0;
	break;
      }
    }
    if (good) {
      // check recent streams
      for (deque<simAddress>::iterator ri = recentStreams.begin();
	   ri != recentStreams.end(); ++ri) {
	if (eaPage == ((*ri) << cacheShift) >> pageShift) {
	  good = 0;
	  //printf("already recent %x\n", eaPage);
	  break;
	}
      }

      if (good) {
	streamSet.insert(eaCache);
	streamsDetected++;
	//printf("  got stream %x %x %llu\n", ea>>cacheShift, ea>>pageShift, component::Cycle());
      }
    }

    for (int i = 1; i < detLeng; ++i) {
      contigCount.erase((eaCache) - i);
    }    
  }

  window.push(eaCache);
  inWin++;
  if (inWin > windowL) {
    inWin--;
    contigCount.erase(window.front());
    window.pop();
  }
}

void prefetcher::preTic() {
  if (streamSet.empty()) return;

  // find next stream to advance
  rr++;
  if (rr >= int(streamSet.size())) rr = 0;
  set<simAddress>::iterator ssi = streamSet.begin();
  for (int i = 0; i < rr; ++i) {ssi++;}
  
  // calculate next block
  simAddress nextBlock = ((*ssi)+1) << cacheShift;
  
  // check if we are over page
  if (nextBlock >> pageShift != ((*ssi)<<cacheShift)>>pageShift) {
    /*printf("%x overpage %x. finishing stream %d active %llu %llu\n", *ssi,
      (*ssi << cacheShift) >> pageShift,
      streamSet.size()-1, component::Cycle(), streamReq);*/
    streamSet.erase(ssi);
    recentStreams.push_front(*ssi);
    if (recentStreams.size() > uint(windowL)) recentStreams.pop_back();
    return;
  }
  
  // make the request
  bool loaded = 0;
  bool issued = memReq(nextBlock, loaded);
  if (issued) {
    if (stats) streamIssued.insert(nextBlock);
    streamReq++;
  }
  
  // advance stream marker, unless we were unable to isse the prefetch
  // because of load.
  if (!loaded) {
    streamSet.erase(ssi);  
    streamSet.insert(nextBlock>>cacheShift);
  }
}

bool prefetcher::memReq(const simAddress nextBlock, bool &loaded) {
  loaded = 0;
  bool already = proc->checkCache(nextBlock);
  // only request if its not in cache already, and we don't have an
  // outstanding request
  if (!already && (addrs.find(nextBlock) == addrs.end())) {
#if 0
    // safety check
    set<simAddress>::iterator i = reqInCache.find(nextBlock);
    if (i != reqInCache.end()) {	  
      printf("req %x in reqInCache\n", nextBlock);
    }
#endif
    
    // request from memory.    
    if (!mc || !loadAware || mc->load() >= 0) {
      fakeDMAInstruction *fake = fakeInst.getItem();
      fake->init(LOAD, nextBlock, 0);
      fakes.insert(fake);
      addrs.insert(nextBlock);
      //printf("pref request %x %llu inst %p\n", nextBlock, 
      //component::Cycle(), fake);
      requestsIssued++;
      proc->sendToMem(fake);
      return 1;
    } else {
      loaded = 1;
    }
  }
  return 0;
}


//: Inform Prefetcher of Memory Ref
//
// Should be called by the processor to inform the prefetcher that a
// memory reference has occured. The address (memEA), type, direction
// (dir), and if the reference "hit" the cache should be provided.
void prefetcher::memRef(const simAddress memEA, const memAccType type, 
			const memAccDir dir, bool hit) {
#if 1
  ERROR("Prefetcher not supported");
#else
  totalReq++; subTotalReq++;
  // check our hit rate
  if (stats) {
    if (reqInCache.find((memEA>>cacheShift)<<cacheShift) != reqInCache.end()) {
      requestsHit++; subRequestsHit++;
    } 
  }    

  if (stats && streams) {
    if (reqInSCache.find((memEA>>cacheShift)<<cacheShift) != 
	reqInSCache.end()) {
      streamRequestsHit++;
    }
  }

  // adapt if called for
  if (adaptive && (subTotalReq > 0) && 
      ((component::Cycle() & adaptQuantaMask) == 0)) {    
    int hitRate = int(double(subRequestsHit*100)/double(subTotalReq));
    //printf("%d\n", hitRate);

    if (hitRate < decDeg && degree > 1) {
      degree--; adaptions++;
      //printf("decrementing degree to %d\n", degree);
    } else if (hitRate > incDeg && degree < adaptMax) {
      degree++; adaptions++;
      //printf("incrementing degree to %d\n", degree);
    }

    subRequestsHit = subTotalReq = 0;
  }

  // detect streams
  if (!hit && (dir == READ) && (type==DATA) && streams > 0) detectStream(memEA);

  // non-tagged OBL only cares about misses
  if (!tagged && hit) return;

  // only prefetch reads (OBL)
  if (dir == READ && type == DATA) {
    for (int d = 0; d < degree; ++d) {
      simAddress nextBlock = ((memEA >> cacheShift) + d + 1) << cacheShift;
      if (memEA>>pageShift != nextBlock>>pageShift) {
	//out of page. ignore
	overPage++;
	break;
      } 
      
      // make the request
      bool dontCare;
      memReq(nextBlock, dontCare);
    }
  }

#if 0
  if ((component::cycle() & 0x3fff) == 0) {
    printf("%llu %d %.2f %.2f\n", component::cycle(), addrs.size(), double(component::cycle())/double(requestsIssued), double(totalReq)/double(requestsIssued));
  }
#endif
#endif
}

#if 0
//: Filter parcels for memory returns
//
// This should be called by the parent processor in its 'handleParcel'
// to screen if a given parcel is meant for the prefetcher. If it is,
// this function will handle the parcel, including deallocating it,
// and return 'true'. If not, the parcel will not be modified and the
// function will return 'false'.
bool prefetcher::handleParcel(parcel *p) {
  instruction *inst = p->inst();
  if (inst) {
    set<fakeDMAInstruction*>::iterator i = fakes.find((fakeDMAInstruction*)inst);
    if (i != fakes.end()) {
      // it is ours
      simAddress memEA = inst->memEA();

      // wake up anyone who needs it
      wakeUpMap_t::iterator wmi = wakeUpMap.find(memEA); 
      if (wmi != wakeUpMap.end()) {
	wakeUpList_t &wakeList = wmi->second;
	//printf("got back %p. waking people\n", (void*) memEA);
	for(wakeUpList_t::iterator wi = wakeList.begin();
	    wi != wakeList.end(); ++wi) {
	  proc->wakeUpPrefetched(*wi);
	  //printf(" woke %p\n", *wi);
	}
	wakeUpMap.erase(memEA);
      }

      //printf("insert %x %d\n", memEA, proc->checkCache(memEA));
      if (proc->checkCache(memEA) != 0) {
	//printf("inject %x already in cache\n", memEA);
	tooLate++;
      } else {
	proc->insertCache(memEA);
	if (stats && (streamIssued.find(memEA) != streamIssued.end())) {
	  streamIssued.erase(memEA);
	  reqInSCache.insert(memEA);
	}
	if (stats) reqInCache.insert(memEA);
      } 
      // clean up
      fakeInst.returnItem(*i);
      fakes.erase(i);
      set<simAddress>::iterator ai = addrs.find(memEA);
      if (ai != addrs.end()) addrs.erase(ai);
#if 0
      else printf(" addr not in addrs\n");
#endif
      parcel::deleteParcel(p);
      return true;
    }
  }
  return false;
}
#endif


//: Report ejection from Cache
//
// Called by processor to tell the prefetcher an address has been
// ejected from Cache.
void prefetcher::reportCacheEject(const simAddress memEA) {
  if (stats) {
    simAddress addr = (memEA>>cacheShift) << cacheShift;
    //printf("%x eject", addr);
    inCacheSet::iterator i = reqInCache.find(addr);
    if (i != reqInCache.end()) {
      reqInCache.erase(i);
      reqInSCache.erase(addr);
      //printf(" eject %x\n", addr);
    }
    //printf("\n");
  }
}

//: Finish
//
// print stats.
void prefetcher::finish() {
  printf("Prefetcher %s:\n", prename.c_str());
  printf("pre: requestsIssued: %llu\n", requestsIssued);
  if (streams) {
    printf("pre: stream Requests: %llu\n", streamReq);
    printf("pre: streeams Detected: %llu\n", streamsDetected);
  }
  printf("pre: requests not issued (overpage): %llu\n", overPage);
  printf("pre: requests too late: %llu\n", tooLate);
  if (stats) printf("pre: requests hit : %llu (%.2f%%)\n", requestsHit, double(requestsHit*100)/double(totalReq));
  if (streams) {
    if (stats) printf("pre: stream requests hit : %llu (%.2f%%)\n", streamRequestsHit, double(streamRequestsHit*100)/double(totalReq));
  }
  if (adaptive) {
    printf("pre: adaptions: %d\n", adaptions);
  }  
}
