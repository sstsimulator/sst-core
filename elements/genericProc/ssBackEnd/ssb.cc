// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007,2009,2010 Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "ssb.h"
#include "level1/level1.h"
#include "ssb_mainProc.h"
#include "ssb_simpleNet.h"
#include "configuration.h"
#include "hetero.h"
#include "ssb_topo.h"

#include "level1/SW2.h"
#include "level1/DRAM.h"

#include "smpMemory.h"
#include "smpProc.h"

#include "ssb_NIC.h"

#include <HTLink/HTLinkIF.h>
#include <HTLink/HTLink_bw.h>
#include <memBus/memBus.h>

int ssb::numSystems = 0;
int ssb::memReqSizeBits = 0;
static int numProcsWeSay = 0; //note - may be a lie
vector<mainProc*> ssb::mainProcs;

int ssb::numProcs() {
  return numProcsWeSay;
}
component *__HTLink_bw;

void ssb::makeTopo(string cfgstr) {
  numSystems = configuration::getValue(cfgstr + ":numSystems");
  if (numSystems == -1) {
    INFO("defaulting to numSystems = 1.\n");
    numSystems = 1;
  }
  int numSMPs = configuration::getValue(cfgstr + ":numSMP");
  if (numSMPs < 0) numSMPs = 0;
  int numDRAM = configuration::getValue(cfgstr + ":DRAMsPerMainProc");
  string netTopo = configuration::getStrValue(cfgstr + ":network");
  int simpleM = configuration::getValueWalk(cfgstr, "simpleMemory");
  int pimBacked = configuration::getValueWalk(cfgstr, "pimBacked");
 
  int useBus = configuration::getValueWithDefault(cfgstr + ":useBus", 0);

#define PSIZE(VAR, CON, DESC, DEFAULT)					\
    VAR = configuration::getValue(cfgstr + ":parcelSize:" CON);		\
    if (VAR == -1) {							\
      INFO("No given size for %s parcel. Assuming %d bits\n", DESC, DEFAULT); \
      VAR = DEFAULT;							\
    } 

    PSIZE(memReqSizeBits, "DRAMReq", "DRAM Request (ssBackend)", 4*8);
#undef PSIZE	 

  int lieProcs = configuration::getValueWithDefault(cfgstr + ":lieProcs", -1);

  // XXX: kbw: why do we want pimProcs to be 1 when we haven't defined any
  // LPCs?
  int pimProcs = configuration::getValueWithDefault(cfgstr + ":hetero:level1:numLPCs", -1) * configuration::getValueWithDefault(cfgstr + ":hetero:level1:LWPsPerLPC", -1);

  if (numSMPs > 0) {
    numProcsWeSay = numSMPs;
    if (pimBacked) numProcsWeSay = numSMPs * (1 + pimProcs);
  } else {
    numProcsWeSay = 1;
    if (pimBacked) numProcsWeSay = (1 + pimProcs);
  }
  if (lieProcs != -1) {
    INFO("We have %d processors, but if anyone asks, we have %d\n", 
	   numProcsWeSay, lieProcs);
    numProcsWeSay = lieProcs;
  }

  if (pimBacked != 1) pimBacked = 0;
  genericNetwork *net=0;
  topo *top=0;
  int x=0;
  int y=0;
  int z=0;
  long bw=0;
  int delay=0;

  if (simpleM && pimBacked) {
    ERROR("simple Memory model and pim backing don't mix\n");
  }

  if (numSystems > 1) {
    if (netTopo == "simple")			
      {
	if (numSystems > 1) 
	  net = new simpleNetwork(cfgstr);
      }
    else 
      {
	if(netTopo == "3dmesh")
	  {
	    if(numSystems > 1)
	      {
		x = configuration::getValue(cfgstr + ":xdim");
		y = configuration::getValue(cfgstr + ":ydim");
		z = configuration::getValue(cfgstr + ":zdim");
		bw = configuration::getValue(cfgstr + ":bw");
		delay = configuration::getValue(cfgstr + ":delay");
		top = new mesh3d(cfgstr,x,y,z,bw,delay);
	      }
	  }
	else
	  {
	    if(netTopo == "2dmesh")
	      {
		if(numSystems > 1)
		  {
		    x = configuration::getValue(cfgstr + ":xdim");
		    y = configuration::getValue(cfgstr + ":ydim");
		    bw = configuration::getValue(cfgstr + ":bw");
		    delay = configuration::getValue(cfgstr + ":delay");
		    top = new mesh2d(cfgstr,x,y,bw,delay);
		  }
	      } 
	    else
	      {
		ERROR("***unknown convProc network topology: %s\n", netTopo.c_str());
	      }
	  }
      }
  }

  //MLV This is farked until system correlates to addresses...
  if (numSMPs == 0) 
    memory::setUpLocalDistribution(14, numSystems);
  else
    memory::setUpLocalDistribution(14, numSystems * numSMPs);

  int procCount = -1;
  for (int s = 0; s < numSystems; ++s) {
    if((numSystems > 1) && ((netTopo=="3dmesh")||(netTopo=="2dmesh"))) {
      net=top->links[s][0];
    }
    vector<DRAM*> DRAMs;
    if (! useBus && simpleM == 0 && !pimBacked) {
      DRAM::getDRAMs(cfgstr,DRAMs,numDRAM);
    }
    if (numSMPs > 1) {
      if (pimBacked) {
	// untested...
	WARN("the hetero/SMP configuation is untested\n");
	heteroNIF *sw = new heteroNIF(cfgstr);
	for (int smp = 0; smp < numSMPs; ++smp) {
	  mainProcs.push_back(new heteroProc(cfgstr, sw, sw, net, ++procCount, 0));
	  memory::addLocalID(mainProcs.back(), smp + (s * numSMPs));
	}
	DRAM::registerMC(DRAMs,sw);
      } else {
	smpMemory *sw = new smpMemory(cfgstr,DRAMs);
	for (int smp = 0; smp < numSMPs; ++smp) {
	  mainProcs.push_back(new smpProc(cfgstr, sw, sw, net, ++procCount, sw));
	  memory::addLocalID(mainProcs.back(), smp + (s * numSMPs));
	}
	DRAM::registerMC(DRAMs,sw);
      }
    } else {
      if (pimBacked) {
	heteroNIF *sw = new heteroNIF(":hetero" + cfgstr);
	mainProcs.push_back(new heteroProc(cfgstr + ":mainProc",sw, sw, net, ++procCount, 0));
	DRAM::registerMC(DRAMs,sw);
	memory::addLocalID(mainProcs.back(), s);
      } else if ( useBus ) {

        PRINTF("useBus\n");

        char tmp[0];
        sprintf(tmp,"%d",procCount+1);
        HTLink_bw *bw = new HTLink_bw( cfgstr + ":HTLink_bw", tmp ); 

        // This is a hack, we need this further down the call chain but should
        // is be a parameter to mainProc??
        __HTLink_bw = bw;

        MemBus *bus = new MemBus(cfgstr + ":bus" );
    
        mainProcs.push_back(new mainProc(cfgstr + ":mainProc", (component*)bus,
                                net, ++procCount, 0, 0 ) );
  
        NIC *nicProc = (NIC*) mainProcs.back()->getNICProc();
        mainProc *hostProc = mainProcs.back();
      
        hostProc->getBaseMem()->RegisterMemIF( cfgstr + ":mainProc:HTLink",
                                               bw->getMemIF(0), NULL );
        nicProc->getBaseMem()->RegisterMemIF( cfgstr + ":NIC:HTLink",
                                               bw->getMemIF(1), NULL  );

        bw->registerLinkIF( 0, bus, hostProc->getBaseMem() );
        bw->registerLinkIF( 1, nicProc->getMemCtrl(), nicProc->getBaseMem() );

      } else {
	SW2 *sw2 = 0;
	if(simpleM==0) sw2 = new SW2(cfgstr,DRAMs);
	mainProcs.push_back(new mainProc(cfgstr + ":mainProc", sw2, net, ++procCount, 0, sw2));
	DRAM::registerMC(DRAMs,sw2);
	memory::addLocalID(mainProcs.back(), s);
      }
    }
  }
}

