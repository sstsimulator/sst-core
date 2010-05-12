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
|  Sample Psst Model                                       | Chad D. Kersey  |
+----------------------------------------------------------+-----------------+
|  A simple model demonstrating some of the capabilities of the Qemu/Psst    |
|  processor model interface for SST. Note that this interface is currently  |
|  incomplete-- is has yet to acquire an iterface for serialization.         |
\---------------------------------------------------------------------------*/
#include <cstdio>
#include "model.h"

/* Comment out the following lines to see verbose output. */
//#define printf(x...) 
//#define puts(x) 

/* Module load/unload prototypes - standard boilerplate code. */
extern "C" {
  Models::Model *mod_obj = NULL;
  void __init(void);
}

/* Could just say "using namespace Models" but this way we are only "using" 
   what we need. */
using Models::Model;              using Models::CallbackHandler;
using Models::MemOpHandler;       using Models::MemAccessType;
using Models::InstructionHandler; using Models::MagicInstHandler;

using Models::callbackRequest;  using Models::getCycle;      
using Models::memRead;          using Models::memWrite;

using Models::ParamIt;          using Models::model_params;

/*Implementation of our actual model class, in this case "SampleModel" */
class SampleModel: 
  public Model,              /* Required inheritance for all models. */
  public CallbackHandler,    /* Receives callback events. */
  public MemOpHandler,       /* Receives memory operation events. */
  public InstructionHandler, /* Receives instruction events. */
  public MagicInstHandler    /* Receives CPUID instruction notification. */
{
private:
  int period;
public:
  SampleModel(): 
    CallbackHandler(), MemOpHandler(), InstructionHandler(), period(1) 
  {
    puts("SampleModel constructor.");

    /* Read configuration from parameters. */
    for (
      ParamIt i = (*model_params)->begin(); i != (*model_params)->end(); i++
    ) {
      if (i->first.compare("__sample_interval") == 0) 
        sscanf(i->second.c_str(), "%d", &period);
    }

    /* Schedule a callback for 10000 cycles into the simulation. */
    callbackRequest(this, 10000);
  }

  /* The callback event handler. */
  virtual void callback(int vm_idx, uint64_t cycle) {
    /* Schedule another callback 10000 cycles in the future. */
    callbackRequest(this, cycle + 10000);
  }

  /* The memory operation event handler. */
  virtual void memOp(
    int vm_idx, uint64_t vaddr, uint64_t paddr, uint8_t size, 
    MemAccessType type
  ) {
    /*Print out only this period's memory ops.*/
    static uint64_t i = 0; if (++i == period) i = 0; else return;
    printf(
       "%10lld: Got a mem op from vm %d, address 0x%08llx/%08llx: %d bytes\n",
       (long long)getCycle(vm_idx), vm_idx, (long long)vaddr, 
       (long long)paddr, (int)size
    );
  }

  virtual void instruction(
    int vm_idx, uint64_t vaddr, uint64_t paddr, uint8_t size, uint8_t *inst
  ) {
    /*Print out every millionth instruction, just to show we're here.*/
    static uint64_t local_icount = 0; local_icount++;
    static uint64_t i = 0; if (++i == period) i = 0; else return;
    printf(
      "%10lld(%lld): ", (long long)getCycle(vm_idx), 
      (long long)local_icount
    );
    printf("0x%08llx/0x%08llx:", (long long)vaddr, (long long)paddr);
    for (int i = 0; i < size; i++) printf(" %02x", inst[i]);
    printf("\n");
  }

  virtual void magicInst(int vm_idx, uint64_t rax) {
    printf("CPUID: rax=0x%08lx\n", (unsigned long)rax);
  }
};

/* Module constructor and destructor implementations-- put any necessary
   initialization in the _object_ constructor-- just instantiate it here. */
void __init(void) {
  mod_obj = new SampleModel();
}

