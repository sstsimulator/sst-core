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


/*----------------------------------------------------------+----------------\
| model.cpp                                                 | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| Interface between Psst and Models                                          |
\---------------------------------------------------------------------------*/

#include <dlfcn.h>

#include "model.h"
#include "../eventqueue.h"
#include "../queue.h"
#include "../vm-iface.h"
#include "../vm.h"
#include <cstdio>
#include <list>
#include <vector>
#include <map>
#include <utility>

const size_t VM_OUT_QUEUE_SIZE = 2*(1<<20); // 2 Megabyte queues each way.
const size_t VM_IN_QUEUE_SIZE = 2*(1<<20);  //

using Models::Model;               using Models::MemOpHandler;
using Models::InstructionHandler;  using Models::CallbackHandler;
using Models::MagicInstHandler;    using Models::model_params;

using std::list;   using std::vector; using std::pair; using std::multimap;
using std::string;

// The entire model.so global context.
struct ModelSoContext {
  ModelSoContext():vmOut(VM_OUT_QUEUE_SIZE),vmIn(VM_IN_QUEUE_SIZE) {}

  EventQueue eventQueue;

  vector<pair<string, string> > *modelParams;

  list<InstructionHandler *> instructionHandlers;
  list<MemOpHandler *> memOpHandlers;
  list<CallbackHandler *> callbackHandlers;
  list<MagicInstHandler *> magicInstHandlers;
  vector<Vm*> vms;

  Queue vmOut;
  Queue vmIn;
};

EventQueue *eventQueue;

vector<pair<string, string> > **Models::model_params;

vector<ModelSoContext *> contexts;

list<InstructionHandler *> *instructionHandlers;
list<MemOpHandler *> *memOpHandlers;
list<CallbackHandler *> *callbackHandlers;
list<MagicInstHandler *> *magicInstHandlers;

vector<Vm*> *vms;

Queue *vmOut;
Queue *vmIn;

// Select a context using the ID returned by initModelSo()
extern "C" void setPsst(int i) {
  instructionHandlers = &(contexts[i]->instructionHandlers);
  memOpHandlers = &(contexts[i]->memOpHandlers);
  callbackHandlers = &(contexts[i]->callbackHandlers);
  magicInstHandlers = &(contexts[i]->magicInstHandlers);
  vms = &(contexts[i]->vms);
  vmOut = &(contexts[i]->vmOut);
  vmIn = &(contexts[i]->vmIn);
  eventQueue = &(contexts[i]->eventQueue);
  model_params = &(contexts[i]->modelParams);
}

// Create a new context and return an integer ID for the psst instance.
extern "C" int initModelSo() {
  static int nextPsstId = 0;
  contexts.push_back(new ModelSoContext());
  setPsst(nextPsstId);

  return nextPsstId++;
}

// The "dispatch" function: dispatch the next event from the vmOut queue.
// Return 1 if there are events left to dispatch, 0 otherwise.
extern "C" int dispatch() {
  uint8_t opc;
  typedef list<InstructionHandler *>::iterator ih_it_t;
  typedef list<MemOpHandler *>::iterator mo_it_t;
  typedef list<CallbackHandler *>::iterator cb_it_t;
  typedef list<MagicInstHandler *>::iterator mi_it_t;

  try {
    vmOut->get(opc);
  } catch (queue_underflow q) {
    return 0;
  }

  switch (opc) {
    case QOPC_CALLBACK: {
        uint64_t cycle;
        vmOut->get(cycle);
        for (
          cb_it_t it = callbackHandlers->begin(); 
          it != callbackHandlers->end(); it++
	) {
          (*it)->callback(0, cycle);
        }
      }
      break;
    case QOPC_MEMOP: {
        uint64_t vaddr, paddr; uint8_t len; uint8_t type;
        vmOut->get(vaddr); vmOut->get(paddr); 
        vmOut->get(len); vmOut->get(type);
        for (
          mo_it_t it = memOpHandlers->begin();
          it != memOpHandlers->end(); it++
        ) {
          (*it)->memOp(0, vaddr, paddr, len, (Models::MemAccessType)type);
	}
      }
      break;
    case QOPC_INSTRUCTION: {
        uint64_t vaddr, paddr; uint8_t len; uint64_t instp;
        vmOut->get(vaddr); vmOut->get(paddr); 
        vmOut->get(len); vmOut->get(instp);
        for (
          ih_it_t it = instructionHandlers->begin();
          it != instructionHandlers->end(); it++
        ) {
          (*it)->instruction(0, vaddr, paddr, len, (uint8_t*)instp);
        }
      }
      break;
    case QOPC_MAGICINST: {
        uint64_t rax;
        vmOut->get(rax);
        for (
	  mi_it_t it = magicInstHandlers->begin();
          it != magicInstHandlers->end(); it++
        ) {
          (*it)->magicInst(0, rax);
        }
      
    }
    break;
  }

  return !vmOut->empty();
}

