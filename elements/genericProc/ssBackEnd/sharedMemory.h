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


#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include "memory.h"
#include <vector>

//: Interface for shared memory processors
// 
// Allows the processor to react to bus transactions. The shared mem
// proc is responsible for registering itself with a shared memory
// object.
//
//!SEC:shmem
class sharedMemProc {
public:
  virtual ~sharedMemProc() {;}
  virtual void busReadMiss(simAddress)=0;
  virtual void busWriteMiss(simAddress)=0;
  virtual void busWriteHit(simAddress)=0;
};

//: A shared memory
//
// Implements the simple cache coherence protocol in Hennessy and Patterson.
//
//!NOTE: Assumes all bus transactions are atomic. 
//!SEC:shmem
class sharedMemory : public memory_interface {
  base_memory *myMem;
  //:Registered Processors
  vector<sharedMemProc*> procs;
public:
  //: Bus Message types
  typedef enum {READ_MISS, WRITE_MISS, WRITE_HIT} msgType;
  //: Register a new processor
  void registerProcessor(sharedMemProc* p) {procs.push_back(p);}
  //: add in contention on the bus (if any)
  virtual void registerPost() = 0;
  // Post a message to the bus
  int postMessage(msgType t, simAddress addr, sharedMemProc* poster) {
    int nP = procs.size();
    for(int i=0; i < nP; ++i) {
      sharedMemProc* targP = procs[i];
      if (targP != poster) {
	switch(t) {
	case READ_MISS:
	  targP->busReadMiss(addr); break;
	case WRITE_MISS:
	  targP->busWriteMiss(addr); break;
	case WRITE_HIT:
	  targP->busWriteHit(addr); break;
	default:
	  printf("unknown bus message tyoe %d\n", t);
	}
      }
    }
    registerPost();
    return 0;
  }

#define MEM_FUNC_GEN(S)                                                 \
  virtual uint##S ReadMemory##S(const simAddress sa, const bool s) {    \
    return myMem->ReadMemory##S(sa, s);                                 \
  }                                                                     \
  virtual bool WriteMemory##S(const simAddress sa, const uint##S d, const bool s) { \
    return myMem->WriteMemory##S(sa, d, s);                             \
  }

  MEM_FUNC_GEN(8)
  MEM_FUNC_GEN(16)
  MEM_FUNC_GEN(32)

  virtual uint8 getFE(const simAddress sa) {return myMem->getFE(sa);}
  virtual void setFE(const simAddress sa, const uint8 FEValue) {
    myMem->setFE(sa, FEValue);
  }
  virtual void squashSpec() {myMem->squashSpec();}


  base_memory *getBaseMem() {return myMem;}

  sharedMemory(string cfgstr) {myMem = new base_memory(cfgstr);}
  ~sharedMemory() {
    delete myMem;
  }
  virtual void setup()=0;
  virtual void finish()=0;
  virtual void handleParcel(parcel *p)=0;
  virtual void preTic()=0;
  virtual void postTic()=0;
};


#endif
