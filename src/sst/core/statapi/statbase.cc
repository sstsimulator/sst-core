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

namespace SST::Stat::pvt {

void
registerStatWithEngineOnRestart(
    SST::Statistics::StatisticBase* stat, SimTime_t start_factor, SimTime_t stop_factor, SimTime_t output_factor)
{
    Simulation_impl::getSimulation()->getStatisticsProcessingEngine()->reregisterStatisticWithEngine(
        stat, start_factor, stop_factor, output_factor);
}

} // namespace SST::Stat::pvt

namespace SST::Statistics {

StatisticBase::StatisticBase(
    BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params)
{
    component_ = comp;

    stat_name_   = stat_name;
    stat_sub_id_ = stat_sub_id;

    /* Parameter: resetOnOutput */
    clear_data_on_output_ = stat_params.find<bool>("resetOnOutput", false);
}

const std::vector<ElementInfoParam>&
StatisticBase::ELI_getParams()
{
    static std::vector<ElementInfoParam> empty;
    return empty;
}

const std::string
StatisticBase::getCompName() const
{
    return component_->getName() + stat_mod_name_;
}

DISABLE_WARN_MISSING_NORETURN
void
Statistic<void>::outputStatisticFields(StatisticFieldsOutput* UNUSED(stat_output), bool UNUSED(end_of_sim_flag))
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
        "void statistic %s, type %s does not support outputing fields", getFullStatName().c_str(),
        getStatTypeName().c_str());
}

void
Statistic<void>::registerOutputFields(StatisticFieldsOutput* UNUSED(stat_output))
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
        "void statistic %s, type %s does not support outputing fields", getFullStatName().c_str(),
        getStatTypeName().c_str());
}
REENABLE_WARNING

void
StatisticBase::incrementCollectionCount(uint64_t increment)
{
    current_collection_count_ += increment;
    output_collection_count_ += increment;
    checkEventForOutput();
}

void
StatisticBase::setCollectionCount(uint64_t new_count)
{
    current_collection_count_ = new_count;
    output_collection_count_  = new_count;
    checkEventForOutput();
}

void
StatisticBase::resetCollectionCount()
{
    output_collection_count_ = 0;
}

void
StatisticBase::setCollectionCountLimit(uint64_t new_limit)
{
    collection_count_limit_ = new_limit;
    checkEventForOutput();
}

std::string
StatisticBase::buildStatisticFullName(const char* comp_name, const char* stat_name, const char* stat_sub_id)
{
    return buildStatisticFullName(std::string(comp_name), std::string(stat_name), std::string(stat_sub_id));
}

std::string
StatisticBase::buildStatisticFullName(
    const std::string& comp_name, const std::string& stat_name, const std::string& stat_sub_id)
{
    std::string stat_full_name_rtn = comp_name + ".";
    stat_full_name_rtn += stat_name;
    if ( stat_sub_id != "" ) {
        stat_full_name_rtn += '.';
        stat_full_name_rtn += stat_sub_id;
    }
    return stat_full_name_rtn;
}

void
StatisticBase::checkEventForOutput()
{
    if ( (!registered_collection_mode_) && (output_collection_count_ >= collection_count_limit_) &&
         (1 <= collection_count_limit_) ) {
        // Dont output if CountLimit is zero
        Simulation_impl::getSimulation()->getStatisticsProcessingEngine()->performStatisticOutput(this);
    }
}

bool
StatisticBase::operator==(StatisticBase& check_stat)
{
    return (getFullStatName() == check_stat.getFullStatName());
}

SimTime_t
StatisticBase::getStartAtFactor()
{
    return Simulation_impl::getSimulation()->getStatisticsProcessingEngine()->getStatisticStartTimeFactor(this);
}

SimTime_t
StatisticBase::getStopAtFactor()
{
    return Simulation_impl::getSimulation()->getStatisticsProcessingEngine()->getStatisticStopTimeFactor(this);
}

SimTime_t
StatisticBase::getOutputRateFactor()
{
    return getGroup() ? getGroup()->output_freq : 0;
}


void
StatisticBase::serialize_order(SST::Core::Serialization::serializer& ser)
{
    /* Only serialize info if stat is non-null - this should be skipped for null stats anyways */
    if ( !isNullStatistic() ) {
        SST_SER(stat_type_name_); // Deprecated
        SST_SER(stat_mod_name_);
        SST_SER(flags_);
        SST_SER(current_collection_count_);
        SST_SER(output_collection_count_);
        SST_SER(collection_count_limit_);
        SST_SER(registered_collection_mode_);
        SST_SER(stat_enabled_);
        SST_SER(reset_count_on_output_);
        SST_SER(clear_data_on_output_);
        SST_SER(output_at_end_of_sim_);


        /* Store/restore data type */
        if ( ser.mode() != SST::Core::Serialization::serializer::UNPACK ) {
            std::string name(StatisticFieldInfo::getFieldTypeShortName(stat_data_type_));
            SST_SER(name);
        }
        else {
            std::string name;
            SST_SER(name);
            stat_data_type_ = StatisticFieldTypeBase::getField(name.c_str());
        }
    }
}

void
StatisticBase::serializeStat(SST::Core::Serialization::serializer& ser)
{
    std::string    stat_name = getStatName();
    std::string    stat_id   = getStatSubId();
    BaseComponent* comp      = getComponent();
    SimTime_t      factor    = getOutputRateFactor();
    SST_SER(comp);
    SST_SER(stat_name);
    SST_SER(stat_id);
    SST_SER(factor);
    this->serialize_order(ser);

    // Get state stored at stat engine if needed, must be after stat serialization
    // because we need to read flags first to determine if these fields exist
    if ( getStartAtFlag() ) {
        factor = getStartAtFactor();
        SST_SER(factor);
    }
    if ( getStopAtFlag() ) {
        factor = getStopAtFactor();
        SST_SER(factor);
    }
}

BaseComponent*
StatisticBase::deserializeComponentPtr(SST::Core::Serialization::serializer& ser)
{
    BaseComponent* comp = nullptr;
    SST_SER(comp);
    return comp;
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

} // namespace SST::Statistics
