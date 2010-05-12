// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, 2009, 2010 Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/* There are going to be 4 sections of memory
   Section 1: Global Text
   Section 2: Global Data
   Section 3: Stacks
   Section 4: Heap

   Sections 1 and 2 will come from the front end where the
   executable is loaded. Any loads and stores that come to
   an address which the front end identifies as being part
   of the loaded data will be translated into another address
   in the sim.

   Section 3 and 4 are separted for debugging/stat collecting purposes
   Threads can have stack boundaries that they shouldn't go
   outside of

   This will make the loaded executable able to be placed anywhere
   in memory, not just the addresses specified in the binary
*/

#include "memory.h"

simAddress memory::segRange[MemTypes][2] = {
  { 0, 0 }, /* Error */
  { 0,          0x03ffffff }, /* GlobalText */
  { 0x04000000, 0x0fffffff }, /* GlobalData */
  { 0x10000000, 0x5fffffff }, /* LocalDynamic */
  { 0x60000000, 0x7fffffff }, /* GlobalDynamic */
  };
  
const char *memory::segName[] = { "Error",
				  "GlobalText",
				  "GlobalData",
				  "LocalDynamic",
				  "GlobalDynamic" };


vmRegionAlloc memory::globalDynamic(memory::segRange[GlobalDynamic][0], 
				    memory::segRange[GlobalDynamic][1],
				    memory::segName[GlobalDynamic]);


localRegionAlloc memory::localDynamic(memory::segRange[LocalDynamic][0], 
				      memory::segRange[LocalDynamic][1],
				      memory::segName[LocalDynamic]);



memoryAccType
memory::getAccType(simAddress sa) {
  for (int i = 1; i < MemTypes; i++) {
    if ((sa >= segRange[i][0]) && (sa <= segRange[i][1]))
      return (memoryAccType)i;
  }
  return AddressError;
}

simAddress
memory::globalAllocate(unsigned int size) {
  if (size == 0)
    return 0;

  simAddress ret = globalDynamic.allocate(size);
  //printf ("ALLOC %x - %d\n", ret, size);
  return ret;
}

simAddress
memory::localAllocateNearAddr (unsigned int size, simAddress addr) {
  if (size == 0)
    return 0;

  simAddress ret =  localDynamic.allocate(size, localDynamic.whichLoc(addr));
  //printf ("LOCAL NEAR %x ALLOC %x - %d\n", addr, ret, size);
  return ret;
}

simAddress
memory::localAllocateAtID (unsigned int size, unsigned int ID) {
  if (size == 0)
    return 0;

  simAddress ret = localDynamic.allocate(size, ID);
  //printf ("LOCAL AT %d ALLOC %x - %d\n", ID, ret, size);
  return ret;
}

unsigned int 
memory::memFree(simAddress addr, unsigned int size) 
{
  unsigned ret = 555;

  switch (getAccType(addr)) {
  case LocalDynamic:
    ret = localDynamic.free(addr);
    break;
  case GlobalDynamic:
    ret = globalDynamic.free(addr, size);
    break;
  default:
    printf ("Error tried to free addr %d from segment %s\n", 
	    addr, segName[getAccType(addr)]);
    fflush(stdout);
    return 0;
  };

  //printf ("UN ALLOC %x - %d, ret %d\n", addr, size, ret);
  return(ret);  // wcm: bugfix?
}
