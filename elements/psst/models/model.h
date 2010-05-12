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
| model.h                                                   | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| Interface between Psst and Models                                          |
\---------------------------------------------------------------------------*/
#ifndef _MODEL_H
#define _MODEL_H
#include <sst_stdint.h>
#include <string>
#include <list>
#include <utility>
#include <vector>

namespace Models {

class Model; class CallbackHandler;

enum MemAccessType {MEM_READ = 0, MEM_WRITE = 1};

// Responses to add to Qemu's input queue.
void callbackRequest(CallbackHandler* m, uint64_t cycles);
  // Request a callback at "cycles" cycles.
void setIpc(double ipc);
  // Set Qemu's IPC

// Asynchronous calls from VM
uint64_t getCycle(int vm_idx);
  // Return current cycle at which Qemu is executing.

// Asynchronous calls from models to VM
uint8_t memRead(int vm_idx, uint64_t addr);
  // Return contents of memory at address "addr"
void memWrite(int vm_idx, uint64_t addr, uint8_t data);
  // Set contents of memory at address "addr"
uint64_t memSize(int vm_idx);
  // Get the size of simulated RAM

/* All models should inherit from this so they are initiailized/added to the
   big list. */
class Model {
public:
   Model();
   virtual ~Model() {} // Just to make the class polymorphic.
};

/* Inherit from one or more of the following abstract classes to receive 
   events from the VMs as function calls. */
class CallbackHandler {
public:
  CallbackHandler();
  virtual void callback(int vm_id, uint64_t cycle) = 0;
};

class MemOpHandler {
public:
  MemOpHandler();
  virtual void memOp(
    int vm_id, uint64_t vaddr, uint64_t paddr, uint8_t size, 
    MemAccessType type
  ) = 0;
};

class InstructionHandler {
public:
  InstructionHandler();
  virtual void instruction(
    int vm_id, uint64_t vaddr, uint64_t paddr, uint8_t len, uint8_t inst[]
  ) = 0;
};

class MagicInstHandler {
public:
  MagicInstHandler();
  virtual void magicInst(int vm_id, uint64_t rax) = 0;
};

extern "C" std::vector<std::pair<std::string, std::string> > **model_params;
typedef std::vector<std::pair<std::string, std::string> >::iterator ParamIt;
};
#endif
