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

extern int errno;


int main(int argc, char **argv, char **envp) {
  int i = 0x12345678;
  char* p = (char*) &i;

  printf("argc %d\n", argc);
  printf("argv %p\n", argv);
  printf("envp %p\n", envp);
  printf("errno %d\n", errno);

  fflush(stdout);
}
