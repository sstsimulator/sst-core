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


#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "sst_stdint.h" // for uint*_t
#include "sst/component.h"
#include <sst/cpunicEvent.h>
#include "thread.h"
#include "memory.h"
#include "ptoVMapper.h"
#include "pimSysCallTypes.h"
#include <vector>
#include <utility>
#include <boost/serialization/list.hpp>
//#include <writeCombining.h>
//#include <configuration.h>

using namespace std;
using namespace SST;

typedef SST::Component* (*ownerCheckFunc)(const simAddress, const simPID);

//: An inapropriately named structure with home site startup info
struct procPIDPair {
  //: The processor
  processor *first;
  //: Its PID
  simPID second;
  //: binary to load
  //
  // empty string is for the main binary. Otherwise, the binary name
  // will be the config string ":frontEnd:<binaryName>".
  string binaryName;
  //: constructor
  procPIDPair(processor *p, simPID pid) : first(p), second(pid), 
					  binaryName("") {;}
  procPIDPair(processor *p, simPID pid, const string b) : first(p), second(pid),
						 binaryName(b) {;}
};
typedef vector<procPIDPair> procStartVec;
typedef procStartVec (*getFirstThreadsHomeFunc)(string cfgstr);

//: Generic processing component
//!SEC:Framework
class processor : public SST::Component, public memory,  public ptoVmapper
{
#if WANT_CHECKPOINT_SUPPORT
  BOOST_SERIALIZE {
    BOOST_VOID_CAST_REGISTER(processor*,Component*);
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Component );
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( memory );
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( ptoVmapper );
    ar & BOOST_SERIALIZATION_NVP(myProcNum);
    ar & BOOST_SERIALIZATION_NVP(myCoreNum);
    ar & BOOST_SERIALIZATION_NVP(numCores);
  }
#endif
  //static vector<memory_interface*> fullCopies;
  //static vector<processor*> procs;
  //: Unique processor number
  //
  // Each processor has its own unique processor ID number.
  int myProcNum;
  int myCoreNum;
  int numCores;

  volatile bool nic_response;


  //WCUnit *_wcUnit;
public:
   virtual int Setup( ) { return 0; }
   virtual int Finish( ) { return 0; }

  //: request a memory be fully loaded
  //
  // This requests that a memory object recieve a full copy of memory
  // when it is loaded. Useful for ensuring that processors have
  // copies of globals for printing and such.
  //static void requestFullMemCopy(memory_interface *m) {fullCopies.push_back(m);}
