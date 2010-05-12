/* Copyright 2010 Sandia Corporation. Under the terms
 of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
 Government retains certain rights in this software.
 
 Copyright (c) 2007-2010, Sandia Corporation
 All rights reserved.
 
 This file is part of the SST software package. For license
 information, see the LICENSE file in the top level directory of the
 distribution. */


#include "ssb_sim-outorder.h"

//: handle frame request
frameID convProc::requestFrame(int size) {
  static frameID fids = 1;
  simRegister *frameSpace = (simRegister*)malloc(sizeof(*frameSpace)*size);
  frameID fid = fids++;
  allocatedFrames[fid] = frameSpace;
  return fid;
}

//: return a frame to a thread
simRegister* convProc::getFrame(frameID fid) {
  return allocatedFrames[fid];
}

//: deallocate a frame
void convProc::returnFrame(frameID fid) {
  simRegister *retF = allocatedFrames[fid];
  allocatedFrames.erase(fid);
  if (retF) free(retF);
}

//: not supported
bool convProc::insertThread(thread*) {
  printf("convProc: thread insert not supported\n");
  return 0;
}

#if 0
//: Return the owner of memory
bool convProc::isLocal(const simAddress sa, const simPID p) {
  return (whereIs(sa, p) == this);
}
#endif

void convProc::dataCacheInvalidate( simAddress addr )
{
  if (cache_dl1) {cache_invalidate_addr(cache_dl1, addr&~3, TimeStamp());}
  if (cache_dl2) {cache_invalidate_addr(cache_dl2, addr&~3, TimeStamp());}
}
