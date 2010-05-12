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



#ifndef ENVIRON_H
#define ENVIRON_H

#include <string>
#include <global.h>
#include <memory.h>
#include <processor.h>
using namespace std;

extern int environInit(string cfgStr, processor *proc,
				simAddress addr, int len );
#define ENVIRON_SIZE           (0x1000>>1)
#define ENVIRON_ARG_STR_OFFSET (0x0200>>1)
#define ENVIRON_ARG_STR_SIZE   (0x0700>>1)
#define ENVIRON_ENV_STR_OFFSET (0x0900>>1)
#define ENVIRON_ENV_STR_SIZE   (0x0700>>1)

#define MIN_STACK_SIZE         (0x1000>>1)

#endif
