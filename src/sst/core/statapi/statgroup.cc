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

#include "sst/core/statapi/statgroup.h"

#include "sst/core/baseComponent.h"
#include "sst/core/configGraph.h"
#include "sst/core/output.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statengine.h"
#include "sst/core/statapi/statgroup.h"
#include "sst/core/statapi/statoutput.h"

#include <algorithm>

namespace SST::Statistics {

StatisticGroup::StatisticGroup(const ConfigStatGroup& csg, StatisticProcessingEngine* engine) :
    isDefault(false),
    name(csg.name),
    output(const_cast<StatisticOutput*>(engine->getStatOutputs()[csg.outputID])),
    outputFreq(csg.outputFrequency),
    outputId(csg.outputID)
{

    if ( !output->acceptsGroups() ) {
        Output::getDefaultObject().fatal(
            CALL_INFO, 1, "Statistic Output type %s cannot handle Statistic Groups\n",
            output->getStatisticOutputName().c_str());
    }

    // Need to keep track of components & stats associated with this group if not default
    if ( !isDefault ) {
        components = csg.components;
        for ( auto& kv : csg.statMap ) {
            statNames.push_back(kv.first);
        }
    }
}

void
StatisticGroup::restartGroup(StatisticProcessingEngine* engine)
{
    output = const_cast<StatisticOutput*>(engine->getStatOutputs()[outputId]);
    if ( !output->acceptsGroups() ) {
        Output::getDefaultObject().fatal(
            CALL_INFO, 1, "Statistic Output type %s cannot handle Statistic Groups\n",
            output->getStatisticOutputName().c_str());
    }
}


bool
StatisticGroup::containsStatistic(const StatisticBase* stat) const
{
    if ( isDefault ) return true;
    return std::find(stats.begin(), stats.end(), stat) != stats.end();
}

bool
StatisticGroup::claimsStatistic(const StatisticBase* stat) const
{
    if ( isDefault ) return true;
    if ( std::find(statNames.begin(), statNames.end(), stat->getStatName()) != statNames.end() ) {
        if ( std::find(components.begin(), components.end(), stat->getComponent()->getId()) != components.end() )
            return true;
    }

    return false;
}

void
StatisticGroup::addStatistic(StatisticBase* stat)
{
    if ( !isDefault ) { // Structure unused (search is always true) if default group
        stats.push_back(stat);
    }
    stat->setGroup(this);
}

void
StatisticGroup::serialize_order(SST::Core::Serialization::serializer& ser)
{
    ser& isDefault;
    ser& name;
    ser& outputFreq;
    ser& outputId;
    ser& components;
    ser& statNames;
}

} // namespace SST::Statistics
