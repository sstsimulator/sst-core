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
//#include "ppcPimCalls.h"
#include <malloc/malloc.h>
#include <strings.h>


int main(int argc, char **argv, char **envp) {
  /*int i = 0x12345678;
    char* p = (char*) &i;*/

  //printf("test\n");

  char b[100];
  for (int i = 0; i < 100; ++i) b[i] = i;
  bzero(b, 100);
  for (int i = 0; i < 100; ++i) {
    if (b[i] != 0) {
      ;//printf("%c\n", b[i]);
    }
  }

#if 0
  FILE*  f = fopen( "/ws/bench/cth/cth/spy/fonts/default.ttf", "rb" );

  int ret = fseek( f, 0, SEEK_END );

  fclose(f);
#endif

#if 0
  int r = malloc_zone_check(0);
  printf("check %d\n\n", r); fflush(stdout);
  //PIM_quickPrint(0,0,0);

  for (int z = 0; z < 100; ++z) {
    char* x = malloc(rand() % 1024*1);
    if (rand() % 10 < 5) 
      free(x);
    r = malloc_zone_check(0);
    printf("check %d\n\n", r); fflush(stdout);
    if (!r) {
      exit(1);
    }
  }
  //PIM_quickPrint(0,0,0);
  /*x = malloc(80); 
  printf("X = %p\n", x);  fflush(stdout);
  free(x);*/
  r = malloc_zone_check(0);
  printf("check %d\n\n", r); fflush(stdout);
#endif

#if 0
  printf("argc %d\n", argc);
  printf("argv %p\n", argv);
  printf("envp %p\n", envp);
  for (int x = 0; x < argc; ++x) {
    printf("arg %d: %s (%p)\n", x, argv[x], argv[x]);
  }
  for (int x = 0; x < 3; ++x) {
    printf("env %d: %s (%p)\n", x, envp[x], envp[x]);
  }

  for (int z = 0; z < 100; ++z) {
    if (p == argv[0]) printf("*");
    if (*p >= ' ' && *p <= '~') {
      printf("%c", *p);
    } else {
      printf("(%2x)", *p & 0xff);
    }
    fflush(stdout);
    p++;
  }
  printf("\n");

  char* home = getenv("HOME");
  printf("home %s\n", home);
#endif
  fflush(stdout);
}
