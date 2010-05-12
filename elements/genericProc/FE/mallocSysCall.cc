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


#include <assert.h>
#include "mallocSysCall.h"

#if 0
#define DBprintf(...) { printf(__VA_ARGS__); fflush(stdout); }
#else
#define DBprintf(...) ;
#endif


unsigned int vmRegionAlloc::free(simAddress sa, unsigned int size) {
  unsigned int ret = addrRegion::free(sa, size, nextPage);
  DBprintf ("local unalloc %x pages %d\n", sa, ret);
  return ret;
}

unsigned int localRegionAlloc::free(simAddress sa, unsigned int size) {
  if (objectLoc.count(sa) == 0) {
    fprintf (stderr, "Error: tried to free object %x no loc record\n", sa);
    return 0;
  }
  unsigned int loc = objectLoc[sa];
  objectLoc.erase(sa);
  unsigned int ret = addrRegion::free(sa, size, nextPage[loc]);
  DBprintf ("local unalloc %x pages %d\n", sa, ret);
  return ret;
}

unsigned int addrRegion::free(simAddress addr, unsigned int size, unsigned int& nextPage) {
  unsigned int pageC = numPages(size);
  if (size == 0) {
    if (objectSizeInPages.count(addr) == 0) {
      fprintf (stderr, "Tried to fast free pointer %x not fast malloced\n", addr);
      return 0;
    }
    pageC = objectSizeInPages[addr];
    objectSizeInPages.erase(addr);
  }
  else if (pageC == objectSizeInPages[addr])
    objectSizeInPages.erase(addr);
  
  unsigned int addrPage = (addr - baseAddr) >> pageShift;
  DBprintf ("%s: Deallocating %d pages at addr %x start page %d\n",
	  regionName.c_str(), pageC, addr, addrPage);

  for (unsigned int i = 0; i < pageC; i++) {
    pages[addrPage + i] = 'f';
    DBprintf ("unMark page %d addr %x - %x\n", addrPage + i,
	      ((addrPage + i) << pageShift) + baseAddr,
	      ((addrPage + i + 1) << pageShift) + baseAddr - 1);
  }

  nextPage = addrPage;
  return pageC;
}

void localRegionAlloc::setup (unsigned int shift, unsigned int locs)
{
    assert(locs > 0);
    assert(shift > 0);
    pageShift = shift;
    pageSize = 1<<shift;
    numLocs = locs;
    totalPages = (maxAddr - baseAddr) / pageSize;
    pages = new unsigned char[totalPages];
    assert(pages != NULL);
    for (unsigned int i = 0; i < totalPages; i++)
        pages[i] = 'f';
    nextPage = new unsigned int[numLocs];
    assert(nextPage != NULL);
    unsigned int curloc = whichLoc(baseAddr);
    printf("baseAddr is on loc %u\n", curloc);
    for (unsigned int i = 0; i < numLocs; i++) {
        nextPage[i] = curloc++;
        curloc *= (curloc < numLocs);
    }
    makePageMask();
    printf ("Set up local region %s for %d location with chunk size %d bytes\n",
            regionName.c_str(), numLocs, 1 << pageShift);
}

unsigned int localRegionAlloc::whichLoc(simAddress sa) {
  assert(numLocs != 0);
  DBprintf ("Finding loc for addr %x\n", sa);
  DBprintf ("Hash Shift is %d -> %x\n", pageShift, sa >> pageShift);
  DBprintf ("Num locs is %d -> %x\n", numLocs, (sa >> pageShift) % numLocs);

  return ((sa >> pageShift) % numLocs); 
}

unsigned int addrRegion::makePageMask () {
  pageMask = 0;
  for (unsigned int i = 0; i < pageShift; i++) {
    pageMask = pageMask << 1;
    pageMask |= 1;
  }
  return 0;
}

