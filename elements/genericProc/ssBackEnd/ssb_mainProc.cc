// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007,2009,2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "ssb_mainProc.h"
#include "FE/thread.h"
#include "FE/processor.h"
#include "FE/fe_debug.h"


mainProc::mainProc(string configFile, threadSource &tS, int maxMMOut, 
		   processor *p, int id) : 
  convProc(configFile, p, maxMMOut, id), confFile(configFile), myProc(p) {
  tSource = &tS;
  mainProcID = id; 
}

void mainProc::setup() {
  ss_main( confFile.c_str() ); //get options
  
  // if we are the first core, get a thread
  if (mainProcID == 0) {
    thr = tSource->getFirstThread(mainProcID);   
    if (thr) {
      INFO("mainProc %d got thread in startup\n", mainProcID);
      instructionSize = thr->getInstructionSize();
      thr->assimilate(myProc);
    }  
  }

  sim_check_options(sim_odb);
  sim_init();
  sim_load_prog("");

  // first instruction is "free" to get the PC
  if (thr) {
    fetch_pred_PC = thr->getStartPC();
    printf("%d:npc: %p\n", mainProcID, (char*)(size_t)fetch_pred_PC);
	    // wcm: fetch_pred_PC should be simAddress, compiler complains
	    //      if I don't have the size_t, but in 64-bit simAddress should
	    //      be 32bit number.
  }
}

void mainProc::finish(){
  printf("Main proc %d:\n", mainProcID);
  convProc::finish();
}


#if 0
void mainProc::handleParcel(parcel *p){convProc::handleParcel(p);}
#endif

void mainProc::preTic() {
  sim_loop();
}

bool mainProc::spawnToCoProc(const PIM_coProc where, thread* t, simRegister hint) {
  if (where != PIM_NIC) {
    printf("spawnToCoProc destination %d invalid\n", where);
    return 0;
  } 

#if 0
  if (!nic) {
    printf("no NIC configured\n");
    return 0;
  }

  parcel *p = parcel::newParcel();
  p->travThread() = t;
  sendParcel(p, nicComp, TimeStamp()+latToNic);
#endif

  return 1;
}

exceptType mainProc::writeSpecial(const PIM_cmd c, const int nargs, 
			    const uint *args) {
  switch(c) {
  case PIM_CMD_NIC_CMD:
#if 0
    {
      NICCmd *newC = new NICCmd;
      checkNumArgs(c, nargs, 0, 7, 0);
      newC->type = (NICCmdType)args[0];
      for (int ai = 0; ai < 6; ai++) {
	newC->args[ai] = args[ai+1];
      }
      parcel *newP = parcel::newParcel();
      newP->data() = newC;
      newP->tag() = PATAG_NICCMD;
      sendParcel(newP, nicComp, TimeStamp()+latToNic);
    }
    break;
  case PIM_CMD_LU_POST_RECV:
    checkNumArgs(c, nargs, 0, 5, 0);
    {
#if 0
      printf("main proc sending to listUnit\n");
#endif
      listUnitMsg *msg = new listUnitMsg;
      msg->communicator = 0;
      msg->mmBuffer = args[0];
      msg->size = args[1];
      msg->tag = args[2];
      msg->reqAddr = args[3];
      msg->source = args[4];
      parcel *newP = parcel::newParcel();
      newP->data() = msg;
      sendParcel(newP, nicComp, TimeStamp()+latToNic);
    }
    break;
#endif
  default:
    printf("write special %d not recognized on mainProc\n", c);
  }
  return NO_EXCEPTION;
}

exceptType mainProc::readSpecial(const PIM_cmd cmd, const int nInArgs, 
				 const int nOutArgs,
				 const simRegister *args, simRegister *rets) {
  switch(cmd) {
  case PIM_CMD_PROC_NUM: //0
    processor::checkNumArgs(cmd, nInArgs, nOutArgs, 0, 1);
    rets[0] = getMainProcID();
    break;
  case PIM_CMD_CYCLE: //0
    processor::checkNumArgs(cmd, nInArgs, nOutArgs, 0, 1);
    rets[0] = TimeStamp();
    break;
#if 0
  case PIM_CMD_NUM_SYS:
    processor::checkNumArgs(cmd, nInArgs, nOutArgs, 0, 1);
    rets[0] = ssb::numSystems;
    break;
  case PIM_CMD_NUM_PROC:
    processor::checkNumArgs(cmd, nInArgs, nOutArgs, 0, 1);
    rets[0] = ssb::numProcs();
    break;
#endif
  default:
    printf("read special %d not recognized on mainProc\n", cmd);
  }
  return NO_EXCEPTION;
}
