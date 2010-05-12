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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ppcPimCalls.h"

const int N = 500;
int a[N];

int main() {
  int rank = PIM_readSpecial(PIM_CMD_PROC_NUM);
  //printf("hello world %d\n", rank);
  PIM_quickPrint(rank,rank,rank);

  if (rank == 0) {
    for (int i = 0; i < N; ++i) {
      a[i]=10+rank;
    }
  } else {
    for (int i = 0; i < 1000; ++i) {;}
    for (int i = 0; i < N; ++i) {
      PIM_quickPrint(rank, a[i], &a[i]);
      a[i]=10+rank;
    }
  }

  PIM_quickPrint(rank,rank,rank);
  for (int i = 0; i < 30000; ++i) {;}
}
