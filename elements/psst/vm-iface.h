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

#ifndef _VM_IFACE_H
/*----------------------------------------------------------+----------------\
| C VM Interface for Psst                                   | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| This is an interface that a VM module written in C (in this case Qemu) can |
| use to communicate with the Psst system. These functions are currently     |
| implemented in model.cpp.                                                  |
\---------------------------------------------------------------------------*/
#define _VM_IFACE_H

#include <sst_stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The VM can check the following to prevent having to send events
// unnecessarily. Eventually they'll be implemented.
static inline int memOps_enabled() { return 1; }
static inline int instructions_enabled() { return 1; }
static inline int magicInsts_enabled() { return 1; }

// The VM is responsible for _calling_ the following, to enqueue "events" for
// appropriate models to process:
void send_memOp(uint64_t vaddr, uint64_t paddr, uint8_t size, int type);
void send_instruction(
  uint64_t vaddress, uint64_t padress, uint8_t len, uint8_t inst[]
);
void send_magicInst(uint64_t rax);
void discard_instruction();

// The VM is responsible for _implementing_ the following asynchronous
// function calls from models:
uint8_t memRead(uint64_t addr);
void memWrite(uint64_t addr, uint8_t data);
uint64_t memSize();

#ifdef __cplusplus
}
#endif

#endif
