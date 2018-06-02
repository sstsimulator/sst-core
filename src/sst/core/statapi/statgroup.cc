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

#include <sst_config.h>

#include <algorithm>

#include <sst/core/statapi/statgroup.h>
#include <sst/core/statapi/statbase.h>
#include <sst/core/statapi/statengine.h>
#include <sst/core/statapi/statoutput.h>
#include <sst/core/statapi/statgroup.h>
#include <sst/core/configGraph.h>
#include <sst/core/baseComponent.h>
#include <sst/core/output.h>

namespace SST {
namespace Statistics {


StatisticGroup::StatisticGroup(const ConfigStatGroup &csg) :
    isDefault(false), name(csg.name),
    output(const_cast<StatisticOutput*>(StatisticProcessingEngine::getInstance()->getStatOutputs()[csg.outputID])),
    outputFreq(csg.outputFrequency),
    components(csg.components)
{

    if ( !output->acceptsGroups() ) {
        Output::getDefaultObject().fatal(CALL_INFO, 1, "Statistic Output type %s cannot handle Statistic Groups\n", output->getStatisticOutputName().c_str());
    }

    for ( auto & kv : csg.statMap ) {
        statNames.push_back(kv.first);
    }
}


bool StatisticGroup::containsStatistic(const StatisticBase *stat) const
{
    if ( isDefault ) return true;
    std::find(stats.begin(), stats.end(), stat);
    return false;
}


bool StatisticGroup::claimsStatistic(const StatisticBase *stat) const
{
    if ( isDefault ) return true;
    if ( std::find(statNames.begin(), statNames.end(), stat->getStatName()) != statNames.end() ) {
        if ( std::find(components.begin(), components.end(), stat->getComponent()->getId()) != components.end() )
            return true;
    }

    return false;
}


void StatisticGroup::addStatistic(StatisticBase *stat)
{
    stats.push_back(stat);
    stat->setGroup(this);
}


} //namespace Statistics
} //namespace SST