// The opposite of dispatch(): Dispatch responses to the VMs.
extern "C" int dispatch_to_vm(void) {
  uint8_t opc;
  try {
    vmIn->get(opc);
  } catch (queue_underflow q) {
    return 0;
  }
  switch(opc) {
      case QOPC_CALLBACKREQUEST: {
        CallbackHandler* m; uint64_t cycles;
        vmIn->get(m);
        vmIn->get(cycles);
        eventQueue->add(m, cycles);
      }
      break;
      case QOPC_SETIPC: {
        double ipc;
        vmIn->get(ipc);
        (*vms)[0]->ipc = ipc;
      }
      break;
  }

  return !vmIn->empty();
}

// The "request" functions: add requests to queue.
void Models::callbackRequest(CallbackHandler* m, uint64_t cycle) {
  vmIn->put((uint8_t)QOPC_CALLBACKREQUEST); vmIn->put(m); vmIn->put(cycle);
}

void Models::setIpc(double ipc) {
  vmIn->put((uint8_t)QOPC_SETIPC); vmIn->put(ipc);
}

// The "send event" functions: called by VM to add events to queue. 
void send_memOp(uint64_t vaddr, uint64_t paddr, uint8_t size, int type) {
  vmOut->put((uint8_t)QOPC_MEMOP); vmOut->put((uint64_t)vaddr); 
  vmOut->put((uint64_t)paddr);     vmOut->put((uint8_t)size); 
  vmOut->put((uint8_t)type);
}

void send_instruction(
  uint64_t vaddress, uint64_t paddress, uint8_t len, uint8_t inst[]
) {
  vmOut->put((uint8_t)QOPC_INSTRUCTION); vmOut->put((uint64_t)vaddress);  
  vmOut->put((uint64_t)paddress);        vmOut->put((uint8_t)len);
  vmOut->put((uint64_t)inst);
}

void send_magicInst(uint64_t rax) {
  vmOut->put((uint8_t)QOPC_MAGICINST); vmOut->put((uint64_t) rax);
}

// The VM can call this to undo the sending of an instruction
void discard_instruction(void) {
  const size_t INST_BYTES = 1 + 8 + 8 + 1 + 8;
  vmOut->discard(INST_BYTES);
}

// The "interactive" functions: get or set something from the VM's guts.
uint64_t Models::getCycle(int vm_idx) {
  return (*(vms->begin()))->cycle;
}

uint8_t Models::memRead(int vm_idx, uint64_t addr) {
  return (*(vms->begin()))->memRead(addr);
}

void Models::memWrite(int vm_idx, uint64_t addr, uint8_t data) {
  (*(vms->begin()))->memWrite(addr, data);
}

uint64_t Models::memSize(int vm_idx) {
  const uint64_t MEG = 1<<20;
  return 128*MEG;
}

// Implementations of model objects base classes.
Model::Model() {
  printf("New model at %p.\n", this);
}

InstructionHandler::InstructionHandler() {
  printf("New instruction handler at %p.\n", this);
  fflush(stdout);
  instructionHandlers->push_back(this);
}

MemOpHandler::MemOpHandler() {
  printf("New memory op handler at %p.\n", this);
  fflush(stdout);
  memOpHandlers->push_back(this);
}

CallbackHandler::CallbackHandler() {
  printf("New callback handler at %p.\n", this);
  fflush(stdout);
  callbackHandlers->push_back(this);
}

MagicInstHandler::MagicInstHandler() {
  printf("New magic instruction handler at %p.\n", this);
  fflush(stdout);
  magicInstHandlers->push_back(this);
}

// The functions called by Psst
extern "C" Model *load_model(const char* path) {
  void *lib = dlopen(path, RTLD_LAZY|RTLD_LOCAL);
  if (lib == NULL) { 
    printf("dlopen \"%s\":\n %s\n", path, dlerror()); 
    exit(1);
  }

  void (*initfunc)() = (void(*)())dlsym(lib, "__init");
  if (initfunc == NULL) { 
    printf("dlsym \"%s\":\n  %s\n", "__init", dlerror());
    exit(1); 
  }

  initfunc();
  Model** m = (Model **)dlsym(lib, "mod_obj");
  if (m == NULL) { 
    printf("dlsym \"%s\":\n %s\n", "mod_obj", dlerror()); 
    exit(1); 
  }
  
  return *m;
}

extern "C" int load_vm(
  const char* path, const int argc, const char** argv, long freq
) {
  new Vm(path, argc, argv, freq);
  return 1;
}

extern "C" void tick_vms(void) {
  uint64_t cycles = eventQueue->cycles();
  for (
    vector<Vm*>::iterator i = vms->begin();
    i != vms->end(); i++
  ) {
    (*i)->run(
      (int)((double)cycles*(*i)->ipc), (*i)->ipc, eventQueue->current_cycle
    );
    (*i)->cycle += eventQueue->advance();
  }
}
