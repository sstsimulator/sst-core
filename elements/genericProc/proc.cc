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

#include <sst_config.h>

#include "proc.h"
#include "FE/thread.h"
#include "sst/boost.h"
#include <sst/cpunicEvent.h>
#include <algorithm>

int gproc_debug;


extern "C" {
  proc* genericProcAllocComponent( SST::ComponentId_t id,
				   SST::Component::Params_t& params )
  {
    proc* ret = new proc( id, params );
    return ret;
  }
}

// dummy constructor, called in deserialization. 
proc::proc(ComponentId_t idC, Params_t& paramsC, int dummy) :
  processor(idC, paramsC), id(idC), params(paramsC) {
  _GPROC_DBG(1, "Dummy Constructor\n");
}

proc::proc(ComponentId_t idC, Params_t& paramsC) : 
  processor(idC, paramsC), myThread(0), ssBackEnd(0),
    externalMem(0), maxMMOut(-1), cores(1), 
  onDeckInst(NULL),
  id(idC), params(paramsC)
{
  _GPROC_DBG(1, "Constructor\n");
  tSource.init(this, paramsC);
  myThread = 0;
  needThreadSwap = 0;

  memory_t* tmp = new memory_t( );
  // construct the params for the memory device
  Params_t memParams;
  for (Params_t::iterator pi = paramsC.begin(); pi != paramsC.end(); ++pi) {
    if (pi->first.find("mem.") == 0) {
      memParams[pi->first] = pi->second;
    }
  }
  tmp->devAdd(  new memory_t::dev_t( *this, memParams, "mem0" ), 0, 
		0x100000000ULL );
#if 0
  tmp->map( 0x0, 0x0, 0x100000 );
  tmp->map( 0x100000, 0x200000, 0x1000 );
  tmp->map( 0x300000, 0x300000, 0x100000000 - 0x300000 );
#endif
  memory.push_back( tmp );

  // add links
  NICeventHandler = new EventHandler<proc, bool, Event *>
    (this, &proc::handle_nic_events);

  netLinks.push_back(LinkAdd("net0", NICeventHandler));
  
  clockHandler = new EventHandler< proc, bool, Cycle_t>
    ( this, &proc::preTic );

  // find config parameters
  std::string clockSpeed = "1GHz";
  string ssConfig;
  Params_t::iterator it = params.begin();
  while( it != params.end() ) {
    _GPROC_DBG(1, "key=%s value=%s\n",
	       it->first.c_str(),it->second.c_str());
    if ( ! it->first.compare("clock") ) {
//       sscanf( it->second.c_str(), "%f", &clockSpeed );
      clockSpeed = it->second;
    }
    if (!it->first.compare("debug"))   {
      sscanf(it->second.c_str(), "%d", &gproc_debug);
    }
    if (!it->first.compare("cores"))   {
      sscanf(it->second.c_str(), "%d", &cores);
    }
    if (!it->first.compare("ssBackEnd"))   {
      sscanf(it->second.c_str(), "%d", &ssBackEnd);
    }
    if (!it->first.compare("externalMem"))   {
      sscanf(it->second.c_str(), "%d", &externalMem);
    }
    if (!it->first.compare("maxMMOut"))   {
      sscanf(it->second.c_str(), "%d", &maxMMOut);
    }
    if (!it->first.compare("ssConfig"))   {
      ssConfig = it->second;
    }
    ++it;
  }

  if (cores > 1 && !ssBackEnd) {
    printf("Multicore is currently only allowed with the SimpleScalar "
	   "backend\n");
    exit(-1);
  }

  // if we are using the simple scalar backend, initialize it
  if (ssBackEnd) {
    for (int c = 0; c < cores; ++c) {
      mProcs.push_back(new mainProc(ssConfig, tSource, maxMMOut, this, c));
    }
  } else {
    mProcs.clear();
  }

  _GPROC_DBG(1, " Registering clockHandler %p @ %s\n", clockHandler, 
	     clockSpeed.c_str());
  //   ClockRegister( clockSpeed, clockHandler );
  registerClock( clockSpeed, clockHandler );
}

// handle incoming NIC events. Just put them on the list for the
// user to pick up
// bool proc::handle_nic_events( Time_t t, Event *event) {
bool proc::handle_nic_events(Event *event) {
  _GPROC_DBG(4, "CPU %lu got a NIC event at time FIXME\n", Id());
  this->addNICevent(static_cast<CPUNicEvent *>(event));
  return false;
}

