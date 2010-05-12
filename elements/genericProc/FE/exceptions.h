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


#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

//: Exceptions
typedef enum{ NO_EXCEPTION,
	      MOVE_TO_EXCEPTION,
	      FEB_EXCEPTION,
	      YIELD_EXCEPTION,
	      PROC_EXCEPTION,
	      LAST_EXCEPTION
} exceptType;

#endif

