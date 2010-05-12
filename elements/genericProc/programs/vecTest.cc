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

#include "ppcPimCalls.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

int foo = 0;

void coFunc(void *arg) {
  PIM_quickPrint(0,0,0);

  int *fooPtr = &foo;
  asm volatile ("li r3, 0\n" 
                "mr r4, %0\n" 
		"lvx v0, r3, r4\n"
                : 
                : "r" (fooPtr)
                : "r3", "r4");


}

int a = 5;

int main() {
  int b = 0;
  int i;

  PIM_spawnToCoProc(PIM_NIC, (void*)coFunc, &a);

  for (i = 0; i < 30000; ++i) {
    b++;
  }

  return 0;
}
