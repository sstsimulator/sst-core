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

#ifndef PROC_H_
#define PROC_H_

#include "sst/eventFunctor.h"
#include "sst/component.h"
#include <sst/link.h>
#include <sst/cpunicEvent.h>
#include <memory.h>

#include "ssBackEnd/ssb_mainProc.h"
#include "FE/thread.h"
#include "FE/processor.h"

using namespace SST;

#define DBG_GPROC 1
#if DBG_GPROC
#define _GPROC_DBG( lvl, fmt, args...)\
    if (gproc_debug >= lvl)   { \
	printf( "%d:genericProc::%s():%d: "fmt, _debug_rank,__FUNCTION__,__LINE__, ## args ); \
    }
#else
#define _GPROC_DBG( lvl, fmt, args...)
#endif

extern int gproc_debug;


// "processor" class
class proc : public processor {

#if WANT_CHECKPOINT_SUPPORT
BOOST_SERIALIZE {
    _GPROC_DBG(1, "begin\n");
    BOOST_VOID_CAST_REGISTER(proc*,processor*);
    _GPROC_DBG(1, "base\n");
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( processor );
    _GPROC_DBG(1, "links\n");
    ar & BOOST_SERIALIZATION_NVP( memLinks );
    ar & BOOST_SERIALIZATION_NVP( netLinks );
    _GPROC_DBG(1, "tSource\n");
    ar & BOOST_SERIALIZATION_NVP( tSource );
    _GPROC_DBG(1, "thread\n");
    ar & BOOST_SERIALIZATION_NVP( myThread );
    _GPROC_DBG(1, "ss\n");
    ar & BOOST_SERIALIZATION_NVP( ssBackEnd );
    //ar & BOOST_SERIALIZATION_NVP( mProc );
    _GPROC_DBG(1, "end\n");
  }
  SAVE_CONSTRUCT_DATA( proc ) {
    _GPROC_DBG(1, "begin\n");
    ar << BOOST_SERIALIZATION_NVP( t->id );
    ar << BOOST_SERIALIZATION_NVP( t->params );
    _GPROC_DBG(1, "end\n");
  }
  LOAD_CONSTRUCT_DATA( proc ) {
    _GPROC_DBG(1, "begin\n");
    ComponentId_t     id;
    Params_t          params;
    ar >> BOOST_SERIALIZATION_NVP( id );
    ar >> BOOST_SERIALIZATION_NVP( params );
    // call the dummy constructor which dosn't initialize the thread source
    ::new(t)proc( id, params, 0 );
  }
#endif

//  typedef MemoryDev< uint64_t, instruction* > memDev_t; 
  typedef Memory< uint64_t, instruction* > memory_t; 
  // links to memory
  std::vector< memory_t* > memory;
  // links to other processors
  std::vector<Link*> netLinks;
  // thread source
  threadSource tSource;
  // thread of execution
  thread *myThread;
  // use a Simple-scalar-based backend timing model
  int ssBackEnd;
  // use an external memory model
  int externalMem;
  // maximum main memory references outstanding
  int maxMMOut;
  // number of cores
  int cores; 
  // the simple-scalar-based processor model
  vector<mainProc *> mProcs;
  // map of outgoing memory request instructions to cores
  typedef map<instruction *, int> memReqMap_t;
  memReqMap_t memReqMap;
  instruction* onDeckInst;

  ClockHandler_t* clockHandler;
  Event::Handler_t *NICeventHandler;

  ComponentId_t id;
  Params_t& params;

  bool addThread(thread *);
  void swapThreads(bool quanta);
  // flag to determine if we are flushing pipes and need to check for
  // thread swaps
  bool needThreadSwap;
  std::deque<thread*> threadQ;
public:
  virtual int Setup();
  virtual int Finish();
  virtual bool preTic( Cycle_t );
  void processMemDevResp( );
  virtual bool handle_nic_events( Event* );
  proc(ComponentId_t id, Params_t& params);
  proc(ComponentId_t id, Params_t& params, int dummy);

  virtual bool insertThread(thread*);
  virtual bool isLocal(const simAddress, const simPID);
  virtual bool spawnToCoProc(const PIM_coProc, thread* t, simRegister hint);
  virtual bool switchAddrMode(PIM_addrMode);
  virtual exceptType writeSpecial(const PIM_cmd, const int nargs, 
				  const uint *args);
  virtual bool forwardToNetsimNIC(int call_num, char *params,
				  const size_t params_length,
				  char *buf, const size_t buf_len);
  virtual bool pickupNetsimNIC(CPUNicEvent **event);
  virtual bool externalMemoryModel();
  virtual bool sendMemoryReq( instType, uint64_t address, 
			       instruction*, int mProcID);
};

#endif
