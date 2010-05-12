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
#include <malloc/malloc.h>

int main() {
  printf("hello world\n");
  fflush(stdout);
}


#if 1
extern "C" {
/*
 * To avoid linking in libm.  These variables are defined as they are used in
 * pthread_init() to put in place a fast sqrt().
 */
size_t hw_sqrt_len = 0;

double
sqrt(double x)
{
        return(0.0);
}

double
hw_sqrt(double x)
{
        return(0.0);
}
}
#endif
