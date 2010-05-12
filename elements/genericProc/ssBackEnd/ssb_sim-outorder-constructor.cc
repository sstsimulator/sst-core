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

#include "ssb_sim-outorder.h"
#include "processor.h"
#include "prefetch.h"

#define MAX_RS_LINKS 1024

//: Constructor
convProc::convProc(string configFile, processor *p, int maxMMOut, int coreNum )
  : myProc(p), myCoreID(coreNum),
    clearPipe(0), isSyncing(0), clockRatio(1), iFetchBlocker(0), 
    rs_free_list(MAX_RS_LINKS), last_inst_missed(FALSE), 
    last_inst_tmissed(FALSE),
    sim_num_insn(0), sim_total_insn(0), sim_num_refs(0), sim_total_refs(0),
    sim_num_loads(0), sim_total_loads(0), sim_num_branches(0), 
    sim_total_branches(0), inst_seq(0), ptrace_seq(0), spec_mode(FALSE),
    lsq_mult(FALSE), ruu_fetch_issue_delay(0), ruu_dispatch_delay(0),
    pred_perfect(FALSE), fu_pool(NULL), tickCount(0) {
  instructionSize = 4; //default
  //requestFullMemCopy(this);

  simpleMemory = !myProc->externalMemoryModel();
  if (!simpleMemory) {
    printf("Using External Memory Model\n");
  }

  simpleFetch = 0; // simple fetch is currently broken

  maxMMStores = maxMMOut; 
  if (maxMMStores <= 0) {
    maxMMStores = -1; // if not specified, we make it the size of the
		      // LSQ. However, we don't know that right now,
		      // so we set to -1.
    printf("MM\n");
  }

#if 0
  if ( wcUnit() ) { 
    wcUnit()->RegisterMemCtrl(memCtrl);
  }
#endif
  
  portLimitedCommit = 8;//configuration::getValueWithDefault(cfgstr + ":portLimitedCommit", 0);
  regPortAvail = 0;
  
  waciLoadExtra = 1;//configuration::getValueWithDefault(cfgstr + ":waciExtraCycles", 0);
  
  lsqCompares = 0;  

#if GET_IMIX == 1
  for (int i = 0; i < LASTINST; ++i) {
    iMix[i] = 0;
  }
#endif

#warning prefetcher broken
#if 0
  string pre = configuration::getStrValueWithDefault(cfgstr + ":prefetch", "");
  if (pre != "NULL" && pre != "") {
    pref = new prefetcher(cfgstr + ":prefetch",this, pMC);
  } else {
    pref = 0;
  }
#else
  pref = 0;
#endif

  ptrace_nelt = 0;
  bimod_nelt = 1;
  bimod_config[0] = 2048;
  twolev_nelt = 4;
  twolev_config[0] = 1;
  twolev_config[1] = 1024;
  twolev_config[2] = 8;
  twolev_config[3] = FALSE;
  comb_nelt = 1;
  comb_config[0] = 1024;
  ras_size = 8;
  btb_nelt = 2;
  btb_config[0] = 512;
  btb_config[1] = 4;
  ruu_include_spec = TRUE;
  RUU_size = 8;
  LSQ_size = 4;
  mem_nelt = 2;
  mem_lat[0] = /* lat to first chunk */18;
  mem_lat[1] = 2; /* lat between remaining chunks */
  pcstat_nelt = 0;

  thr = 0;

  //id = convProcs.size();
  //convProcs.push_back(this);
  last_op.next = 0;
  last_op.rs = 0;
  last_op.tag = 0;
}

