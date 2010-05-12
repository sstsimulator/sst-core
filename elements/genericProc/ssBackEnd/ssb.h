// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SSB_H
#define SSB_H

#include "processor.h"

class LPC;

class mainProc;

namespace ssb {
  void makeTopo(string cfgstr);
  //component* whereIs1(const simAddress, const simPID);
  //component* whereIs1PIM(const simAddress, const simPID);
  LPC* WhereIsLPC(const simAddress, const simPID);
  ownerCheckFunc getWhereIs(string cfgstr);
  procStartVec getFirstThreadHome(string cfgstr);
  extern int numSystems;
  extern int memReqSizeBits;
  extern vector<mainProc*> mainProcs;
  int numProcs();
}

#endif
