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

#include <stdio.h>
#include <pthread.h>
#include "ppcPimCalls.h"

pthread_t thr;
//pthread_mutex_t mut;
pthread_attr_t attr;

void *tFunc(void* a) {
  PIM_quickPrint(0,0,0);
  printf("test2\n"); fflush(stdout);
  return 0;
}

int main() {
  void *ret;

  printf("test\n"); fflush(stdout);

  pthread_create(&thr, 0, tFunc, 0);
  pthread_join(thr, &ret);
  PIM_quickPrint(1,1,1);

  return 0;
}