simAddress localRegionAlloc::allocate(unsigned int size, unsigned int locID) {
    unsigned int pageC = numPages(size);

    DBprintf ("%s: allocate %d bytes - %d pages (%d total)\n", regionName.c_str(),
	    size, pageC, totalPages);

    if (pageC > 1) {
	printf("%s: Cannot allocate %d bytes local, max bytes = %d\n",
		regionName.c_str(), size, pageSize);
	return 0;
    }

    if (locID > numLocs) {
	printf ("%s: Error: request for local alloc loc %d (only %d locs)\n",
		regionName.c_str(), locID, numLocs);
	return 0;
    }

    DBprintf ("Trying to get a page on loc %d\n", locID);
    assert(nextPage != NULL);
    if (nextPage[locID] == UINT_MAX) {
	fprintf (stderr, "local alloc failed, out of pages\n");
	return 0;
    }

    int addrPage = nextPage[locID];
    DBprintf ("Addr page is %d\n", addrPage);

    if (pages[addrPage] != 'f') {
	printf ("local alloc %d failed, next page %d was marked %c\n", 
		locID, addrPage, pages[addrPage]);
	exit(1);
    }

    simAddress addr = (addrPage << pageShift) + baseAddr;
    objectSizeInPages[addr] = 1;
    objectLoc[addr] = locID;
    DBprintf ("Assign page at loc %d is %d addr %x @ %d\n", locID, addrPage, addr, whichLoc(addr));
    if (whichLoc(addr) != locID) {
	printf ("Error, wrong addr %x calc for loc %d\n", addr, locID);
	exit(1);
    }

    pages[addrPage] = 'm';

    /* So the idea here is to search through pages (our record of allocated (m)
     * and free (f) status for the "pages" (i.e. contiguous chunks of memory)
     * until we find a free (f) page. The pages array is ALL contiguous pages,
     * with all nodes assumed to be interleaved regularly (XXX: please, Lord, let
     * this be true). Thus, to find the next page that belongs to this locID,
     * we must increment by numLocs. */
    unsigned int locPage = locID; // start at the beginning
    while (pages[locPage] != 'f') {
	if (locPage + numLocs >= totalPages) {
	    fprintf (stderr, "Loc %d has run out of pages...\n", locID);
	    nextPage[locID] = UINT_MAX;
	    return addr;
	} else {
	    locPage += numLocs; // go to next (maybe this should be more complex)
	}
    }

    DBprintf ("Set next empty page at loc %d to page %d %c addr %x @ %d\n",
	    locID, locPage, pages[locPage], (locPage << pageShift) + baseAddr,
	    whichLoc((locPage << pageShift) + baseAddr));
    nextPage[locID] = locPage;

    return (addr);
}

simAddress vmRegionAlloc::allocate(unsigned int size) {
    unsigned int pageC = numPages(size);
    unsigned int addrPage = nextPage;

    DBprintf ("%s: allocate %d bytes - %d pages (%d total)\n", regionName.c_str(),
	    size, pageC, totalPages);

    if (pageC > totalPages)
	return 0;

    unsigned int pages_so_far = 0;
    for (unsigned int i = 0; i < totalPages; i++) {
	if (pages[nextPage] != 'f') {
	    pages_so_far = 0;
	    addrPage = ((nextPage+1) >= totalPages) ? 0 : nextPage+1;
	}
	else {
	    pages_so_far++;
	    if (pages_so_far == pageC) {
		for (unsigned int j = 0; j < pageC; j++) {
		    pages[addrPage+j] = 'm';
		}
		simAddress addr = (addrPage << pageShift) + baseAddr;

		//simAddress lastAddr = ((addrPage + pageC) << pageShift) + baseAddr;
		DBprintf ("%s: found %d total pages at page %d addr %x - %x size %d\n", 
			regionName.c_str(), pageC, addrPage, addr,
			lastAddr, lastAddr - addr);
		objectSizeInPages[addr] = pageC;
		return (addr);
	    }
	}
	nextPage = ((nextPage+1) >= totalPages) ? 0 : nextPage+1;
    }
    return 0;
}
