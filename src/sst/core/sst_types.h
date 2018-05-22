// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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

typedef uint64_t  ComponentId_t;
typedef int32_t   LinkId_t;
typedef uint64_t  Cycle_t;
typedef uint64_t  SimTime_t;
typedef double          Time_t;

#define MAX_SIMTIME_T 0xFFFFFFFFFFFFFFFFl
/* Subcomponent IDs are in the high-12 bits of the Component ID */
#define UNSET_COMPONENT_ID 0xFFFFFFFFFFFFFFFFULL
#define COMPONENT_ID_BITS 48
#define COMPONENT_ID_MASK(x) ((x) & 0x0000FFFFFFFFFFFFULL)
#define SUBCOMPONENT_ID_BITS 16
#define SUBCOMPONENT_ID_MASK(x) ((x) >> COMPONENT_ID_BITS)
#define SUBCOMPONENT_ID_CREATE(compId, sCompId) ((((uint64_t)sCompId) << COMPONENT_ID_BITS) | compId)

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
