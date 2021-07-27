// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/statapi/statbase.h"

#include "sst/core/baseComponent.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/statapi/stataccumulator.h"
#include "sst/core/statapi/stathistogram.h"
#include "sst/core/statapi/statnull.h"
#include "sst/core/statapi/statoutputconsole.h"
#include "sst/core/statapi/statoutputcsv.h"
#include "sst/core/statapi/statoutputjson.h"
#include "sst/core/statapi/statoutputtxt.h"
#include "sst/core/statapi/statuniquecount.h"

namespace SST {
namespace Statistics {

StatisticBase::StatisticBase(
    BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams)
{
    m_statName   = statName;
    m_statSubId  = statSubId;
    m_component  = comp;
    m_statParams = statParams;

    initializeProperties();

    m_resetCountOnOutput = statParams.find<bool>("resetOnRead", false);
    m_clearDataOnOutput  = statParams.find<bool>("resetOnRead", false);
}

const std::vector<ElementInfoParam>&
StatisticBase::ELI_getParams()
{
    static std::vector<ElementInfoParam> empty;
    return empty;
}

const std::string&
StatisticBase::getCompName() const
{
    return m_component->getName();
}

void
Statistic<void>::outputStatisticFields(StatisticFieldsOutput* UNUSED(statOutput), bool UNUSED(EndOfSimFlag))
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(
        CALL_INFO, 1, "void statistic %s, type %s for component %s does not support outputing fields",
        getStatTypeName().c_str(), getFullStatName().c_str(), getComponent()->getName().c_str());
}

void
Statistic<void>::registerOutputFields(StatisticFieldsOutput* UNUSED(statOutput))
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(
        CALL_INFO, 1, "void statistic %s, type %s for component %s does not support outputing fields",
        getStatTypeName().c_str(), getFullStatName().c_str(), getComponent()->getName().c_str());
}

void
StatisticBase::incrementCollectionCount(uint64_t increment)
{
    m_currentCollectionCount += increment;
    checkEventForOutput();
}

void
StatisticBase::setCollectionCount(uint64_t newCount)
{
    m_currentCollectionCount = newCount;
    checkEventForOutput();
}

void
StatisticBase::resetCollectionCount()
{
    m_currentCollectionCount = 0;
}

void
StatisticBase::setCollectionCountLimit(uint64_t newLimit)
{
    m_collectionCountLimit = newLimit;
    checkEventForOutput();
}

std::string
StatisticBase::buildStatisticFullName(const char* compName, const char* statName, const char* statSubId)
{
    return buildStatisticFullName(std::string(compName), std::string(statName), std::string(statSubId));
}

std::string
StatisticBase::buildStatisticFullName(
    const std::string& compName, const std::string& statName, const std::string& statSubId)
{
    std::string statFullNameRtn;

    statFullNameRtn = compName + ".";
    statFullNameRtn += statName;
    if ( statSubId != "" ) {
        statFullNameRtn += ".";
        statFullNameRtn += statSubId;
    }
    return statFullNameRtn;
}

void
StatisticBase::initializeProperties()
{
    m_statFullName             = buildStatisticFullName(getCompName(), m_statName, m_statSubId);
    m_registeredCollectionMode = STAT_MODE_UNDEFINED;
    m_statEnabled              = true;
    m_outputEnabled            = true;
    m_currentCollectionCount   = 0;
    m_collectionCountLimit     = 100;
    m_resetCountOnOutput       = false;
    m_clearDataOnOutput        = false;
    m_outputAtEndOfSim         = true;
    m_outputDelayed            = false;
    m_collectionDelayed        = false;
    m_savedStatEnabled         = true;
    m_savedOutputEnabled       = true;
    m_outputDelayedHandler     = new OneShot::Handler<StatisticBase>(this, &StatisticBase::delayOutputExpiredHandler);
    m_collectionDelayedHandler
        = new OneShot::Handler<StatisticBase>(this, &StatisticBase::delayCollectionExpiredHandler);
}

void
StatisticBase::checkEventForOutput()
{
    if ( (m_registeredCollectionMode == STAT_MODE_COUNT) && (m_currentCollectionCount >= m_collectionCountLimit)
         && (1 <= m_collectionCountLimit) ) {
        // Dont output if CountLimit is zero
        StatisticProcessingEngine::getInstance()->performStatisticOutput(this);
    }
}

bool
StatisticBase::operator==(StatisticBase& checkStat)
{
    return (getFullStatName() == checkStat.getFullStatName());
}

void
StatisticBase::delayOutput(const char* delayTime)
{
    // Make sure only a single output delay is active
    if ( false == m_outputDelayed ) {

        // Save the Stat Output Enable setting and then disable the output
        m_savedOutputEnabled = m_outputEnabled;
        m_outputEnabled      = false;
        m_outputDelayed      = true;

        Simulation_impl::getSimulation()->registerOneShot(delayTime, m_outputDelayedHandler, STATISTICCLOCKPRIORITY);
    }
}

void
StatisticBase::delayCollection(const char* delayTime)
{
    // Make sure only a single collection delay is active
    if ( false == m_collectionDelayed ) {

        // Save the Stat Enable setting and then disable the Stat for collection
        m_savedStatEnabled  = m_statEnabled;
        m_statEnabled       = false;
        m_collectionDelayed = true;

        Simulation_impl::getSimulation()->registerOneShot(
            delayTime, m_collectionDelayedHandler, STATISTICCLOCKPRIORITY);
    }
}

void
StatisticBase::delayOutputExpiredHandler()
{
    // Restore the Output Enable to its stored value
    m_outputEnabled = m_savedOutputEnabled;
    m_outputDelayed = false;
}

void
StatisticBase::delayCollectionExpiredHandler()
{
    // Restore the Statistic Enable to its stored value
    m_statEnabled       = m_savedStatEnabled;
    m_collectionDelayed = false;
}

SST_ELI_INSTANTIATE_STATISTIC(AccumulatorStatistic, int32_t);
SST_ELI_INSTANTIATE_STATISTIC(AccumulatorStatistic, uint32_t);
SST_ELI_INSTANTIATE_STATISTIC(AccumulatorStatistic, int64_t);
SST_ELI_INSTANTIATE_STATISTIC(AccumulatorStatistic, uint64_t);
SST_ELI_INSTANTIATE_STATISTIC(AccumulatorStatistic, float);
SST_ELI_INSTANTIATE_STATISTIC(AccumulatorStatistic, double);

SST_ELI_INSTANTIATE_STATISTIC(HistogramStatistic, int32_t);
SST_ELI_INSTANTIATE_STATISTIC(HistogramStatistic, uint32_t);
SST_ELI_INSTANTIATE_STATISTIC(HistogramStatistic, int64_t);
SST_ELI_INSTANTIATE_STATISTIC(HistogramStatistic, uint64_t);
SST_ELI_INSTANTIATE_STATISTIC(HistogramStatistic, float);
SST_ELI_INSTANTIATE_STATISTIC(HistogramStatistic, double);

SST_ELI_INSTANTIATE_STATISTIC(UniqueCountStatistic, int32_t);
SST_ELI_INSTANTIATE_STATISTIC(UniqueCountStatistic, uint32_t);
SST_ELI_INSTANTIATE_STATISTIC(UniqueCountStatistic, int64_t);
SST_ELI_INSTANTIATE_STATISTIC(UniqueCountStatistic, uint64_t);
SST_ELI_INSTANTIATE_STATISTIC(UniqueCountStatistic, float);
SST_ELI_INSTANTIATE_STATISTIC(UniqueCountStatistic, double);

} // namespace Statistics
} // namespace SST
