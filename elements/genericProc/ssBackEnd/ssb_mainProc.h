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


#ifndef MAINPROC_H
#define MAINPROC_H

#include "ssb_sim-outorder.h"
#include "FE/pimSysCallTypes.h"
#include "FE/thread.h"

class genericNIC;
class thread;
class genericNetwork;
class PIM_NICChip;

/** Main Processor (Conventional CPU)

 Models the main processor of a conventional system. Connects to
 other processors by a NIC and network.

**/
class mainProc : public convProc {
  string confFile;
  threadSource *tSource;
  processor *myProc;
public:
  //: List of all mainProcs
  //static vector<mainProc*> mainProcs;
protected:
  //: Latency to NIC
  int latToNic;
  //: Uniqie MainProcessorID
  int mainProcID;
  //: NIC
  // Provies access to the network
  //genericNIC *nic;
  //component *nicComp;
  //processor *nicProc;
  //PIM_NICChip *pimNIC;
public:
  void setClearPipe(bool p) {
    clearPipe = p;
  }
  thread* getThread() const {return thr;}
  void setThread(thread *t) {
    if (thr != NULL) {
      ERROR("Trying to overwrite a running thread!");
    }
    thr = t;
    fetch_pred_PC = thr->getStartPC();
    thr->assimilate(myProc);
  }
  virtual unsigned getFEBDelay() {return 0;}
  //component* getNICMem() const {return nicComp;}
  //processor* getNICProc() const {return nicProc;}
  //PIM_NICChip* getPimNIC() const {return pimNIC;}
  int getMainProcID() const {return mainProcID;}
  mainProc(string configFile, threadSource &tSource, int maxMMO, processor *p,
	   int id);
  virtual void setup();
  virtual void finish();
  //virtual void handleParcel(parcel *p);
  virtual void preTic();
  virtual void postTic(){;} 

  virtual bool spawnToCoProc(const PIM_coProc, thread* t, simRegister);
  virtual bool switchAddrMode(PIM_addrMode) {printf("%s:%i (%s) not sup\n",__FILE__,__LINE__,__func__);return 0;}
  virtual exceptType writeSpecial(const PIM_cmd, const int nargs, 
			    const uint *args);
  virtual exceptType readSpecial(const PIM_cmd, const int nInArgs, 
				 const int nOutArgs, const simRegister *args,
				 simRegister *rets);
};

#endif
