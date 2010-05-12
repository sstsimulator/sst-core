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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

const int N = 60;

long myrand() {
  static long seed = 1234;
  long ret= (25214903917ll * seed + 11);
  seed = ret;
  return ret;
}

struct node {
  int val;
  node* next;
};

int *A, *B, *C;

int c(int i, int j)
{
  return i + (N*j);
}


int main() {
  printf("hello world %d, %d, %d\n", myrand(), myrand(), myrand());

  node *pn = 0;
  for (int i = 0; i < N*N; ++i) {
    node* n = (node*)malloc(sizeof(node));
    n->val = myrand();
    n->next = pn;
    pn = n;
  }

  int i = 0;
  while (pn) {
    if (pn->val > myrand())
      i += pn->val;
    pn = pn->next;
  }

  A = (int*)malloc(sizeof(*A) * N * N);
  B = (int*)malloc(sizeof(*B) * N * N);
  C = (int*)malloc(sizeof(*C) * N * N);

  for (i = 0; i < N*N; ++i) {
    A[i] = myrand();
    B[i] = myrand();
    C[i] = myrand();
  }

  for (i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      for (int k = 0; k < N; ++k) {
        C[c(i,k)] = A[c(i,k)] * B[c(k,j)] + myrand();
      }
    }
  }

}
