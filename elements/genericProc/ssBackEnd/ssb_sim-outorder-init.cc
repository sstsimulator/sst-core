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
#include "ssb_resource.h"
#include "ssb_fu_config.h"
#include "ssb_rs_link.h"
#include "ssb_cv_link.h"
#include "ssb_fetch_rec.h"
//#include "configuration.h"
#include "FE/fe_debug.h"
#include <signal.h>

#include <signal.h>		// wcmclen: added for linux compatibility


/* total RS links allocated at program start */
#define MAX_RS_LINKS                    4096


#if 0
//: build functional unit config
//
// takes a config string (e.g. :NIC:fu:intALU or
// :convProc:fu:floatAdd) and finds the op lat configuation and issue
// latency configuration. opLat is the number of cycles it takes to
// produce a result, issueLat is the number of cycles before another
// instruction can be issued.
void config_fu(string confStr, res_template *resT, res_template *defT) {
  int opLat = configuration::getValue(confStr + ":op");
  int issueLat = configuration::getValue(confStr + ":issue");
  
  resT->Rclass = defT->Rclass;

  if (opLat != -1) {
    resT->oplat = opLat;
  } else {
    resT->oplat = defT->oplat;
  }

  if (issueLat != -1) {
    resT->issuelat = issueLat;
  } else {
    resT->issuelat = defT->issuelat;
  }
}
#endif

//: Perform initializations
// 
// used to load program into simulated state and perform some init
// functions, now just performs the init functions. Takes a config
// string for use in finding functional unit latencies. (e.g.
// :convProc:fu)
void
convProc::sim_load_prog(const string fuConfStr)
{

  /* load program text and data, set up environment, memory, and regs */
  //ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  INFO("Finished loading\n");fflush(stdout);
  /* initialize here, so symbols can be loaded */
#if PTRACE == 1
  if (ptrace_nelt == 2)
    {
      /* generate a pipeline trace */
      ptrace_open(/* fname */ptrace_opts[0], /* range */ptrace_opts[1]);
    }
  else if (ptrace_nelt == 0)
    {
      /* no pipetracing */;
    }
  else
7    fatal("bad pipetrace args, use: <fname|stdout|stderr> <range>");
#endif

  /* finish initialization of the simulation engine */
#if 0
  if (configuration::getValueWithDefault(fuConfStr + ":useDefault", 1)) {
#endif
  if (1) {
    INFO("Using Default FU latencies\n");
    fu_pool = res_create_pool("fu-pool", fu_config, N_ELT(fu_config));
  } else {    
#if 0
    res_desc new_config[FU_FPMULT_INDEX+1];
    for (int i = 0; i < FU_FPMULT_INDEX+1; ++i) {
      new_config[i].name = fu_config[i].name;
      new_config[i].quantity = fu_config[i].quantity;
      new_config[i].busy = fu_config[i].busy;
      switch(i) {
      case FU_IALU_INDEX: 
	config_fu(fuConfStr + ":intALU", &new_config[i].x[0], 
		  &fu_config[i].x[0]);
	break;
      case FU_IMULT_INDEX:
	config_fu(fuConfStr + ":intMult", &new_config[i].x[0],
		  &fu_config[i].x[0]);
	config_fu(fuConfStr + ":intDiv", &new_config[i].x[1],
		  &fu_config[i].x[1]);
	break;
      case FU_MEMPORT_INDEX:
	new_config[i].x[0] = fu_config[FU_MEMPORT_INDEX].x[0];
	new_config[i].x[1] = fu_config[FU_MEMPORT_INDEX].x[1];
	break;
      case FU_FPALU_INDEX:
	config_fu(fuConfStr + ":floatAdd", &new_config[i].x[0],
		  &fu_config[i].x[0]);
	config_fu(fuConfStr + ":floatCmp", &new_config[i].x[1],
		  &fu_config[i].x[1]);
	config_fu(fuConfStr + ":floatCvt", &new_config[i].x[2],
		  &fu_config[i].x[2]);
	break;
      case FU_FPMULT_INDEX:
	config_fu(fuConfStr + ":floatMult", &new_config[i].x[0],
		  &fu_config[i].x[0]);
	config_fu(fuConfStr + ":floatDiv", &new_config[i].x[1],
		  &fu_config[i].x[1]);
	config_fu(fuConfStr + ":floatSqrt", &new_config[i].x[2],
		  &fu_config[i].x[2]);
	break;
      }
    }

    fu_pool = res_create_pool("fu-pool", new_config, N_ELT(new_config));
#endif
  }

  

  //RS_link::rslink_init(MAX_RS_LINKS);

  tracer_init();
  fetch_init();
  CV_link::cv_init(this);
  eventq_init();
  readyq_init();
  ruu_init();
  lsq_init();
  
  //pre_decode();
  /* initialize the DLite debugger */
  //dlite_init(simoo_reg_obj, simoo_mem_obj, simoo_mstate_obj);

}

//: init RUU 
//  allocate and initialize register update unit (RUU) 
void convProc::ruu_init(void)
{
  RUU = (RUU_station*)calloc(RUU_size, sizeof(struct RUU_station));
  if (!RUU)
    fatal("out of virtual memory");

  RUU_num = 0;
  RUU_head = RUU_tail = 0;
  RUU_count = 0;
  RUU_fcount = 0;
}

//: init LSQ
// allocate and initialize the load/store queue (LSQ) 
void convProc::lsq_init(void)
{
  LSQ = (RUU_station*)calloc(LSQ_size, sizeof(struct RUU_station));
  if (!LSQ)
    fatal("out of virtual memory");

  if (maxMMStores == -1) {
    maxMMStores = LSQ_size;
    INFO("maxMMStores not specified, so using LSQ size (%d)\n",maxMMStores);
  }

  LSQ_num = 0;
  LSQ_head = LSQ_tail = 0;
  LSQ_count = 0;
  LSQ_fcount = 0;
  lsq_mult = 0;
}

// init fetch stuff
// initialize the instruction fetch pipeline stage 
void convProc::fetch_init(void)
{
  /* allocate the IFETCH -> DISPATCH instruction queue */
  fetch_data =
    (struct fetch_rec *)calloc(ruu_ifq_size, sizeof(struct fetch_rec));
  if (!fetch_data)
    fatal("out of virtual memory");

  fetch_num = 0;
  fetch_tail = fetch_head = 0;
  IFQ_count = 0;
  IFQ_fcount = 0;
}

//: More state init
// was part of sim_main 
void convProc::sim_init()
{
  sim_num_refs = 0;


  /* allocate and initialize memory space */
  //mem = mem_create("mem");
  //mem_init(mem);

  /* ignore any floating point exceptions, they may occur on mis-speculated
     execution paths */
  signal(SIGFPE, SIG_IGN);
  
  /* register all simulator stats */
  sim_sdb = stat_new();
  sim_reg_stats(sim_sdb);

  /* record start of execution time, used in rate stats */
  sim_start_time = time((time_t *)NULL);
  /* default architected value... */
  sim_num_insn = 0;


}
