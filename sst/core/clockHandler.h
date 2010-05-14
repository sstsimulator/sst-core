// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CLOCKHANDLER_H
#define SST_CLOCKHANDLER_H

#include <sst/sst.h>
#include <sst/eventFunctor.h>

namespace SST {

typedef EventHandlerBase< bool, Cycle_t > ClockHandler_t;

} // namespace SST

#endif // SST_CLOCKHANDLER_H
