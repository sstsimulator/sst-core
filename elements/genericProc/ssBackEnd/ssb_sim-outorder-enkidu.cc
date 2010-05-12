/* Copyright 2010 Sandia Corporation. Under the terms
 of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
 Government retains certain rights in this software.
 
 Copyright (c) 2007-2010, Sandia Corporation
 All rights reserved.
 
 This file is part of the SST software package. For license
 information, see the LICENSE file in the top level directory of the
 distribution. */


#include "ssb_sim-outorder.h"
#include "ssb_cache.h"
#include "FE/fe_debug.h"
#include "FE/thread.h"
#include <sim_trace.h>

//: Print stats
void convProc::finish(){
  printf("Processor stopped at TimeStamp %llu\n", TimeStamp());
  sim_print_stats(stdout);

#if WANT_LSQ_HIST == 1
  printf("LSQ Historgram:\n");
  for(map<int, counter_t>::iterator i = LSQ_hist.begin();
      i != LSQ_hist.end(); ++i) {
    printf("%d: %llu\n", i->first, (long long unsigned)i->second);
  }
#endif
  
#if GET_IMIX == 1
  printf("      iMix\n");
  for (int i = 0; i < LASTINST; ++i) {
    printf("  %8s:", instNames[i]);
    printf(" %10llu ", iMix[i]);
    printf("\n");
  }
#endif
  printf("%llu LSQ Compares\n", lsqCompares);

  if (thr) delete thr;
  if (pref) {
    pref->finish();
    delete pref;
  }
}

//: wakup load which was filtered by prefetcher
void convProc::wakeUpPrefetched(instruction *inst) {
  //printf("woke up for %p %llu\n", (void*)inst->memEA(), Cycle());
  map<instruction*, RUU_station*>::iterator mi;
  if ((mi = mainMemLoads.find(inst)) != mainMemLoads.end()) {
    wakeUpMM(mi);
  } else {
    printf("prefetcher trying to wakeup an instruction %p which isn't sleeping\n", inst);
  }
}

//: wake a load to main mem up
//
// Wake a load instruction which in the mainMemLoads.
void convProc::wakeUpMM(map<instruction*, RUU_station*>::iterator &mi) {
  eventq_queue_event(mi->second, TimeStamp() + 1);
  mainMemLoads.erase(mi->first);  
}

//: Handle returning memory refs
//
// As memory references are returned, handle them.
//
// Instruction Fetches unblock the iFetchBlocker and are squashed
//
// Stores are squashed
// 
// Loads are queued up in the eventq to be finalized.
void convProc::handleMemEvent( instruction* inst ) {

  // check with prefetcher
  if (pref) {
    printf("fix %s %d\n", __FILE__, __LINE__);
    bool wasP = 0;
    //bool wasP = pref->handleParcel(e);
    if (wasP) return;
  }

  /*printf("convProc got mem event 0x%x (type=%d, tag=%x)\n", e->address,
    e->type, e->tag);*/
  set<instruction*>::iterator si;
  map<instruction*, RUU_station*>::iterator mi;
    if (inst) {
      simAddress mem=0;
      bool isInst = 0;
      if ((inst) == (instruction*)(~0)) {
	//printf(" recived writeback\n");
      } else if (inst == iFetchBlocker) {
	//printf(" removing iFetchBlocker\n");
	mem = ntohl(inst->PC());
	isInst = 1;
	thr->squash(inst);
	iFetchBlocker = 0;	
      } else if ((si = mainMemStores.find(inst)) != mainMemStores.end()) {
	//printf(" found store\n");
	mem = inst->memEA();
	handleReturningStore(inst);
      } else if ((mi = mainMemLoads.find(inst)) != mainMemLoads.end()) {
	mem = inst->memEA();
	wakeUpMM(mi);
      } else if (condemnedRemotes.find(inst) != condemnedRemotes.end()) {
	thr->squash(inst);
	condemnedRemotes.erase(inst);
      } else {
	printf("got unknown memory instruction in %s\n", __FILE__);
      }

      // insert into cache(s)
      //if ( addrCached(mem&~3) ) {
      if ( 1 ) {
        cache_t *cs[2][2] = {{cache_dl1, cache_dl2},{cache_il1, cache_il2}};
        for (int i = 0; i < 2; ++i) {
	  if(cs[isInst][i]) {
	    bool dc;
	    md_addr_t bumped=0;
	    cache_access(cs[isInst][i], Inject, (mem&~3), NULL, 0, TimeStamp(), 
		       NULL, NULL, dc, &bumped);
	    if ((i == 1) && bumped && pref) pref->reportCacheEject(bumped);
          }
	}
      }
#ifdef __USE_ME__ 
      else {
            SIM_trace(Cycle(), getProcNum()/2,
				getProcNum()%2 ? "PPC" : "Opteron", 
				SIM_FUNC_ANY, SIM_ANY, mem );
      }
#endif
    } else {
#if 0
      WARN("conv proc got parcel from mem w/o inst EA=%#x\n",
	   (simAddress)(size_t)e->address);
#endif
    }
}

//: Retire buffered instructions on retire List
//
// When a store returns, retire everything that we can. We retire in
// order everything up to and including any completed store. We stop
// retiring if we hit an uncompleted store. Store which arrive out of
// order are recorded in the OOOStores set.
void convProc::handleReturningStore(instruction *inst) {
  //: iSet iterator
  // iset iter
  typedef set<instruction*>::iterator iSIter;
  OOOStores.insert(inst);
  while(!retireList.empty()) {
    instruction *fInst = retireList.front();
    if (mainMemStores.find(fInst) != mainMemStores.end()) {
      if (OOOStores.find(fInst) != OOOStores.end()) {
	//printf("got store back OOO match %p %d %d\n", fInst, OOOStores.size(), retireList.size());
	thr->retire(fInst);
	OOOStores.erase(fInst);
	mainMemStores.erase(fInst);
	retireList.pop_front();
      } else {
	break;
      }
    } else {
      // regular instruction. retire & pop
      thr->retire(fInst);
      retireList.pop_front();
    }
  }
}
