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

/*---------------------------------------------------------+----------------\
| Simple byte queue for Psst: Unit Test                    | Chad D. Kersey |
+----------------------------------------------------------+----------------+
| This simple program ensures that the queue interface operates according   |
| to expectations, and gives an example of usage. By randomly alternating   |
| between reading and writing, this                                         |
\--------------------------------------------------------------------------*/
#include <cstdio>
#include <cstdlib>
#include "queue.h"

Queue q(256);
Queue r(256); /* The "size queue", sizes of contents of q. */

int main() {
  srand(0);

  for (int i = 0; i < 1000000; i++) {
    if (rand() % 2) {
      /*Try to read.*/
      try {
        int s;
        r.get(s);
        uint8_t i; uint16_t j; uint32_t k; uint64_t l;
        unsigned long long m;
	switch (s) {
	  case 0: q.get(i); m = i; break;
	  case 1: q.get(j); m = j; break;
	  case 2: q.get(k); m = k; break;
          case 3: q.get(l); m = l; break;
        }
        printf(
          "Read %d bits: 0x%llx (%dB free)\n", 1<<(3+s), m, (int)q.space()
        );
      }
      catch (queue_underflow q) { puts("Queue underflow."); }
    } else {
      /*Try to write.*/
      int suc = 1, size = rand() % 4;
      uint64_t data = ((uint64_t)rand() << 32) | rand();
      if (size < 3) data &= (((uint64_t)1 << (8 << size)) - 1);
      if (r.space() < sizeof(int)) { 
        puts("Could not write; size queue is full.");
        suc = 0;
      } else try { switch (size) {
        case 0: q.put((uint8_t)data); break;
        case 1: q.put((uint16_t)data); break;
        case 2: q.put((uint32_t)data); break;
        case 3: q.put((uint64_t)data); break;
      } } catch (queue_overflow q) { suc = 0; }
      if (suc) { 
         r.put(size);
         printf(
            "Wrote %d bits: 0x%llx (%dB free)\n", 
            1<<(3+size), (unsigned long long)data, (int)q.space()
         );
      } else { printf("Failed to write %d bytes.\n", 1<<(3+size)); }
      
    }
  }

  return 0;
}

