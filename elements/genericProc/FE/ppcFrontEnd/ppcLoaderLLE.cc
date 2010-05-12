// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2007, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "global.h"
#include "ppcMachine.h"
#include "ppcLoader.h"
#include "processor.h"
#include "ppcFront.h"
#include <unistd.h>
#include <string.h>

/*
 *  LLE file format:
 *
 *    1st word: startPC
 *    2+: <regions>
 *
 *      <region>:
 *        1st: vmaddr
 *        2nd: size (bytes)
 *        3nd: protection
 *        4+ : data
 */

#define VM_PROT_READ    ((vm_prot_t) 0x01)      /* read permission */
#define VM_PROT_WRITE   ((vm_prot_t) 0x02)      /* write permission */
#define VM_PROT_EXECUTE ((vm_prot_t) 0x04)      /* execute permission */

//: LLE Region description
//!SEC:ppcFront
struct LLERegionHeader {
  //: Starting Virtual address
  simAddress vmAddr;
  //: Size
  // in bytes
  uint size;
  //: Protection
  uint protection;
};

FILE* ppcLoader::lle = 0;
char* ppcLoader::copyBuf = 0;
uint ppcLoader::bufSize = 0;

//: Read LLE Region
bool ppcLoader::readRegion() {
  LLERegionHeader hdr;
  
  // get LLE region header
  size_t r = fread(&hdr, sizeof(hdr), 1, lle);
  if (r != 1) {
    if (feof(lle)) {
      // EOF. don't panic
      return 1;
    } else {
      printf("error reading region header\n");
      return 0;
    }
  }

  bool readP = hdr.protection & VM_PROT_READ;
  bool writeP = hdr.protection & VM_PROT_WRITE;
  bool execP = hdr.protection & VM_PROT_EXECUTE;
  if (hdr.size > 4096) {
    printf(" got region @ %#x-%#x. size=%u protection=%c%c%c\n", 
	   hdr.vmAddr, hdr.vmAddr + hdr.size,
	   hdr.size, readP ? 'r' : '-', writeP ? 'w' : '-', execP ? 'x' : '-');
  } 
  if (writeP == 0) {
    // wcm: 64b -- casting here is suspect: simAddress should be 32-bit,
    // so casting pointers to it are dangerous when compiling serialProto 
    // in 64-bit mode. : may need to relook at this.
    ppcThread::constData.push_back(ppcThread::adrRange((simAddress)hdr.vmAddr,
	    (simAddress)(size_t)((char*)hdr.vmAddr)+hdr.size));
    // wcm: this cast to size_t is necessary for ppc, but will kick out
    //      a warning on x86 linux.  We need to look at this and do
    //      something that is more graceful for everybody.
  }

  // resize buffer if needed
  if (bufSize < hdr.size) {
    printf(" resizing copyBuf to %d\n", hdr.size);
    copyBuf = (char*)realloc(copyBuf, hdr.size);
    if (copyBuf == 0) {
      printf("error reallocing copyBuf to %d bytes\n", hdr.size);
      return 0;
    }
    bufSize = hdr.size;
  }

  // read in the data
  r = fread(copyBuf, 1, hdr.size, lle);
  if (r != hdr.size) {
    printf("error reading region data\n");
    return 0;
  }

  if (ppcThread::usingMagicStack() && hdr.vmAddr == 0xbf800000) {
    printf("  not loading stack\n");
  } else {
    // copy to simulator & text memory
    r = ppcLoader::CopyToTEXT(hdr.vmAddr, copyBuf, hdr.size); 
    if (r == 1)
      r = processor::LoadToSIM(hdr.vmAddr, 0, copyBuf, hdr.size);
    
    if (!r) {
      printf("copy/load to Text/SIM failed\n");
    }
  }

  return r;
}

//: Get LLE Header
bool ppcLoader::getHeader(simAddress *startPC, simRegister regs[32]) {
  // get startPC
  size_t r = fread(startPC, 4, 1, lle);
  if (r != 1) {
    printf("error reading LLE header.");
    return 0;
  } else {
    printf(" StartPC = %p\n", (void*)(size_t)startPC);
  }
  // get registers
  for (int i = 0; i < 32; ++i) {
    size_t r2 = fread(&regs[i], 4, 1, lle);
    if (r2 != 1) {
      printf("error reading LLE header (regs).");
      return 0;
    } else {
      printf(" r%2d = %8lx", i, regs[i]);
      if ((i % 3) == 2) printf("\n");
    }
  }
  printf("\n");

  return 1;
}

//: Initialize threads, the LLE Way
bool ppcLoader::initLLEThreads(const simAddress startPC,
			       const simRegister regs[32],
			       const vector<ppcThread*> &p) {
  for (uint i = 0; i < p.size(); ++i) {
    p[i]->ProgramCounter = startPC;
    if (ppcThread::usingMagicStack()) {
      p[i]->setStack = 0; // indicate that they need to set their stacks      
    } else {
      p[i]->setStack = 1; // indicate that it doens't need to set its stack
      // set the registers
      for (int r = 0; r < 32; ++r) {
	p[i]->packagedRegisters[r] = regs[r];
      }
    }
  }
  return 1;
}

//: Load LLE
//
// Gets the header, reads the regions, and inits the threads.
bool ppcLoader::LoadLLE(const char *Filename,
			const vector<ppcThread*> &p,
			char **argv,
			char **argp) {
  printf("Attempting to read Linked&Loaded Executable (lle) %s\n", Filename);

  lle = fopen(Filename, "r");
  if(!lle) {
    printf("Error reading input file: %s\n", Filename);
    return ( false );
  }
  
  //get header info
  simAddress startPC;
  simRegister initRegs[32];
  bool ret = getHeader(&startPC, initRegs);

  // read regions
  int nReg = 0;
  while(ret && feof(lle) == 0) {
    nReg++;
    ret = readRegion();    
  }
  printf("%d regions read\n", nReg);

  // init threads
  ret = initLLEThreads(startPC, initRegs, p);

  if (copyBuf) free(copyBuf);
  bufSize = 0;
  fclose(lle);
  if(!ret) {
    printf("Could not load file!\n");
  }
  return ret;
}
