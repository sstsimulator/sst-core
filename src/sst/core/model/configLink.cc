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

#include "sst_config.h"

#include "sst/core/model/configLink.h"

#include "sst/core/simulation_impl.h"
#include "sst/core/timeLord.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>

namespace SST {
std::map<std::string, uint32_t> ConfigLink::lat_to_index;

uint32_t
ConfigLink::getIndexForLatency(const char* latency)
{
    std::string lat(latency);
    uint32_t&   index = lat_to_index[lat];
    if ( index == 0 ) {
        // Wasn't there, set it to lat_to_index.size(), which is the
        // next index (skipping zero as that is the check for a new
        // entry)
        index = lat_to_index.size();
    }
    return index;
}

std::vector<SimTime_t>
ConfigLink::initializeLinkLatencyVector()
{
    TimeLord*              timeLord = Simulation_impl::getTimeLord();
    std::vector<SimTime_t> vec;
    vec.resize(lat_to_index.size() + 1);
    for ( auto& [lat, index] : lat_to_index ) {
        vec[index] = timeLord->getSimCycles(lat, __FUNCTION__);
    }
    return vec;
}

SimTime_t
ConfigLink::getLatencyFromIndex(uint32_t index)
{
    static std::vector<SimTime_t> vec = initializeLinkLatencyVector();
    return vec[index];
}

std::string
ConfigLink::latency_str(uint32_t index) const
{
    static TimeLord* timelord = Simulation_impl::getTimeLord();
    UnitAlgebra      tb       = timelord->getTimeBase();
    auto             tmp      = tb * latency[index];
    return tmp.toStringBestSI();
}

void
ConfigLink::setAsNonLocal(int which_local, RankInfo remote_rank_info)
{
    // First, if which_local != 0, we need to swap the data for index
    // 0 and 1
    if ( which_local == 1 ) {
        std::swap(component[0], component[1]);
        std::swap(port[0], port[1]);
        std::swap(latency[0], latency[1]);
    }

    // Now add remote annotations.  Rank goes in component[1], thread
    // goes in latency[1], port[1] is not needed
    component[1] = remote_rank_info.rank;
    latency[1]   = remote_rank_info.thread;
    port[1].clear();

    nonlocal = true;
}

void
ConfigLink::updateLatencies()
{
    // Need to clean up some elements before we can test for zero latency
    if ( order >= 1 ) {
        latency[0] = ConfigLink::getLatencyFromIndex(latency[0]);
    }
    // If this is nonlocal, latency[1] holds the remote thread instead of the latency
    if ( order >= 2 && !nonlocal ) {
        latency[1] = ConfigLink::getLatencyFromIndex(latency[1]);
    }
}

} // namespace SST
