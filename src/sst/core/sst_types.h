// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SST_TYPES_H
#define SST_CORE_SST_TYPES_H

#include <cstdint>
#include <limits>

namespace SST {

using ComponentId_t = uint64_t;
using StatisticId_t = uint64_t;
using LinkId_t      = uint32_t;
using HandlerId_t   = uint64_t;
using Cycle_t       = uint64_t;
using SimTime_t     = uint64_t;
using Time_t        = double;

#define PRI_SIMTIME PRIu64

static constexpr StatisticId_t STATALL_ID = std::numeric_limits<StatisticId_t>::max();

#define MAX_SIMTIME_T 0xFFFFFFFFFFFFFFFFl

/* Subcomponent IDs are in the high-16 bits of the Component ID */
#define UNSET_COMPONENT_ID                      0xFFFFFFFFFFFFFFFFULL
#define UNSET_STATISTIC_ID                      0xFFFFFFFFFFFFFFFFULL
#define COMPONENT_ID_BITS                       32
#define COMPONENT_ID_MASK(x)                    ((x)&0xFFFFFFFFULL)
#define SUBCOMPONENT_ID_BITS                    16
#define SUBCOMPONENT_ID_MASK(x)                 ((x) >> COMPONENT_ID_BITS)
#define SUBCOMPONENT_ID_CREATE(compId, sCompId) ((((uint64_t)sCompId) << COMPONENT_ID_BITS) | compId)
#define CONFIG_COMPONENT_ID_BITS                (COMPONENT_ID_BITS + SUBCOMPONENT_ID_BITS)
#define CONFIG_COMPONENT_ID_MASK(x)             ((x)&0xFFFFFFFFFFFFULL)
#define STATISTIC_ID_CREATE(compId, statId)     ((((uint64_t)statId) << CONFIG_COMPONENT_ID_BITS) | compId)
#define COMPDEFINED_SUBCOMPONENT_ID_MASK(x)     ((x) >> 63)
#define COMPDEFINED_SUBCOMPONENT_ID_CREATE(compId, sCompId) \
    ((((uint64_t)sCompId) << COMPONENT_ID_BITS) | compId | 0x8000000000000000ULL)

using watts  = double;
using joules = double;
using farads = double;
using volts  = double;

#ifndef LIKELY
#define LIKELY(x)   __builtin_expect((int)(x), 1)
#define UNLIKELY(x) __builtin_expect((int)(x), 0)
#endif

enum class SimulationRunMode {
    UNKNOWN, /*!< Unknown mode - Invalid for running */
    INIT,    /*!< Initialize-only.  Useful for debugging initialization and graph generation */
    RUN,     /*!< Run-only.  Useful when restoring from a checkpoint (not currently supported) */
    BOTH     /*!< Default.  Both initialize and Run the simulation */
};

/**
   Struct used as a base class for all AttachPoint metadata passed to
   registration functions.  Needed so that dynamic cast can be used
   since different tools may pass different metadata through the
   AttachPoints.
 */
struct AttachPointMetaData
{
    AttachPointMetaData() {}
    virtual ~AttachPointMetaData() {}
};

} // namespace SST

#endif // SST_CORE_SST_TYPES_H
