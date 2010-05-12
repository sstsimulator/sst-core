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

#include "ofile_print.h"

#include "mach-o/loader.h"

int ppcLoader::execFD=0;
vector<processor*> *ppcLoader::subProc=0;
vector<simPID> *ppcLoader::subPID=0;
processor *ppcLoader::theProc=0;

simAddress ppcLoader::constrLoc = 0;
simAddress ppcLoader::constrSize = 0;

#if BYTE_ORDER==LITTLE_ENDIAN
// need to figure this out - we load the state into the simulator
// memory in ppc-order, and then have to change the powerpc.def and
// related to convert before we ever use the data.

void notohlBuf(uint32_t *buf, size_t sz) {
  size_t items = sz >> 2;
  if ((items << 2) != sz) {
    printf("%s: buffer size not factor of 4!!\n", __FUNCTION__);    
  }
  for (size_t i = 0; i < items; ++i) {
    buf[i] = ntohl(buf[i]);
  }
}
#endif

//: MachO thread Command
//!SEC:ppcFront
struct my_thread_command {
  //: LC_THREAD or LC_UNIXTHREAD 
  uint32_t cmd;  
  //: total size of this command 
  uint32_t cmdsize;
  uint32_t flavor;            
  uint32_t count;
  struct ppc_thread_state state;
};
/* wcm: unsigned longs are suspect here for 64-bit comatibility,
 *      changing unsigned long to uint32_t here to retain 32-bit
 *      alignments.
 */


//: Load thread part of Mach-O
bool ppcLoader::loadUnixThread(const mach_header *mh, 
			       const thread_command* lcs,
			       const vector<ppcThread*> &p)
{
  const my_thread_command *ppcTC = (const my_thread_command*)lcs;
  if (ntohl(ppcTC->flavor) != PPC_THREAD_STATE) {
    printf(" Wrong thread flavor\n");
    return 0;
  } else if (ntohl(ppcTC->count) != PPC_THREAD_STATE_COUNT) {
    printf(" thread count wrong\n");
    return 0;
  } else {
    // init thread
    for (uint i = 0; i < p.size(); ++i) {
      p[i]->ProgramCounter = ppcTC->state.srr0;
      p[i]->setStack = 0; // indicate that they need to set their stacks
    }
    return 1;
  }
}

//: Copies directly to a memory
//
// Again, its slow, but I don't think its a signifigant bottleneck
// Used for copying TEXT segment 
bool ppcLoader::CopyToTEXT(simAddress dest, void* source, int Bytes)
{
#if 0
  uint8_t *BytePtr = (uint8_t*)source;
  simAddress bDestination = dest;
  bool working = 1;

  INFO("        Write TEXT: from %p to %p  -- %d Bytes\n", 
	  source, (void*)(size_t)dest, Bytes);
  while(working && Bytes) {
    working = ppcThread::WriteTextByte((simAddress)(size_t)bDestination,(uint8)*BytePtr);
    --Bytes;
    ++BytePtr;
    ++bDestination;
  }
  
  if (!working) {printf("problem writing to core\n");}
  return working;
#else
  return 1;
#endif
}

