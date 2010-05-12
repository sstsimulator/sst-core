// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2010, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifdef HAVE_CONFIG_H
#include "sst_config.h"
#endif

#include "sst/component.h"
#include "processor.h"
#include "memory.h"
//#include "configuration.h"
//#include "pim.h"
//#include "level1/level1.h"
//#include "ssBackEnd/ssb.h"
//#include "pc/pc.h"
//#include "pc/pcX2.h"
#if USE_REDSTORM_SIM
#include "redstorm/redstorm.h"
#include "pgas/pgas.h"
#endif
//#include "tlet/tlet.h"
//#include "waciBasic/waciBasic.h"

//: Determine address owner
//
// Function pointer to function which determines the owner of a given
// address. This is required so we know which addresses are assigned
// where.
//ownerCheckFunc processor::whereIs = NULL;
//: Number of processors
//vector<processor*> processor::procs;
//: Memories requesting full copy
//
// At load time, a memory can have all of memory loaded into it. This
// is useful to get global variables.
//vector<memory_interface*> processor::fullCopies;
//:Get the processor homes for the first threads
//getFirstThreadsHomeFunc processor::FirstThreadsHome = NULL;


//: Return starting point of first thread
//
// The front-end can call this function before the simulation starts so
// that it will know where the first thread will be starting. This
// allows the front-end to assimilate() that thread to its future home.
//
// This function is essentially a switch around the corrent
// getFirstThreadHome() function for the chosen topology.
//
// Note: ugly implementation, but we should only call this once.
procStartVec processor::getFirstThreadsHomes() 
{
  /*static procStartVec tHomes;
    string topo; */

  /* note: this really needs to be done in an initialization function
   * somewhere, but I'm too lazy to write it
   */

  /*if(FirstThreadsHome == NULL) {
      topo = configuration::getStrValue(cfgstr + ":topology");
      if (topo == "simple") {
          FirstThreadsHome = pim::getFirstThreadHome;
      } else if (topo == "level1") {
          FirstThreadsHome = level1::getFirstThreadHome;
      } else if (topo == "convProc") {
	FirstThreadsHome = ssb::getFirstThreadHome;
      } else if (topo == "pc") {
	FirstThreadsHome = pc::getFirstThreadHome;
      } else if (topo == "pcX2") {
	FirstThreadsHome = pcX2::getFirstThreadHome;
#if USE_REDSTORM_SIM
      } else if (topo == "Redstorm") {
	FirstThreadsHome = Redstorm::getFirstThreadHome;
      } else if (topo == "pgas") {
	FirstThreadsHome = pgas::getFirstThreadHome;
#endif
      } else if (topo == "tlet") {
	FirstThreadsHome = tlet::getFirstThreadHome;
      } else if (topo == "waciBasic") {
	FirstThreadsHome = waciBasic::getFirstThreadHome;
      } else {
          fprintf(stderr, "processsor::getFirstThreadsHomes(): unknown "
                  "topology %s\n", topo.c_str());
          exit(-1);
    }
  }      

  if (tHomes.empty())
      tHomes = FirstThreadsHome(cfgstr);

      return tHomes;*/

  procPIDPair p(this,0);
  procStartVec v;
  v.push_back(p);
  return v;
}

//: Initialize the memory model
//
// Similar to processor::getFirstThreadHome(), this is essentially a
// switch which depends on the chosen memory model. It checks that
// configuration value, and then sets processor:whereIs based upon the
// value.
//
// This is required so we know which addresses are assigned where.
//void processor::init(string cfgstr)
//{
  // init the memory model
  /*string md = configuration::getStrValue(cfgstr + ":memDist");
  
  if (md == "allpim") {
    whereIs =  pim::pimWhereIs;
  } else if (md == "level1") {
    whereIs = level1::getWhereIs(cfgstr);
  } else if (md == "convProc") {
    whereIs = ssb::getWhereIs(cfgstr);
  } else if (md == "pc") {
    whereIs = pc::getWhereIs(cfgstr);
  } else if (md == "pcX2") {
    whereIs = pcX2::getWhereIs(cfgstr);
#if USE_REDSTORM_SIM
  } else if (md == "Redstorm") {
    whereIs = Redstorm::getWhereIs(cfgstr);
  } else if (md == "pgas") {
    whereIs = pgas::getWhereIs(cfgstr);
#endif
  } else if (md == "tlet") {
    whereIs = tlet::getWhereIs(cfgstr);
  } else if (md == "waciBasic") {
    whereIs = waciBasic::getWhereIs(cfgstr);
  } else {
    fprintf(stderr, "processor::init(): unknown memory Distribution: %s\n", 
            md.c_str());
    exit(-1);
    }*/
//}

//: Copy data to the simulated memory
//
// Copy Bytes bytes of data from source in the
// application to the simulation address dest.
//
// This is primarily used by the front-end loader to load DATA into
// the address space. It is also used by some system calls (like
// read()). This handles making sure data is routed to the correct
// component which owns the address. Ownership is determed by the
// whereIs function.
bool processor::CopyToSIM(simAddress dest, 
	                  const simPID pid, 
			  void* source, 
			  const unsigned int Bytes)
{
    uint8_t *srcptr;
    simAddress destptr;
    //memory_interface *t;
    size_t i;

    srcptr = (unsigned char *)source;
    destptr = dest;

    for(i=0 ; i < Bytes; i++)  // 64b
    {
      //t = dynamic_cast<memory_interface*> (whereIs((simAddress)destptr, pid)); // 64b
      //if(!t->WriteMemory8(destptr, (uint8)*srcptr, 0))  // 64b
      if(!WriteMemory8(destptr, *srcptr, 0))  // 64b
	{
	  fprintf(stderr, "processor::CopyToSIM: error.\n");
	  return(0);
        }
        srcptr++;
        destptr++;
    }
    return true;
}

//: Load data to simulated memories
//
// Similar to CopyToSIM, except it will load the data to all memories
// which have requested a full copy. This is useful for ensuring that
// processors have copies of globals for printing and such.
bool processor::LoadToSIM(simAddress dest, const simPID pid, void* source,
			  const unsigned int origBytes)
{
    uint8 *srcptr;
    simAddress destptr;
    vector<memory_interface *>::iterator i;
    size_t j;

    processor::CopyToSIM(dest, pid, source, origBytes);

    //for(i=fullCopies.begin(); i != fullCopies.end(); i++) {

    srcptr = (unsigned char *)source;
    destptr = dest;
    
    for(j=0; j < origBytes; j++) {
      //if(!(*i)->WriteMemory8(destptr, (uint8)*srcptr, 0)) {
      if(!WriteMemory8(destptr, (uint8)*srcptr, 0)) {
	fprintf(stderr, "processor::CopyToSIM: error.\n");
	return(0);
      }
      srcptr++;
      destptr++;
    }
    //}

    return true;
}

//: Copy data from the simulated memory
//
// Copy Bytes bytes of data from the simulation address
// source to the application address dest.
bool processor::CopyFromSIM(void* dest, const simAddress source, const simPID pid,
			    const unsigned int Bytes)
{
    simAddress srcptr = source;
    uint8 *destptr = (uint8 *)dest;
    //memory_interface *t;
    size_t i;

    for(i=0; i < Bytes; i++) {
      //t = dynamic_cast<memory_interface*> (whereIs(srcptr, pid));
      //*destptr = t->ReadMemory8(srcptr, 0);
      *destptr = ReadMemory8(srcptr, 0);
      destptr++;
      srcptr++;
    }

    return true;
}

