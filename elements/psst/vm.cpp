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
| Psst VM Class                                             | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| This is a C++ class wrapper for the (C) VM module.                         |
\---------------------------------------------------------------------------*/
#include <dlfcn.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <vector>

#include "vm.h"

#define TMPFILE_PREFIX "/tmp/psst_"

using namespace std;

// Make a temporary copy of a shared object and then dlopen() the temp file.
static void *copy_and_load(const char* file, const char* target_file) {
  size_t len = strlen(target_file) + strlen(file) + 5;
  char *cp_line = new char[len];
  snprintf(cp_line, len, "cp %s %s", file, target_file);
  if (system(cp_line) == -1) {
    fprintf(stderr, "\"%s\" failed.\n", cp_line);
    exit(1);
  }

  void *ret = dlopen(target_file, RTLD_NOW|RTLD_LOCAL);
  if (ret == NULL) {
    fprintf(stderr, "dlopen(\"%s\") failed:\n  %s\n", target_file, dlerror());
    exit(1);
  }

  delete[] cp_line;

  return ret;
}

template <typename T> void dlsympp(T *&ret, void* lib, const char* sym) {
  (void*&)ret = dlsym(lib, sym);
  if (char* err = dlerror()) {
    fprintf(stderr, "dlsym(\"%s\") failed:\n %s\n", err);
  }
}


Vm::Vm(const char* filename, const int argc, const char** argv, long freq) :
  argc(argc), argv(argv), ipc(1.0), cycle(0), clock_freq(freq)
{
  // Make a temporary copy of the file, so that opening multiple instances 
  // will succeed and not cause globals to interfere.
  static int max_filename = 0;
  target_filename = new char[23];
  snprintf(target_filename, 23, TMPFILE_PREFIX "%08d", max_filename++);
  handle = copy_and_load(filename, target_filename);

  // Get function pointers to VM initialization/run functions
  dlsympp(main, handle, "main");
  dlsympp(run, handle, "vm_run");

  // Pass clock frequency to VM module.
  long *clock; 
  dlsympp(clock, handle, "clock_freq");
  *clock = clock_freq;

  // Get function pointers to all of the VM access calls.
  dlsympp(memRead, handle, "memRead");
  dlsympp(memWrite, handle, "memWrite");
  dlsympp(memSize, handle, "memSize");

  // Run modified main to init VM (this will return)
  main(argc, argv);

  // Add self to vector of VMs.
  vms->insert(vms->end(), this);
}

Vm::~Vm() {
  unlink(target_filename);
  delete target_filename;
}
