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
  printf("hello world\n");

  int r = open("/Users/afrodri/Desktop/foo", O_CREAT|O_WRONLY, S_IRWXU);
  printf("ret %d\n", r);
  r = close(r);
  printf("ret %d\n", r);
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
