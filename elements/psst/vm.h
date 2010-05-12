// Copyright 2009 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _VM_H
/*----------------------------------------------------------+----------------\
| Psst VM Class                                             | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| A C++ wrapper for the VM module, which uses a C API.                       |
\---------------------------------------------------------------------------*/
#define _VM_H

#include <vector>

class Vm {
public:
  Vm(const char* path, const int argc, const char** argv, long freq);	
  ~Vm();
  int argc;
  const char **argv;

  //Function pointers into VM, set by constructor.
  int (*main)(int, const char **);
  int (*run)(int instructions, double ipc, uint64_t start_cycle);

  uint8_t (*memRead)(uint64_t addr);
  void (*memWrite)(uint64_t addr, uint8_t data);
  uint64_t (*memSize)(void);

  long clock_freq;
  double ipc;
  uint64_t cycle;

private:
  void *handle;
  char *target_filename;
};

extern std::vector<Vm*> *vms;

#endif
