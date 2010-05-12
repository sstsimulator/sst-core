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


/*---------------------------------------------------------+-----------------\
| Fault Model for Psst                                     | Chad D. Kersey  |
+----------------------------------------------------------+-----------------+
| Example of a fault injector model for Psst. Injects random transient bit   |
| flips lasting [0, 10,000] cycles in random locations in physical RAM in    |
| the VM.                                                                    |
\---------------------------------------------------------------------------*/
#include <cstdio>
#include <cstdlib>
#include "model.h"

/* Comment out the following lines to see verbose output. */
// #define printf(x...)
// #define puts(x) 

extern "C" {
  Models::Model *mod_obj = NULL;
  void __init(void);
}

/* Bring in everything we need from the Models:: namespace. */
using Models::Model;            using Models::CallbackHandler;
using Models::MemOpHandler;     using Models::MemAccessType;
using Models::InstructionHandler;

using Models::callbackRequest;  using Models::getCycle;      
using Models::memRead;          using Models::memWrite;
using Models::memSize;

using Models::MEM_READ;         using Models::MEM_WRITE;

/*Implementation of our actual model class, in this case "FaultModel" */
class FaultModel: 
  public Model,              /* Required inheritance for all models. */
  public CallbackHandler,    /* Receives callback events. */
  public MemOpHandler,       /* Receives memory operation events. */
  public InstructionHandler  /* Receives instruction events. */
{
private:
  bool fault_active;
  uint64_t fault_addr;
  uint8_t original_data;
  uint8_t fault_data;

public:
  FaultModel(): CallbackHandler(), MemOpHandler(), InstructionHandler(), 
    fault_active(false) {
    puts("FaultModel constructor.");
    // Seed RNG with constant so predictable results can be obtained.
    srand(0xbec001);

    /* Schedule a callback for a random number of cycles into the simulation,
       between 0 and 10000. */
    callbackRequest(this, rand()%10000);
  }

  /* The callback event handler. */
  virtual void callback(int vm_idx, uint64_t cycle) {
    /* Schedule another callback a random number of cycles in the future. */
    callbackRequest(this, cycle + rand()%10000);

    /* Reverse the previous fault if it hasn't been overwritten. */
    if (fault_active && memRead(vm_idx, fault_addr) == fault_data) {
        memWrite(vm_idx, fault_addr, original_data);
    }
    fault_active = false;

    /* Create a memory fault (bit flip) */
    if (rand()%10 == 3) {
      fault_addr = rand()%memSize(vm_idx);
      fault_data = original_data = memRead(vm_idx, fault_addr);
      fault_data ^= 1<<(rand()%8);
      memWrite(vm_idx, fault_addr, fault_data);
      fault_active = true;
    }
  }

  /* The memory operation event handler. */
  virtual void memOp(
    int vm_idx, uint64_t vaddr, uint64_t paddr, uint8_t size, 
    MemAccessType type
  ) {

    /* If a write happens to the address of a fault, deactivate it. */
    if (
      fault_active && 
      fault_addr < paddr + size && 
      fault_addr >= paddr && 
      type == MEM_WRITE
    ) {
      fault_active = false;
    }

    /* If a read happens at the address of a fault, make it known. */
    if (fault_active && (type == MEM_READ) && 
	(fault_addr < paddr+size) && (fault_addr >= paddr)
    ) {
      printf("Faulty data at 0x%08lx has been read.\n", paddr);
    }
  }

  virtual void instruction(
    int vm_idx, uint64_t vaddr, uint64_t paddr, uint8_t size, uint8_t *inst
  ) {
    /* Could do some instruction stream based fault injection. */

    /* If an instruction is executed at the address of a fault, report it. */
    if (fault_active && (fault_addr < paddr+size) && (fault_addr >= paddr))
      printf("Instruction with data fault at 0x%08lx executed.\n", paddr);
      
  }
};

/* Module constructor and destructor implementations-- put any necessary
   initialization in the _object_ constructor-- just instantiate it here. */
void __init(void) {
  mod_obj = new FaultModel(); /* Propagate the instantiation. */
}

