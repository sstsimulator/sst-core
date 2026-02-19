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

#include "sst/core/util/bit_util.h"

#include <cstdint>
#include <iosfwd>
#include <limits>
#include <ostream>
#include <utility>

namespace SST {

/**********************************************************************
 *  Common types used by SST
 **********************************************************************/
using ComponentId_t = uint64_t;
using StatisticId_t = uint64_t;
using LinkId_t      = uint64_t;
using HandlerId_t   = uint64_t;
using Cycle_t       = uint64_t;
using SimTime_t     = uint64_t;
using Time_t        = double;

/* PorModuleId_t is a <port name,index> pair*/
using PortModuleId_t = std::pair<std::string, size_t>;

// Macro to use when printing SimTime_t in printf or Output
#define PRI_SIMTIME PRIu64

// Flag used to indicate that a version of *EnableAll* was used to enable stats in an object
static constexpr StatisticId_t STATALL_ID = std::numeric_limits<StatisticId_t>::max();

constexpr SimTime_t MAX_SIMTIME_T = SST::bit_util::type_max<ComponentId_t>;

/**********************************************************************
 *  constexpr variable and inline functions used to build ID for
 *  generating the various IDs for elements
 *
 *  The Component and Statistic IDs use subfields in the following way:
 *  31:0  - Bottom 30 bits are the ID of the parent Component
 *  47:32 - The next 16 bits is the ID of a SubComponent with that
 *          component
 *  63:48 - The top 16 bits are unused for Component IDs, but they are
 *          used to uniquely identify a statistic with a (Sub)Component
 **********************************************************************/

/** Value used to indicate a Component's ID has not yet been set */
constexpr ComponentId_t UNSET_COMPONENT_ID = SST::bit_util::type_max<ComponentId_t>;

/** Value used to indicate a Statistic's ID has not yet been set */
constexpr StatisticId_t UNSET_STATISTIC_ID = SST::bit_util::type_max<StatisticId_t>;

/** Number of bits used for the parent component base ID */
constexpr size_t COMPONENT_ID_BITS = 32;

/**
   Gets the parent Component base ID from a general (i.e. either Component or SubComponent) ID

   @param complete_id The complete Component ID, including any SubComponent ID

   @return Parent Component base ID
*/
inline ComponentId_t
COMPONENT_ID_MASK(ComponentId_t complete_id)
{
    return complete_id & SST::bit_util::lower_mask<ComponentId_t, COMPONENT_ID_BITS>;
}

/** Number of bits used to store the ID of a SubComponent and part of the complete ID */
constexpr size_t SUBCOMPONENT_ID_BITS = 16;

/**
   Get the SubComponent portion of the Component ID

   @param complete_id The complete Component ID, including any SubComponent ID

   @return SubComponent portion of the Component ID
*/
inline ComponentId_t
SUBCOMPONENT_ID_MASK(ComponentId_t complete_id)
{
    return (complete_id >> COMPONENT_ID_BITS) & SST::bit_util::lower_mask<ComponentId_t, SUBCOMPONENT_ID_BITS>;
}

/**
   Create a component ID from the specified base Component ID and SubComponent ID

   @param comp_id Base ID of the parent Component

   @param sub_comp_id ID of the SubComponent with the Component

   @return Complete ID of the SubComponent
*/
inline ComponentId_t
SUBCOMPONENT_ID_CREATE(ComponentId_t comp_id, ComponentId_t sub_comp_id)
{
    return (sub_comp_id << COMPONENT_ID_BITS) | comp_id;
}

/** Number of bits in the complete Component ID */
constexpr size_t CONFIG_COMPONENT_ID_BITS = (COMPONENT_ID_BITS + SUBCOMPONENT_ID_BITS);

/**
   Gets the Complete Component ID (Component + SubComponent portions), masking out any Statistic ID that may be in the
   upper bits

   @param complete_id Complete ID (including any Statistic ID)

   @return Component portion of the ID with any Statistic ID masked out
*/
inline ComponentId_t
CONFIG_COMPONENT_ID_MASK(ComponentId_t complete_id)
{
    return complete_id & SST::bit_util::lower_mask<ComponentId_t, COMPONENT_ID_BITS + SUBCOMPONENT_ID_BITS>;
}

/**
   Create a Statistic ID from the complete ComponentID and ID of the Statistic within that Component

   @param comp_id Complete ID of the Component + SubComponent

   @param stat_id Statistic ID

   @return Combined ID for the Statistic
*/
inline StatisticId_t
STATISTIC_ID_CREATE(ComponentId_t comp_id, StatisticId_t stat_id)
{
    return (stat_id << CONFIG_COMPONENT_ID_BITS) | comp_id;
}

/**
   Determine whether a SubComponent is Component Defined (i.e. anonymous) or not

   @param complete_id Component Component ID for the SubComponent

   @return True if SubComponent is Anonymous, false otherwise
*/
inline bool
COMPDEFINED_SUBCOMPONENT_ID_MASK(ComponentId_t complete_id)
{
    return complete_id >> (sizeof(ComponentId_t) * 8 - 1);
}

/**
   Create a component ID from the specified base Component ID and SubComponent ID, with the component defined
   (i.e. anonymous) bit set.

   @param comp_id Base ID of the parent Component

   @param sub_comp_id ID of the SubComponent

   @return Complete ID for the SubComponent with the component defined bit set
*/
inline ComponentId_t
COMPDEFINED_SUBCOMPONENT_ID_CREATE(ComponentId_t comp_id, ComponentId_t sub_comp_id)
{
    return (sub_comp_id << COMPONENT_ID_BITS) | comp_id | SST::bit_util::upper_mask<ComponentId_t, 1>;
}

using watts [[deprecated("watts will be removed in SST 17")]]   = double;
using joules [[deprecated("joules will be removed in SST 17")]] = double;
using farads [[deprecated("farads will be removed in SST 17")]] = double;
using volts [[deprecated("volts will be removed in SST 17")]]   = double;

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

// Overload the << operator for SimulationRunMode
std::ostream& operator<<(std::ostream& os, const SimulationRunMode& mode);

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
