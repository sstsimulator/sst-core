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


#ifndef MALLOC_SYS_CALL_H
#define MALLOC_SYS_CALL_H

#include "global.h"
#include <map>
#include <string>

using namespace std;

class addrRegion {
 protected:
  simAddress baseAddr;
  simAddress maxAddr;

  map<simAddress, unsigned int> objectSizeInPages;

  unsigned char* pages;
  unsigned int pageSize;
  unsigned int pageShift;
  unsigned int pageMask;
  unsigned int totalPages;

  unsigned int makePageMask ();

  unsigned int numPages (unsigned int size) {
    unsigned int count = size >> pageShift;
    if ((size & pageMask) != 0)
      count++;
    return count;
  }

 public:
  string regionName;

 addrRegion(simAddress base, simAddress max, const char *name) :
     baseAddr(base), maxAddr(max),
     pages(NULL), pageSize(4096), pageShift(12), totalPages((max-base)/4096),
     regionName(name)
    {}

  unsigned int free (simAddress sa, unsigned int size, unsigned int& nextPage);
};

class processor;

class localRegionAlloc : public addrRegion {
  unsigned int numLocs;
  unsigned int *nextPage;

  map<simAddress, unsigned int> objectLoc;

  map<processor*, unsigned int> procLoc;

 public:
  unsigned int free (simAddress sa) { return free (sa, 0);}
  unsigned int free (simAddress sa, unsigned int size);

  void setup (unsigned int shift, unsigned int locs);

  void addLoc (processor *p, unsigned int loc) {
    procLoc[p] = loc;
  }

  int getLoc (processor *p) {
    if (procLoc.count(p) == 0) {
      printf ("Error, could not find loc for proc %p\n",p);
      return -1;
    }
    return procLoc[p];
  }

  simAddress addrOnLoc(unsigned int locID) {
    return ((locID << pageShift) + baseAddr);
  }
  unsigned int whichLoc(simAddress sa);
  unsigned int stride() { return (1 << pageShift); }
  unsigned int locs() {return numLocs;}
  simAddress allocate (unsigned int size, unsigned int locID);

  localRegionAlloc (simAddress base, simAddress max, const char* name) :
      addrRegion(base, max, name)
    { }

};

class vmRegionAlloc : public addrRegion {

 public:
  unsigned int nextPage;

 vmRegionAlloc (simAddress base, simAddress max, const char* name) :
  addrRegion(base, max, name), nextPage(0) {
    pageSize = 4096;
    pageShift = 12;
    totalPages = (max - base) / pageSize;
    pages = new unsigned char[totalPages];
    for (unsigned int i = 0; i < totalPages; i++)
      pages[i] = 'f';
    makePageMask();
  }
  
  unsigned int free (simAddress sa) { return free (sa, 0);}
  unsigned int free (simAddress sa, unsigned int size);

  simAddress allocate(unsigned int size);
};

#endif