void proc::processMemDevResp( ) {
  instruction* inst;
  while ( memory[0]->popCookie( inst ) ) {

    if (mProcs.empty()) {
      continue;
    }

    //INFO("recived mem for addr=%#lx (type=?, tag=%p)\n", 
    //(unsigned long) 0, inst);
    if ((inst) == (instruction*)(~0)) {
      // if it is a returning writeback, just give it to the first
      // core
      mProcs[0]->handleMemEvent(inst);
    } else {
      memReqMap_t::iterator mi = memReqMap.find(inst);
      if (mi != memReqMap.end()) {
        // hand it off
	    mProcs[mi->second]->handleMemEvent(inst);
	    // remove the reference
	    memReqMap.erase(mi);
      } else {
	    printf("Got back memory req for instruction we never issued\n");
	    exit(-1);
      }
    }
  }  
}

int proc::Setup() {
    _GPROC_DBG(1,"\n");
  registerExit();
  if (!mProcs.empty()) {
    for (int c = 0; c < cores; ++c) {
      mProcs[c]->setup();
    }
  } else {
    myThread = tSource.getFirstThread(0);
    INFO("proc got Thread %p\n", myThread);
    myThread->assimilate(this);
  }
  return 0;
}

int proc::Finish() {
    _GPROC_DBG(1,"\n");
  if (!mProcs.empty()) {
    for (int c = 0; c < cores; ++c) {
      mProcs[c]->finish();
    }
  }
  printf("proc finished at %llu ns\n", getCurrentSimTimeNano());
  return false;
}

// if we have extra threads, try to swap them in 
void proc::swapThreads(bool quanta) {
  // check if there is anyone to swap in...
  if (threadQ.empty()) {
    return;
  } else {
    // check if this is a normal thread quanta, or if we have already
    // told cores to flush
    if (quanta) {
      // try to add threads until we have added them all, tried to, or
      // adding a thread failed
      bool done = false;
      int tries = threadQ.size();
      while (!done && (tries > 0)) {
	if (threadQ.empty()) {
	  done = true;
	} else {
	  thread *t = threadQ.front();
	  threadQ.pop_front();
	  if (addThread(t) == false) {
	    done = true;
	  }
	}
	tries--;
      }
      // if we weren't able to add them all, tell some processors to
      // flush
      if (!threadQ.empty()) {
	int numToFlush = std::max(threadQ.size(), mProcs.size());
	int tellFlush = std::min(numToFlush, int(mProcs.size()));
	for (int i = 0; i < tellFlush; ++i) {
	  // tell the processor to start clearing its pipe
	  mProcs[i]->setClearPipe(1);
	}
	needThreadSwap = 1;
      } else {
	// make sure no one is left trying to clear
	for (vector<mainProc *>::iterator i = mProcs.begin();
	   i != mProcs.end(); ++i) {
	  (*i)->setClearPipe(0);
	}
	needThreadSwap = 0;
      }
    } else {
      // we have asked processors to clear their pipelines.
      // check which processors are available
      for (vector<mainProc *>::iterator i = mProcs.begin();
	   i != mProcs.end(); ++i) {
	mainProc *p = *i;
	if ((p->getThread() != NULL) && p->pipeClear()) {
	  // found a processor waiting for a new thread.
	  // save the old thread
	  threadQ.push_back(p->getThread());
	  // reset the processor to take new threads
	  p->setThread(NULL);
	  p->setClearPipe(0);
	}
      }
      // try to reschedual waiting threads
      swapThreads(1);
    }
  }
}

// processor preTic(). Runs some tests
bool proc::preTic(Cycle_t c) {

  processMemDevResp(); 

  if (!mProcs.empty()) {
    // Note: need to be able to adjust thread schedualer quantum
    if (((c & 0x1fffff) == 0))  {
      swapThreads(1);
    } else if (needThreadSwap && ((c & 0xf) == 0)) {
      swapThreads(0);
    }

    for (int c = 0; c < cores; ++c) {
      mProcs[c]->preTic();
    }
  } else {
    if (myThread) {

      if (myThread->isDead()) {
	    tSource.deleteThread(myThread);
	    myThread = 0;
	    //_GPROC_DBG(1, " Dead Thread\n");
	    unregisterExit();
	    return false;
      }

      if ( onDeckInst == NULL ) {
        _GPROC_DBG(1,"getNextInstruction\n");
        onDeckInst = myThread->getNextInstruction(); 
      
        onDeckInst->fetch(this);
        onDeckInst->issue(this);
      }
 
      if (( onDeckInst->op() == LOAD || onDeckInst->op() == STORE) 
            && externalMemoryModel()) 
      {
	if ( sendMemoryReq( LOAD, 
			    (uint64_t)onDeckInst->memEA(), 0, 0) )
        {
          if (onDeckInst->commit(this)) {
	        myThread->retire(onDeckInst);
          } else {
	        WARN("instruction exception!!!");
          }
          onDeckInst = NULL;
        } else {
            _GPROC_DBG(1,"memory stalled\n");
        }
      } else {

        if (onDeckInst->commit(this)) {
	      myThread->retire(onDeckInst);
        } else {
	      WARN("instruction exception!!!");
        }
        onDeckInst = NULL;
      }
    }
  }

  return false;
}

