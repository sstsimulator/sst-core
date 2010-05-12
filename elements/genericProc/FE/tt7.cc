// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2010, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.




// this file just provides an array of the names for the instruction
// opcodes defined in tt7.h

//: Instruction Names as strings
const char *instNames[] = { "NOP", 
			    "QUIT",
			    "UNKNOWN",
			    "TRAP",  
			    "ALU",    
			    "FP",    /* floating point */
			    "LOAD",
			    "STORE",
			    "JMP",     /* unconditional */
			    "BRANCH",   /* conditional */
			    "VEC",  
			    "BUBBLE",
			    "ISDEAD",
			    "LASTINST" };