public:
  void procExit() {
    unregisterExit();
  }

  //:Initilize a processor.
  // Assigns the processor a unique ID.
  //processor(int name, base_memory *baseM, int coreNum = 0) : 
  processor(ComponentId_t id, Params_t& params) : 
    Component( id  ), memory(0),
    myCoreNum(0), numCores(1)
  {
    myProcNum = 0;//procs.size(); procs.push_back(this);
    setUpLocalDistribution(12, 1); // kbw: this is a total hack, but then, so's the previous line
    nic_response= false;
    INFO("Processor at %p initialized\n", this);
  }

  //WCUnit* wcUnit() {return _wcUnit;}

  //static ownerCheckFunc whereIs;
  //static getFirstThreadsHomeFunc FirstThreadsHome;
  //static void init(string cfgstr);
  procStartVec getFirstThreadsHomes();
  bool CopyToSIM(simAddress dest, const simPID, void* source, const unsigned int Bytes);
  bool LoadToSIM(simAddress dest, const simPID, void* source, const unsigned int Bytes);
  bool CopyFromSIM(void* dest, const simAddress source, const simPID, const unsigned int Bytes);
  //: Return processor's unique id
  int getProcNum() const {return myProcNum;}
  //: Return processor's core num 
  int getCoreNum() const {return myCoreNum;}
  //: Return num of cores 
  int getNumCores() const {return numCores;}
  //: Request a frame for a thread
  //
  // A thread can request a frame from a processor of words
  // words. This frame is private to the thread and is generally used
  // to contain registers. The thread is responsible for returning the
  // thread when it is done with it.
  //
  // The frame is identified by a frameID which can be used to
  // retrieve and access the frame.
  //virtual frameID requestFrame(int words)=0;
  //: Return a frame for reuse
  //
  // Deallocate a frame.
  //virtual void returnFrame(frameID)=0;
  //: Access a frame
  //
  // Takes a frame ID and returns a pointer to that frame. 
  //virtual simRegister* getFrame(frameID)=0;
  //: Insert a new thread into the processor
  //
  // When a new thread is created, it must be inserted into a
  // processor.
  virtual bool insertThread(thread*)=0;
  //: Check the locality of an address
  //
  // Checks if a given address is local to the processor.
  virtual bool isLocal(const simAddress, const simPID)=0;
  //: Spawn a thread to a Coprocessor
  //
  // Take the thread t, and send it to a coProcessor. 
  virtual bool spawnToCoProc(const PIM_coProc, thread* t, simRegister hint)=0;
  //: Switch Addressing mode
  //
  // A processor may have multiple addressing modes. If so, this is
  // used to switch between them.
  virtual bool switchAddrMode(PIM_addrMode) = 0;
  //: Write special registers
  //
  // A number of special commands or functions are accessible by
  // "writing" special registers. 
  virtual exceptType writeSpecial(const PIM_cmd, const int nargs, 
				  const uint *args)=0;
  //: Read special registers
  //
  // A number of special commands or functions are accessible by
  // "reading" special registers. Reading may return sereral results.
  virtual exceptType readSpecial(const PIM_cmd cmd, const int nInArgs, 
				 const int nOutArgs, const simRegister *args,
				 simRegister *rets) {
    exceptType retval = PROC_EXCEPTION;
    switch( cmd ) {
      case PIM_CMD_GET_NUM_CORE:
        rets[0] = ntohl(getNumCores());
        retval = NO_EXCEPTION;
        break;
      case PIM_CMD_GET_CORE_NUM:
        rets[0] = ntohl(getCoreNum());
        retval = NO_EXCEPTION;
        break;
      case PIM_CMD_GET_MHZ:
        //rets[0] = ClockMhz();
        retval = NO_EXCEPTION;
        break;
    default:
      ;
    }
    return retval;
  };

  //: Reset internal counters
  //
  // reset internal accounting meters (IPC, inst committed, etc...)
  virtual void resetCounters() {
    printf("Reset Counters Not Supported on this Processor.\n");
  }

  //: Arg Checking Utility function
  //
  //Checks num of args to specials
  static inline void checkNumArgs(const PIM_cmd cmd, const int givenInArgs, 
				  const int givenOutArgs, 
				  const int reqInArgs, const int reqOutArgs) {
    if (givenInArgs != reqInArgs) {
      printf("Syscall %d does not have %d Input arguments. (%d given)\n", cmd, 
	     reqInArgs, givenInArgs);
    }
    if (givenOutArgs != reqOutArgs) {
      printf("Syscall %d does not have %d Output arguments. (%d given)\n", cmd, 
	     reqOutArgs, givenOutArgs);
    }
  }

  // We use this function to forward netsim calls to the netsim NIC
  virtual bool forwardToNetsimNIC(int call_num, char *params,
                                    const size_t params_length,
				    char *buf, const size_t buf_len) = 0;

  // This function returns data the NIC may have sent back to the CPU
  virtual bool pickupNetsimNIC(CPUNicEvent **event) = 0;

  // These functions are used to manage the queue of NIC replies
  std::list<CPUNicEvent *> staging_area;

  int getNICresponse(void) {return !staging_area.empty();}

  virtual bool externalMemoryModel() = 0;
  virtual bool sendMemoryReq( instType, uint64_t address,
			       instruction *inst, int mProcID) = 0;

  void addNICevent(CPUNicEvent *e) {
    staging_area.push_back(e);
  }

  CPUNicEvent *getNICevent(void) {
    CPUNicEvent *rc;

    rc= staging_area.front();
    staging_area.pop_front();
    return rc;
  }



  virtual void dataCacheInvalidate( simAddress addr ) {WARN("not implemented\n");};

#define CPU_MEM_FUNC_GEN(S)						\
  virtual uint##S##_t ReadMemory##S(const simAddress sa, const bool s) {	\
    return memory::ReadMemory##S( getPhysAddr(sa), s);			\
  }									\
    virtual bool WriteMemory##S(const simAddress sa, const uint##S##_t d,	\
				const bool s) {				\
      return memory::WriteMemory##S( getPhysAddr(sa), d, s);		\
    } 
  
  CPU_MEM_FUNC_GEN(8)
  CPU_MEM_FUNC_GEN(16)
  CPU_MEM_FUNC_GEN(32)
  CPU_MEM_FUNC_GEN(64)

};

#endif
