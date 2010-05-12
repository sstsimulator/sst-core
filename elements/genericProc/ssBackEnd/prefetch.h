// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _PREFETCH_H_
#define _PREFETCH_H_

#include "sst_config.h"
#include <string>
#include <queue>
#include <set>
#include <map>
#ifdef HAVE_UNORDERED_SET
# include <unordered_set>
#elif defined(HAVE_TR1_UNORDERED_SET)
# include <tr1/unordered_set>
#else /* unordered_set */
# ifdef __GNUC__
#  if __GNUC__ < 3
#   include <hash_set.h>
namespace extension { using ::hash_set; }; // inherit globals
#  else
#   include <ext/hash_set>
//#   include <backward/hash_set>
#   if __GNUC__ == 3 && __GNUC_MINOR__ == 0
namespace extension = std; // GCC 3.0
#   else
namespace extension = ::__gnu_cxx; // GCC 3.1 and later
#   endif
#  endif
# else
#  include <hash_set>
# endif
#endif /* unordered_set */
#include <list>
#include "FE/pool.h"
#include "ssb_DMA_fakeInst.h"
#include "instruction.h"

using namespace std;

//: Interface for a prefetch-aware memory controller
class prefetchMC {
public:
  virtual ~prefetchMC() {;}
  //: Returns bandwidth load
  //
  // a positive number means low load, a negative means a backup.
  virtual int load()=0;
};

//: Interface for a prefetching processor
//
// This is what a prefetcher expects of its processor. 
class prefetchProc {  
public:
  virtual ~prefetchProc() {;}
  //: Check if a given address is in cache
  virtual bool checkCache(const simAddress)=0;
  //: insert a value to the cache
  virtual void insertCache(const simAddress)=0;
  //: Tell processor to send a memory request
  virtual void sendToMem(instruction *p)=0;
  //: Tell processor to 'wake up' an instruction 
  virtual void wakeUpPrefetched(instruction*)=0;
};

//: Prefetcher
//
// A semi-generic prefetcher. Currently only implements OBL prefetch. 
class prefetcher {
  string prename;
  //: pointer to "parent" processor
  prefetchProc *proc;
  //: Use Tagged OBL?
  int tagged;
  //: Degree of prefetch
  //
  // i.e. number of lines ahead to prefetch
  int degree;
  //: Use Adaptive prefetch?
  int adaptive;
  //: Mask to determine when to adapt
  unsigned long long adaptQuantaMask;
  //: Maximum prefetch degree 
  //
  // for adaptive prefetching
  int adaptMax;
  //: Degree decrement threshold
  //
  // for adaptive prefetching
  int decDeg;
  //: Degree increment threshold
  //
  // for adaptive prefetching
  int incDeg;
  //: page shift for prefetcher
  // a prefetcher will not fetch over a page boundary
  int pageShift;
  //: Set of outstanding "fake" instructions
  //
  // Used to detect if a given parcel was sent on our behest.
  set<fakeDMAInstruction*> fakes;
  //: Set of requested addresses
  set<simAddress> addrs;

#ifdef HAVE_UNORDERED_SET
  typedef std::unordered_set<simAddress> inCacheSet;
#elif defined(HAVE_TR1_UNORDERED_SET)
  typedef std::tr1::unordered_set<simAddress> inCacheSet;
#else /* unordered_set */
# ifdef __GNUC__
#  if __GNUC__ < 3
  typedef hash_set<simAddress> inCacheSet;
#  elif __GNUC__ == 3 && __GNUC_MINOR__ == 0
  typedef std::hash_set<simAddress> inCacheSet; // GCC 3.0
#  else
  typedef __gnu_cxx::hash_set<simAddress> inCacheSet; // GCC 3.1 and later
#  endif /* __GNUC__ versions */
# else /* __GNUC__ */
  typedef std::hash_set<simAddress> inCacheSet; // non-GCC
# endif
#endif

  //: Set of addresses we placed in the cache
  //
  // used to determine prefetch hit rate
  inCacheSet reqInCache;
  //: Pool of fake instructions
  static pool<fakeDMAInstruction> fakeInst;
  //: Number of requests issued
  unsigned long long requestsIssued;
  //: Number of "hits" on requests
  unsigned long long requestsHit;
  //: Total memory requests
  unsigned long long totalReq;
  //: Page limited request cancels
  //
  // Number of requests we didn't issue because they crossed a page
  // boundary.
  unsigned long long overPage;
  unsigned long long tooLate;
  //: Number of times we adaptivly changed the degree
  int adaptions;
  //: Requests in current quanta
  //
  // Used to calculate adaptive hit rate
  int subTotalReq;
  //: Request 'hits' in current quanta
  //
  // Used to calculate adaptive hit rate
  int subRequestsHit;

  //: number of stream requests
  unsigned long long streamReq;

  unsigned long long streamsDetected;

  //: should the prefetcher take load into account
  int loadAware;

  typedef list<instruction *> wakeUpList_t;
  typedef map<simAddress, wakeUpList_t > wakeUpMap_t;
  //: Map of instructions which will be woken
  //
  // Map key is the address which was being prefetched, map data is a
  // list of instruction to be woken.
  wakeUpMap_t wakeUpMap;
  //: a prefetch aware MC, capable of giving load information
  prefetchMC *mc;

  /* Streaming prefetcher stuff */

  //: set of cache lines the stream prefetcher has issued
  set<simAddress> streamIssued;
  //: Set of cachelines placed in the cache by the streaming prefetcher
  inCacheSet reqInSCache;
  //: Number of times data which was streamed in was touched
  unsigned long long streamRequestsHit;

  //: number of prefetch streams
  int streams;
  //: stream detection window
  int windowL;
  //: Necessary stream length before detection
  int detLeng;
  //: Stream round robin counter
  int rr;
  //: Current streams
  //
  // each element is the last address fetched, shifted by the cacheShift
  set<simAddress> streamSet;
  //: Count of items in window
  int inWin;
  //: temporal record of when blocks were seen
  std::queue<simAddress> window;
  //: list of pages which have beens streamed recently
  deque<simAddress> recentStreams;
  //: window of recently seen blocks
  //
  // used to detect streams
  inCacheSet contigCount;
  //: last block seen
  //
  // for a simple optimization
  simAddress lastBlock;
  void detectStream(const simAddress);

  //: collect prefetch performance stats or not
  //
  // not collecting stats improves performance by a small percentage
  int stats;

  bool memReq(const simAddress, bool &);
public:
  prefetcher(string, prefetchProc *, prefetchMC *);
  typedef enum {INST, DATA} memAccType;
  typedef enum {READ,WRITE} memAccDir;
  void memRef(const simAddress, const memAccType, const memAccDir, bool hit);
  void reportCacheEject(const simAddress);
  //bool handleParcel(parcel *p);
  bool isPreFetching(const simAddress);
  void setWakeUp(instruction *, simAddress);
  void finish();
  void preTic();
};

#endif


