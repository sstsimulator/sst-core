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


#ifndef THREAD_H
#define THREAD_H

#include "instruction.h"
#include "memory.h"

#include <vector>
using namespace std;

#include <string>
using std::string;

#include "sst/boost.h"
#include "sst/component.h"

// forward declare function from ppcMachine.h so that we don't ahve to
// drag all that code into here.
void md_init_decoder(void);

class processor;
class thread;

class threadSource {
  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_NVP(firstThreads);
    ar & BOOST_SERIALIZATION_NVP(proc);    
  }
  template<class Archive>
  void load(Archive & ar, const unsigned int version)
  {
    ar & BOOST_SERIALIZATION_NVP(firstThreads);
    ar & BOOST_SERIALIZATION_NVP(proc);    
    md_init_decoder();
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  vector<thread*> firstThreads;
  processor *proc;
public:
  void init(processor *, SST::Component::Params_t& paramsC);
  thread* getFirstThread(unsigned int);
  void deleteThread(thread *t);
};

//: Front End Thread
//
// Representation of a thread.
//
// Threads are factories for instructions which are consumed by
// processors.
//
// For backends which which to model out-of-order execution or branch
// prediction, things are a bit more complex. The back end it required
// to track the program counter. Instead of simply asking for the next
// instruction, the backend must provide a program counter. The
// backEnd is responsible for detecting when a misprediction has
// occured. If it wishes to model speculative execution, it must
// inform the thread that it is beginning speculation and when it is
// ending speculative execution.
//
// This allows the thread to do something similar to
// simpleScalar. When speculative execution begins, a copy of the
// threads state is made which is used during the speculative
// execution. When speculation ends, the thread can simply discard the
// speculative state.
//
//!SEC:Framework
class thread {  
  string Cfgstr;
  friend class boost::serialization::access;
  template<class Archive> 
  void serialize(Archive & ar, const unsigned int version )
  {
    ar & BOOST_SERIALIZATION_NVP(callStack);
    ar & BOOST_SERIALIZATION_NVP(stackStack);
    ar & BOOST_SERIALIZATION_NVP(targetStack);
    ar & BOOST_SERIALIZATION_NVP(mem_accs);
    ar & BOOST_SERIALIZATION_NVP(_isDead);
    ar & BOOST_SERIALIZATION_NVP(eviction);
    ar & BOOST_SERIALIZATION_NVP(migration);    
  }
 protected:  
  vector<simAddress> callStack;
  vector<simAddress> stackStack;
  vector<simAddress> targetStack;
  //: Is this thread active?
  //
  // A thread may be 'dead' but has not been collected yet. 
  bool _isDead;

  simAddress stack_top, stack_base;
  unsigned int call_count, max_call_stk;
  unsigned int mem_accs[MemTypes];

  bool eviction, migration;

 public:

  void setEvict(bool tf) {eviction = tf;}
  bool canEvict() {return eviction;}
  void setMigrate(bool tf) {migration = tf;}
  bool canMigrate() {return migration;}

 thread() : stack_base(0), eviction(true), migration(true) {
    for (int i = 0; i < MemTypes; i++)
      mem_accs[i] = 0;
  }

  void recordMemStat();

  void freeStack();
#if 0
  void setStackLimits (simAddress max, simAddress min);
  int checkAccess (simAddress sa);
  virtual void printCallStack();
  virtual simAddress popFromCallStack();
  virtual void pushToCallStack(simAddress lr, simAddress sp, simAddress target);
#endif

  virtual ~thread() {;}
  //: Accessor for 'death' status of thread
  bool isDead() {return _isDead;};
  //: Returns the next instruction in the stream
  //
  // This function may return a NULL if the thread cannot produce
  // another instruction. For example, it may not support more than
  // one outstanding instruction, or it may not be able to speculate
  // beyond a branch.
  virtual instruction* getNextInstruction()=0;
  //: Cancel execution of an instruction
  //
  // If an instruction which may have been fetch()ed and issue()ed
  // needs to be cancled, squash() allows this. The thread should
  // rollback any state changes caused by this instruction. An
  // instruction which has been commit()ed cannot be squash()ed.
  virtual bool squash(instruction *)=0; 
  //: Finish an instruction
  //
  // After an instruction has been commit()ed, it should be retire()ed
  // so the thread can know it has been completed and is no longer
  // outstanding.
  // 
  // This may fail if the thread detects and error. ex: an instruction
  // which was not issued from the thread was retired.
  virtual bool retire(instruction *)=0; 
  //: Acquaint a thread with a new processor
  //
  // Upon arriving (or being created) at a processor, assimilate() is
  // called to allows the thread to perform whatever book keeping or
  // resource managemnt it requires. For example, allocating a frame
  // and copying in data.
  virtual void assimilate(processor *)=0;
  //: Prepare a thread to be migrated
  //
  // Tells a thread to prepare for migration. This may include
  // deallocating frames, copying data, etc...
  virtual void packageToSend(processor *)=0;  
  //: Get InitalPC
  //
  // Get the address of the instruction where execution should start
  // (or resetart, after a thread migration say)
  virtual simAddress getStartPC()=0;
  //: Check PC validity
  //
  // See if a given address contains a valid instruction. Useful for
  // checking after branch prediction.
  virtual bool isPCValid(const simAddress)=0;
  //: Get instruction at given address
  //
  // Request an instruction at the given address from the thread. This
  // is required by back ends which may predict branches.
  virtual instruction* getNextInstruction(const simAddress)=0;
  //: Squash speculative state
  //
  // A processor may begin speculating down a 'wrong' path. After it
  // wishes to stop this speculation, it calls squashSpec() to dump
  // any changes to state that the "wrong" path may have performed.
  virtual void squashSpec()=0;
  //: Prepare speculative state
  //
  // A processor may begin speculating down a 'wrong' path. When it
  // begins to do so (say, after a branch mispredict) it calls
  // prepareSpec() so the thread can initialize a special speculative
  // state area which will be dumped eventually.
  virtual void prepareSpec()=0;
  //: Get process ID
  virtual simPID pid() const =0;
  //: Set process ID
  virtual void changePid(const simPID) =0;
  //: return instruction size in bytes
  // Required for some branch predictors.
  virtual int getInstructionSize()=0;

  //: Return Stack location
  //
  // Optional for threads to implement. Default returns 0.
  virtual simRegister getStack() const {return 0;}

  //: Returns if a given address is constant
  //
  // If a given region of memory is determined to be constant (i.e.
  // cstring sections).
  virtual bool isConstSection(const simAddress, const simPID) const {return 0;}

  //: squash an instruction but keep it around.
  //
  // This performs the actions of a squash, but does not yet reclaim
  // the instruction data structure. This allows an instruction which
  // we know will be squashed to be removed from any internal thread
  // structures, but still keep the instruction data structure active
  // and valid (i.e. it is not recycled).
  virtual bool condemn(instruction *)=0;
};

#endif 
