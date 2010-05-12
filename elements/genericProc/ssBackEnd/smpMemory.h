// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007,2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SMPMEMORY_H
#define SMPMEMORY_H

#include "sharedMemory.h"
#include "level1/SW2.h"

class smpProc;

//: Shared memory for SMP system
//!SEC:shmem
class smpMemory : public sharedMemory, public SW2 {
public:
  typedef vector<smpProc*> smpVec;
protected:
  smpVec smps;
  static const int postSizeBits = 8;
  void registerPost() {
    portCount[TO_DRAM] -= postSizeBits;
    portCount[FROM_DRAM] -= postSizeBits * (smps.size() - 1);
  }
public:
  smpMemory(string cfgstr, const vector<DRAM*>& d) : sharedMemory(cfgstr), SW2(cfgstr,d) {;}
  void registerProcessor(sharedMemProc* p, smpProc* sm) {
    sharedMemory::registerProcessor(p);
    smps.push_back(sm);
  }
  const smpVec& getSMPs() const {return smps;}
  virtual void setup(){SW2::setup();}
  virtual void finish(){SW2::finish();}
  virtual void handleParcel(parcel *p){SW2::handleParcel(p);}
  virtual void preTic(){SW2::preTic();}
  virtual void postTic(){;}
};

#endif
