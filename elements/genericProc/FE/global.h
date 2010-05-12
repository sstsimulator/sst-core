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


#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <sst_stdint.h>
#include <errno.h>
#include <arpa/inet.h>	    // for endianness
#include <sys/param.h>	    // for endianness

extern  bool    InitializeSimMaskToOpCodeTable();
const  int      HumanFormat = 0;

//: unsigned integer
typedef	unsigned	int	uint;
//: signed integer
typedef	signed		int	sint;
//: 64-bit integer
//typedef	long		long	int64;
typedef	    uint64_t  int64;

//: 64-bit unsigned integer
typedef uint64_t	uint64;
typedef unsigned	long	ulong;

//: 32-bit unsigned integer
typedef uint32_t	uint32;

//: 32-bit integer
typedef int32_t		int32;

//: unsigned 16-bit integer
typedef uint16_t	uint16;

//: 16-bit integer
typedef int16_t		int16;

//: unsigned 8-bit integer
typedef uint8_t		uint8;

//: 8-bit integer
typedef int8_t		int8;

//: 8-bit integer
typedef uint8_t		byte;

//: Simulation Address Datatype
//typedef unsigned int simAddress;    // 32-bit int
//typedef size_t simAddress;          // size_t=4 on 32bit, 8 on 64bit
typedef uint32_t simAddress;           // wcm:: simAddress = 32-bit field

//: Frame (register set) identifier
typedef simAddress frameID;

//: Simulation register datatype
typedef int32 simRegister;

//: Simulation Process ID
typedef simRegister simPID;

// from tt7.cc
extern const char *instNames[];

#ifndef	true
#	define	true	1
#endif

#ifndef	false
#	define	false	0
#endif


#endif
