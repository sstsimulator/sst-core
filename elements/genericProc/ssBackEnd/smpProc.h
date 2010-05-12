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


#ifndef SMPPROC_H
#define SMPPROC_H

#include "ssb_mainProc.h"
#include "smpMemory.h"
#include <map>

using namespace std;

//: Conventinoal SMP processor
//!SEC:shmem
class smpProc : public mainProc, public sharedMemProc {
  typedef enum {INVALID, SHARED, EXCLUSIVE} blkTag;
  typedef map<simAddress, blkTag> tagMap;
  tagMap blkTags;
  smpMemory *sharedMem;
  void writeBackBlock(simAddress);
  unsigned long long invalidates[2];
  unsigned long long busMemAccess;
public:
  smpProc(string cfgstr, smpMemory *sm, component *mc, genericNetwork *net, int sysNum, prefetchMC *pmc);
  virtual unsigned getFEBDelay() {return 8;}
  virtual void noteWrite(const simAddress a);
  virtual void busReadMiss(simAddress);
  virtual void busWriteMiss(simAddress);
  virtual void busWriteHit(simAddress);
  virtual void setup();
  virtual void preTic();
  virtual void finish();
  virtual void handleParcel(parcel *p);
  virtual bool spawnToCoProc(const PIM_coProc, thread* t, simRegister hint);

  virtual unsigned int cplx_mem_access_latency(const enum mem_cmd cmd,
					       const md_addr_t baddr,
					       const int bsize,
					       bool &needMM);

  virtual uint8 ReadMemory8(const simAddress, const bool);
  virtual uint16 ReadMemory16(const simAddress, const bool);
  virtual uint32 ReadMemory32(const simAddress, const bool);
  virtual bool WriteMemory8(const simAddress, const uint8, const bool);
  virtual bool WriteMemory16(const simAddress, const uint16, const bool);
  virtual bool WriteMemory32(const simAddress, const uint32, const bool);
  virtual uint8 getFE(const simAddress);
  virtual void setFE(const simAddress, const uint8 FEValue);
  virtual void squashSpec();
};

#endif
