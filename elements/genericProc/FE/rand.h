// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SIM_RAND_H
#define SIM_RAND_H

#define RAND_MAX_NUM 0x7fffffff

namespace SimRand {
  static int __attribute__((unused))
  do_rand(unsigned long *ctx)
  {
    /*
     * Compute x = (7^5 * x) mod (2^31 - 1)
     * wihout overflowing 31 bits:
     *      (2^31 - 1) = 127773 * (7^5) + 2836
     * From "Random number generators: good ones are hard to find",
     * Park and Miller, Communications of the ACM, vol. 31, no. 10,
     * October 1988, p. 1195.
     */
    long hi, lo, x;
    
    /* Can't be initialized with 0, so use another value. */
    if (*ctx == 0)
      *ctx = 123459876;
    hi = *ctx / 127773;
    lo = *ctx % 127773;
    x = 16807 * lo - 2836 * hi;
    if (x < 0)
      x += 0x7fffffff;
    return ((*ctx = x) % ((unsigned long)RAND_MAX_NUM + 1));
  }

  extern unsigned long next;

  static unsigned int __attribute__((unused)) rand() { return do_rand(&SimRand::next); }
};

#endif
