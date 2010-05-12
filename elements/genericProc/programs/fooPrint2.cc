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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ppcPimCalls.h"

int main() {
  PIM_quickPrint(0,0,0);

  volatile j = 0;
  int i =0;
  for (i = 0; i < 50000; ++i) {
    j++;
  } 
  
  //char c[] = "hello world\n";
  //malloc_printf("%p %p %p\n", c, &c[2], &c[11]);
  //malloc_printf("hello World %d %x\n", 5, 10);
  //write(1, c, sizeof(c));
}

#if 0
int main() {
    malloc_printf("Hello World\n");
    /*    printf("test %f\n", sin(3.0));
      printf("sqrt %f\n", sqrt(9.0));*/
    //return 0;
}
#endif