//: Simple whereIs
//
// Assumes each convProc is its own 'process' for multi-system cases
// ... when there are multiple CPUs that share an address space, there
// **SHOULD** only be one pid (PID == address space id)
component* ssb::whereIs1(const simAddress addr, const simPID pid) {
    component* retval = NULL;
  if (pid < numSystems) {
      retval = mainProcs[pid];
  } else {
      retval = mainProcs[pid-numSystems]->getNICMem();
  }
  if (retval == NULL) {
      ERROR("no processor claims pid %i! numSystems=%i\n", pid, numSystems);
  }
  return retval;
}

//: WhereIs for PIMNIC
//
// Assumes main procs are pid 0...(numSystems-1) and NIC procs are
// numSystems...(numSystems*2-1)
component* ssb::whereIs1PIM(const simAddress, const simPID pid) {
  if (pid < numSystems) {
    return mainProcs[pid];
  } else {
    return mainProcs[pid-numSystems]->getNICMem();
  }
}

LPC* ssb::WhereIsLPC(const simAddress, const simPID pid) {
  if (pid < numSystems) {
    // its not on an LPC
    return 0;
  } else {
    return mainProcs[pid-numSystems]->getPimNIC();
  }
}

ownerCheckFunc ssb::getWhereIs(string cfgstr) {
  int pimBacked = configuration::getValue(cfgstr + ":pimBacked");
  if (pimBacked == 1) {
    return level1::getWhereIs(cfgstr);
  }

  numSystems = configuration::getValueWithDefault(cfgstr + ":numSystems", 1);
  if (configuration::getStrValueWithDefault(cfgstr + ":type", "") == string("PIMNIC")) {
    level1::whichLPC = ssb::WhereIsLPC;
    return ssb::whereIs1PIM;
  } else {
    return ssb::whereIs1;
  }
}

procStartVec ssb::getFirstThreadHome(string cfgstr) {
  procStartVec tmpV;
  int diffExec = configuration::getValueWithDefault(cfgstr + ":differentExec", -1);
  int numHome = configuration::getValue(cfgstr + ":numFirstThreadHomes");

  if (diffExec > 0) {
    if (numHome > 0) {
      ERROR("%s:numFirstThreadHomes not miscable with "
	     "%s:differentExec\n",cfgstr.c_str(),cfgstr.c_str());
    }
    // init the main procs
    for (uint i = 0; i < mainProcs.size(); ++i)
      tmpV.push_back(procPIDPair(mainProcs[i], i));
    // init the nic procs
    for (uint i = 0; i < mainProcs.size(); ++i) {
      processor *nProc = mainProcs[i]->getNICProc();
      procPIDPair pp = procPIDPair(nProc, numSystems+i, "altExecFile");
      tmpV.push_back(pp);
    }
  } else {
    // normal cases - everyone gets same executable, though NICs don't
    // get a starting thread
    if (numHome < 0 || numHome >= int(mainProcs.size())) {
      numHome =  mainProcs.size();
    }
    for (int i = 0; i < numHome; ++i)
      tmpV.push_back(procPIDPair(mainProcs[i], i));
  }
  return tmpV;
}
