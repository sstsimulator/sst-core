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

#include "sst/core/config.h"

#include "sst/core/component.h"
#include "sst/core/configGraph.h"
#include "sst/core/factory.h"
#include "sst/core/from_string.h"
#include "sst/core/model/configComponent.h"
#include "sst/core/namecheck.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeLord.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>

namespace SST {

void
ConfigStatistic::addParameter(const std::string& key, const std::string& value, bool overwrite)
{
    bool bk = params.enableVerify(false);
    params.insert(key, value, overwrite);
    params.enableVerify(bk);
}

bool
ConfigStatGroup::addComponent(ComponentId_t id)
{
    if ( std::find(components.begin(), components.end(), id) == components.end() ) {
        components.push_back(id);
    }
    return true;
}

bool
ConfigStatGroup::addStatistic(const std::string& name, Params& p)
{
    statMap[name] = p;
    if ( outputFrequency.getValue() == 0 ) {
        /* aka, not yet really set to anything other than 0 */
        setFrequency(p.find<std::string>("rate", "0ns"));
    }
    return true;
}

bool
ConfigStatGroup::setOutput(size_t id)
{
    outputID = id;
    return true;
}

bool
ConfigStatGroup::setFrequency(const std::string& freq)
{
    UnitAlgebra uaFreq(freq);
    if ( uaFreq.hasUnits("s") || uaFreq.hasUnits("hz") ) {
        outputFrequency = uaFreq;
        return true;
    }
    return false;
}

std::pair<bool, std::string>
ConfigStatGroup::verifyStatsAndComponents(const ConfigGraph* graph)
{
    for ( auto id : components ) {
        const ConfigComponent* comp = graph->findComponent(id);
        if ( !comp ) {
            std::stringstream ss;
            ss << "Component id " << id << " is not a valid component";
            return std::make_pair(false, ss.str());
        }
        for ( auto& statKV : statMap ) {

            bool ok = Factory::getFactory()->GetStatisticValidityAndEnableLevel(comp->type, statKV.first) != 255;

            if ( !ok ) {
                std::stringstream ss;
                ss << "Component " << comp->name << " does not support statistic " << statKV.first;
                return std::make_pair(false, ss.str());
            }
        }
    }

    return std::make_pair(true, "");
}
} // namespace SST
