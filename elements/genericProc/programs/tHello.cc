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

extern "C" void sst_libc_useSpawnToCoProc(int u);

void *foo(void *arg) {
  int *a = (int*)arg;

  printf("> thread %d\n", *a);

  return 0;
}

int main() {
  volatile int x = 0;

  printf("> threaded Hello World\n");

  // manually create a thread
  //PIM_threadCreate((void*)foo, 0);

  // pause
  for (int i = 0; i < 1000; ++i) {
    x++;
  }

  // use pthreads to create threads
  pthread_attr_t attr;  
  pthread_attr_init(&attr);

  sst_libc_useSpawnToCoProc(0);

  const int N = 10;
  pthread_t thr[N];
  int args[N];
  for (int i = 0; i < N; ++i) {
    args[i] = i+1;
    pthread_create(&thr[i], &attr, foo, &args[i]);
  }


  // pause
  for (int i = 0; i < 3000; ++i) {
    x++;
  }

  printf("> main done\n");

  return 0;
}
