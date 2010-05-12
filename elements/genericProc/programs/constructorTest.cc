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
#include "ppcPimCalls.h"

int x = 0;

struct A {
  A() {x++;}
};

struct B {
  B() {x++;}
};

A a;
B b;

/*asm (".globl .constructors_used");
  asm (".constructors_used = 0");*/

/*
void _runConstructors() {
  unsigned int size;
  int loc = PIM_readSpecial_2(PIM_CMD_GET_CTOR, &size);
  void (**init_routines) (void);
  printf("%d constructors @ %p\n", size, (void*)loc);
  init_routines = (void (**) (void)) loc;
  for (int i = 0; i < size; ++i) {
    printf("constructor %d: %p\n", i, init_routines[i]);
    (*init_routines[i])();
  }
}
*/

int main() {
  //_runConstructors();
  
  printf("constructors run: %d (should be 2)\n", x);

  return 0;
}
