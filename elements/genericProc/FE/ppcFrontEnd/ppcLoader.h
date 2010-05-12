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


#ifndef PPCLOADER_H_
#define PPCLOADER_H_

#include "ppcFront.h"

#include "mach-o/loader.h"

//#define NULL 0


//:Loads PPC/Mach-O executable into memory
//!SEC:ppcFront
class ppcLoader
{
  static char* copyBuf;
  static uint bufSize;
  static FILE *lle;
  static int  execFD;
  static bool CopyToTEXT(simAddress dest, void* source, int Bytes);
  static bool loadUnixThread(const mach_header *mh, 
			     const thread_command* lcs,
			     const vector<ppcThread*> &p);
  static bool performLoadCommands(const mach_header *Mh,
				  const load_command *lcs,
				  const vector<ppcThread*> &p,
				  const bool subset);
  static bool loadSegment(const mach_header *mh,
			  const segment_command *lcs,
			  const vector<ppcThread*> &p,
			  const bool subset);
  static bool loadSections(const mach_header *mh,
			   const segment_command *lcs,
			   bool isExec,
			   const bool subset);
  static bool getHeader(simAddress*, simRegister[32]);
  static bool readRegion();
  static bool initLLEThreads(const simAddress, const simRegister[32],
			     const vector<ppcThread*> &p);
public:
  //: location of constructor section of the header
  static simAddress constrLoc;
  //: length of constructor section
  static simAddress constrSize;
  //: subset info
  static vector<processor*> *subProc;
  //: subset info
  static vector<simPID> *subPID;
  //: processor to load
  static processor *theProc;
  static bool LoadFromDevice(int	fd,
			     const vector<ppcThread*>	&p,
			     processor *proc,
			     char	**argv = NULL,
			     char	**argp = NULL,
			     bool subset=0);
  static bool LoadFromDevice(const char	*Filename,
			     const vector<ppcThread*>	&p,
			     processor *proc,
			     char	**argv = NULL,
			     char	**argp = NULL,
			     bool subset=0);
  static bool LoadLLE(const char	*Filename,
		      const vector<ppcThread*>	&p,
		      char	**argv = NULL,
		      char	**argp = NULL);
};

#endif

