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


#include "ssb_DMA_fakeInst.h"

//: Negative One constant
//
// We make this a static int rather than a const because we need
// something with an address.
int fakeDMAInstruction::negOne = -1;