bool proc::addThread(thread *t) {
  if (mProcs.size() < 2) {
    ERROR("adding threads to running process only allowed with multiple cores");
  } else {
    // find a processor with an open thread slot, add thread
    for (vector<mainProc *>::iterator i = mProcs.begin();
	 i != mProcs.end(); ++i) {
      mainProc *p = *i;
      if (p->getThread() == NULL) {
	p->setThread(t);
	return true;
      }
    }
    // couldn't find a slot
    WARN("all cores full!");
    threadQ.push_back(t);
    return false;
  }
}

bool proc::insertThread(thread* t) {
  addThread(t);
  return true;
}

bool proc::isLocal(const simAddress, const simPID) {
  WARN("%s not supported\n", __FUNCTION__);
  return false;
}

bool proc::spawnToCoProc(const PIM_coProc, thread* t, simRegister hint) {
  return addThread(t);
}

bool proc::switchAddrMode(PIM_addrMode) {
  WARN("%s not supported\n", __FUNCTION__);
  return false;
}

exceptType proc::writeSpecial(const PIM_cmd cmd, const int nargs, 
			      const uint *args) {
  WARN("%s %d not supported\n", __FUNCTION__, cmd);
  return NO_EXCEPTION;
}

bool proc::forwardToNetsimNIC(int call_num, char *params, const size_t params_length,
    char *buf, const size_t buf_len)
{
  _GPROC_DBG(2, "%s: call_num is %d, params %p, params len %ld, buf %p, len %ld\n",
    __FUNCTION__, call_num, params, (unsigned long) params_length, buf,
    (unsigned long) buf_len);

  // This is where we create an event and send it to the NIC
  CPUNicEvent *event;
  event= new CPUNicEvent;
  event->AttachParams(params, params_length);
  event->SetRoutine(call_num);

  if (buf != NULL)   {
    // Also attach the send data
    event->AttachPayload(buf, buf_len);
  }

  // Send the event to the NIC
  CompEvent *e= static_cast<CompEvent *>(event);
  netLinks[0]->Send(e);

  return false;
}


bool proc::pickupNetsimNIC(CPUNicEvent **event) {
  if (!this->getNICresponse())   {
    _GPROC_DBG(5, "Nothing to pick-up from NIC %lu\n", Id());
    return false;
  }

  *event= this->getNICevent();
  _GPROC_DBG(4, "NIC %lu has data for the user\n", Id());
  return true;
}

bool proc::externalMemoryModel() {
  return externalMem;
}

bool proc::sendMemoryReq( instType iType,
            uint64_t address, instruction *i, int mProcID) {
    
  //note:it is not clear that a failure to send is handled anywhere...

  _GPROC_DBG(1,"instruction type %d\n", iType );
  if ( iType == STORE ) {
    if ( ! memory[0]->write(address, i ) ) {
      INFO("Memory Failed to Write");
      return false;
    }
  } else {
    if ( ! memory[0]->read(address, i ) ) {
      INFO("Memory Failed to Read");
      return false;
    }
  }   

  if (!mProcs.empty() && (i != (instruction*)(~0))) {
    // if we need to, record the memory event
    memReqMap[i] = mProcID;
  }
  //INFO("Issuing mem req for addr=%#lx (type=%d, tag=%p) %d\n", 
  //     (unsigned long) address, iType, i, memReqMap.size());
  return true;
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(proc)

// // register event handler
// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 proc, bool, SST::Time_t, SST::Event* )
// // register clock event
// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
// 			      proc, bool, SST::Cycle_t, SST::Time_t )

BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                proc, bool, SST::Event* )
// register clock event
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
			      proc, bool, SST::Cycle_t )
#endif
    
