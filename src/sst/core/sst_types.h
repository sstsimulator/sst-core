// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SST_TYPES_H
#define SST_CORE_SST_TYPES_H

//#include <sst_stdint.h>

#include <cstdint>

namespace SST {

typedef unsigned long   ComponentId_t;
typedef long   LinkId_t;
typedef uint64_t  Cycle_t;
typedef uint64_t  SimTime_t;
typedef double          Time_t;

#define MAX_SIMTIME_T 0xFFFFFFFFFFFFFFFFl
#define UNSET_COMPONENT_ID 0xFFFFFFFF
 
typedef double watts;
typedef double joules;
typedef double farads;
typedef double volts;

#ifndef LIKELY
#define LIKELY(x)       __builtin_expect((int)(x),1)
#define UNLIKELY(x)     __builtin_expect((int)(x),0)
#endif


} // namespace SST

#endif //SST_CORE_SST_TYPES_H