//: Load sections of MachO
//
// Copies everything to the data segment, and text to the text segment.
bool ppcLoader::loadSections(const mach_header *mh,
			     const segment_command *lc,
			     bool isExec,
			     const bool subset)
{
  section *sec = (section*)(lc+1);
  for (uint i = 0; i < ntohl(lc->nsects); ++i) {
    bool useExec = isExec;
    INFO("   sec: %s, %s\n", sec->sectname, sec->segname);
    
    // onlt the __text, __TEXT section should really be put in the
    // TEXT area.
    if (isExec && ((ntohl(sec->flags) & 0xff) != S_REGULAR)) {
      useExec = 0;
    }

    if (ntohl(sec->nreloc) > 0) {
      INFO("     Contains relocation entries!\n");
      return 0;
    }

    if (ntohl(sec->offset) != 0) {
      if (strncmp(sec->sectname, "__constructor", strlen("__constructor"))
	  == 0 ) {
	// detected the constructor list, a vector of funtion pointers
	// to run the static constructors.
	constrLoc = ntohl(sec->addr);
	constrSize = ntohl(sec->size) / sizeof(simRegister);
	INFO("     Found constructor list. location %#lx, %d entries\n",
	     (unsigned long)constrLoc, constrSize);
      }

      const char *memT[] = {"Data", "TEXT"};
      INFO("     Copying %u bytes from fileoff %u to vaddr %p-%p (%s)\n",
	   ntohl(sec->size), ntohl(sec->offset), 
	   (void*)(size_t)ntohl(sec->addr),
	   (void*)(((char*)ntohl(sec->addr))+ntohl(sec->size)), memT[useExec]);
	    // wcm: (char*)(size_t)sec->addr makes the warning go away, but
	    //      would that still be correct?  investigate this!
      uint8 *Data = new uint8[ntohl(sec->size)];
      
      lseek(execFD, ntohl(sec->offset), SEEK_SET);
      if(read(execFD, Data, ntohl(sec->size)) != (int)ntohl(sec->size)) {
	printf("error loading section of file\n");
	delete Data;
	return false ;
      }
      
      // We copy everything to the data segment, and some things to
      // the text segment
      bool ret;
      if (subset==0) {
	if (useExec||1) {
	  ret = ppcLoader::CopyToTEXT(ntohl(sec->addr), Data, ntohl(sec->size));
	}
	ret = theProc->LoadToSIM(ntohl(sec->addr), 0, Data, ntohl(sec->size));
      } else {
	INFO("      to subset:");
	//copy to each PID in the subset
	for(uint si = 0; si < subPID->size(); si++) {	  
	  simPID pid = (*subPID)[si];
	  INFO(" %u ", (unsigned int)pid);
	  ret = theProc->CopyToSIM(ntohl(sec->addr), pid, Data, 
				   ntohl(sec->size));
	}
	INFO("\n");
      }

      delete Data;
      if (!ret) {return ret;}
    } else {
      // I assume that if the fileoffset is zero, they just mean to
      // zero fill.
      INFO("     %u bytes at vaddr %p are zero'ed\n", ntohl(sec->size),
	   (void*)(size_t)ntohl(sec->addr));
    }
    sec++;
  }
  return 1;
}

//: Load MachO segment
//
// does some checking, and then passes on the loadSections
bool ppcLoader::loadSegment(const mach_header *mh,
			    const segment_command *lc,
			    const vector<ppcThread*> &p,
			    const bool subset) {
  INFO(" Segment: %s\n", lc->segname);
  if (ntohl(lc->nsects) == 0) {
    INFO("  Not Loaded\n");
    return 1;
  } else if (ntohl(lc->initprot) & VM_PROT_EXECUTE) {
    INFO("  Executable Section\n");
  } else {
    INFO("  Non-Executable section\n");
  }

  for (uint i = 0; i < p.size(); ++i) {

    if ( strncmp( lc->segname, "__TEXT", strlen("__TEXT") ) == 0 ) {
      p[i]->loadInfo.text_addr = ntohl(lc->vmaddr);
      p[i]->loadInfo.text_len = ntohl(lc->vmsize);
    }

    if ( strncmp( lc->segname, "__DATA", strlen("__DATA") ) == 0 ) {
      p[i]->loadInfo.data_addr = ntohl(lc->vmaddr);
      p[i]->loadInfo.data_len = ntohl(lc->vmsize);
    }
  }
  return loadSections(mh, lc, ntohl(lc->initprot) & VM_PROT_EXECUTE, subset);
}

