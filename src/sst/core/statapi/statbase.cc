// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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
#include "sst/core/statapi/statoutputcsv.h"
#include "sst/core/statapi/statoutputjson.h"
#include "sst/core/statapi/statoutputtxt.h"
#include "sst/core/statapi/statuniquecount.h"

namespace SST {

namespace Stat {
namespace pvt {

void
registerStatWithEngineOnRestart(SST::Statistics::StatisticBase* s)
{
    Simulation_impl::getSimulation()->getStatisticsProcessingEngine()->registerStatisticWithEngine(s);
}

} // namespace pvt
} // namespace Stat

namespace Statistics {


StatisticBase::StatisticBase(
    BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams)
{
    m_statName   = statName;
    m_statSubId  = statSubId;
    m_component  = comp;
    m_statParams = statParams;

    initializeProperties();

    m_clearDataOnOutput = statParams.find<bool>("resetOnOutput", false);
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
    m_outputCollectionCount += increment;
    checkEventForOutput();
}

void
StatisticBase::setCollectionCount(uint64_t newCount)
{
    m_currentCollectionCount = newCount;
    m_outputCollectionCount  = newCount;
    checkEventForOutput();
}

void
StatisticBase::resetCollectionCount()
{
    m_outputCollectionCount = 0;
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
    m_outputCollectionCount    = 0;
    m_collectionCountLimit     = 100;
    m_resetCountOnOutput       = false;
    m_clearDataOnOutput        = false;
    m_outputAtEndOfSim         = true;
    m_outputDelayed            = false;
    m_collectionDelayed        = false;
    m_savedStatEnabled         = true;
    m_savedOutputEnabled       = true;
}

void
StatisticBase::checkEventForOutput()
{
    if ( (m_registeredCollectionMode == STAT_MODE_COUNT) && (m_outputCollectionCount >= m_collectionCountLimit) &&
         (1 <= m_collectionCountLimit) ) {
        // Dont output if CountLimit is zero
        m_component->getStatEngine()->performStatisticOutput(this);
    }
}

bool
StatisticBase::operator==(StatisticBase& checkStat)
{
    return (getFullStatName() == checkStat.getFullStatName());
}

// GV_TODO: Potentially remove this. It doesn't work as intended and delays should be user-set, not component-set
void
StatisticBase::delayOutput(const char* delayTime)
{
    // Make sure only a single output delay is active
    if ( false == m_outputDelayed ) {

        // Save the Stat Output Enable setting and then disable the output
        m_savedOutputEnabled = m_outputEnabled;
        m_outputEnabled      = false;
        m_outputDelayed      = true;

        Simulation_impl::getSimulation()->registerOneShot(
            delayTime, new OneShot::Handler<StatisticBase>(this, &StatisticBase::delayOutputExpiredHandler),
            STATISTICCLOCKPRIORITY);
    }
}

// GV_TODO: Potentially remove this. It doesn't work as intended and delays should be user-set, not component-set
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
            delayTime, new OneShot::Handler<StatisticBase>(this, &StatisticBase::delayCollectionExpiredHandler),
            STATISTICCLOCKPRIORITY);
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

void
StatisticBase::serialize_order(SST::Core::Serialization::serializer& ser)
{
    ser& m_component; // BaseComponent*
    ser& m_statName;
    ser& m_statSubId;
    ser& m_statFullName;
    ser& m_statParams; // OPTIMIZATION: Don't need all of these for recreation
    ser& m_registeredCollectionMode;
    ser& m_currentCollectionCount;
    ser& m_outputCollectionCount;
    ser& m_collectionCountLimit;
    ser& m_statDataType; // TODO: Need to store type name instead & lookup ID on return -> may differ if custom types
                         // exist
    ser& m_statEnabled;
    ser& m_outputEnabled;
    ser& m_resetCountOnOutput;
    ser& m_clearDataOnOutput;
    ser& m_outputAtEndOfSim;
    ser& m_outputDelayed;
    ser& m_savedStatEnabled;
    ser& m_savedOutputEnabled;
    // ser& m_group; // This will get re-created on restart
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
