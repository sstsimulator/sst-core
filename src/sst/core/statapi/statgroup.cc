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
#include "sst/core/simulation_impl.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statengine.h"
#include "sst/core/statapi/statgroup.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"

#include <algorithm>

namespace SST::Statistics {

StatisticGroup::StatisticGroup(const ConfigStatGroup& csg, StatisticProcessingEngine* engine) :
    is_default(false),
    name(csg.name),
    output(const_cast<StatisticOutput*>(engine->getStatOutputs()[csg.outputID])),
    output_id(csg.outputID)
{
    if ( csg.outputFrequency.getValue() != 0 ) { // outputfreq = 0 by default
        output_freq =
            Simulation_impl::getSimulation()->getTimeLord()->getTimeConverter(csg.outputFrequency)->getFactor();
    }

    if ( !output->acceptsGroups() ) {
        Output::getDefaultObject().fatal(CALL_INFO, 1, "Statistic Output type %s cannot handle Statistic Groups\n",
            output->getStatisticOutputName().c_str());
    }

    // Need to keep track of components & stats that match with this group if not default
    if ( !is_default ) {
        components = csg.components;
        for ( auto& kv : csg.statMap ) {
            stat_names.push_back(kv.first);
        }
    }
}

StatisticGroup::~StatisticGroup()
{
    for ( auto& stat : stats ) {
        delete stat;
    }
}

void
StatisticGroup::restartGroup(StatisticProcessingEngine* engine)
{
    output = const_cast<StatisticOutput*>(engine->getStatOutputs()[output_id]);
    if ( !output->acceptsGroups() ) {
        Output::getDefaultObject().fatal(CALL_INFO, 1, "Statistic Output type %s cannot handle Statistic Groups\n",
            output->getStatisticOutputName().c_str());
    }
}


bool
StatisticGroup::containsStatistic(const StatisticBase* stat) const
{
    return std::find(stats.begin(), stats.end(), stat) != stats.end();
}

bool
StatisticGroup::claimsStatistic(const StatisticBase* stat) const
{
    if ( is_default ) return true;
    if ( std::find(stat_names.begin(), stat_names.end(), stat->getStatName()) != stat_names.end() ) {
        if ( std::find(components.begin(), components.end(), stat->getComponent()->getId()) != components.end() )
            return true;
    }

    return false;
}

void
StatisticGroup::addStatistic(StatisticBase* stat)
{
    stats.push_back(stat);
    stat->setGroup(this);
}

void
StatisticGroup::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(is_default);
    SST_SER(name);
    SST_SER(output_freq);
    SST_SER(output_id);
    SST_SER(components);
    SST_SER(stat_names);
    SST_SER(output);
    // stats vector is reconstructed on restart
}

} // namespace SST::Statistics