//: Execute MachO load commands
//
// i.e. load segment, symtab (ignored), load threaed
bool ppcLoader::performLoadCommands(const mach_header *mh,
				    const load_command *lcs,
				    const vector<ppcThread*> &p,
				    const bool subset) {
  bool ret = 1;
  for (unsigned int i = 0; ret && i < mh->ncmds; ++i) {
    switch (ntohl(lcs->cmd)) {
    case LC_SEGMENT:
      INFO("Load: LC_SEGMENT\n");
      ret = loadSegment(mh, (const segment_command*)lcs, p, subset );
      break;
    case LC_SYMTAB:
      INFO("Load: LC_SYMTAB\n");
      INFO(" Not loading\n");
      break;
    case LC_UNIXTHREAD:
      INFO("Load: LC_UNIXTHREAD\n");
      ret = loadUnixThread(mh, (const thread_command*)lcs, p);
      break;
    case 0x1b:
      INFO("Load: LC_UUID\n");
      INFO(" Ignoring\n");
      break;
    default:
      printf("Unknown Load Command 0x%x\n",ntohl(lcs->cmd));
      return 0;
      break;
    }

    const char *lcsAddr = (const char*)lcs;
    lcsAddr += ntohl(lcs->cmdsize);
    lcs = (load_command*)lcsAddr;
  }
  return ret;
}

//:Initialize memory from a file descriptor
bool ppcLoader::LoadFromDevice (int		fd,
				const vector<ppcThread*> &p,
				processor *proc,
				char		**argv,
				char		**argp,
				bool subset)
{
  execFD = fd;
  ppcLoader::theProc = proc;

  // get the header
  mach_header mh;
  if(read(fd, &mh, sizeof(mh))!=sizeof(mh)) {
    printf("error loading Mach Header\n");
    return false ;
  } else {
#if BYTE_ORDER==LITTLE_ENDIAN
    mh.magic = ntohl(mh.magic);
    mh.cputype = ntohl(mh.cputype);
    mh.cpusubtype = ntohl(mh.cpusubtype);
    mh.filetype = ntohl(mh.filetype);
    mh.ncmds = ntohl(mh.ncmds);
    mh.sizeofcmds = ntohl(mh.sizeofcmds);
    mh.flags = ntohl(mh.flags);
#endif
    // check header
    if ( __print_info )
    print_mach_header(&mh, 1);
    if ((mh.flags & MH_NOUNDEFS) == 0) {
      printf("File has undefined references\n");
      return false;
    }
  }

  // get load commands
  load_command *lcs = (load_command*)malloc(mh.sizeofcmds);

  if(read(fd, lcs, mh.sizeofcmds) != (long)mh.sizeofcmds) {
    printf("<wcm> 1\n"); fflush(stdout); /* wcm */
    printf("error loading Mach load commands\n");
    return false ;
  } else {
    // print_loadcmds(&mh, lcs, 0, 0, 1, 1);
    if ((mh.flags & MH_DYLDLINK) == 0) {
      // printf("<wcm> 3\n"); fflush(stdout); /* wcm */
      // normal static linking
      bool ret = performLoadCommands(&mh, lcs, p, subset);
      if (!ret) {
	printf("could not perform load commands\n");
	return false ;
      }
    } else {
      printf("Cannot load dynamic binaries.\n");
      exit(1);
    }
  }

  free(lcs);
  return 1;
}


//:Initialize memory from a file
bool ppcLoader::LoadFromDevice (const char *Filename,
				const vector<ppcThread*> &p,
				processor *proc,
				char **argv,
				char **argp,
				bool subset)
{
  FILE	*fp = fopen(Filename, "rb");
  if(!fp) {
    printf("Error reading input file: %s\n", Filename);
    return ( false );
  }
  INFO("Loading input file: %s\n", Filename);

  bool	Ret = LoadFromDevice(fileno(fp), p, proc, argv, argp, subset);

  fclose(fp);
  if(!Ret) {
    printf("Could not load file!\n");
  }
  return Ret;
}

