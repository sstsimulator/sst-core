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
registerStatWithEngineOnRestart(SST::Statistics::StatisticBase* s)
{
    Simulation_impl::getSimulation()->getStatisticsProcessingEngine()->registerStatisticWithEngine(s);
}

} // namespace SST::Stat::pvt

namespace SST::Statistics {

StatisticBase::StatisticInfo StatisticBase::null_info_;

StatisticBase::StatisticBase(BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id,
    Params& stat_params, bool null_stat = false)
{
    component_ = comp;

    if ( null_stat ) {
        info_ = &null_info_;
        return;
    }

    /* Remaining initialization is only for non-null statistics */
    info_                  = new StatisticInfo();
    info_->stat_name_      = stat_name;
    info_->stat_sub_id_    = stat_sub_id;
    info_->stat_full_name_ = buildStatisticFullName(getCompName(), info_->stat_name_, info_->stat_sub_id_);

    /* Parameter: startat */
    try {
        UnitAlgebra startat = stat_params.find<UnitAlgebra>("startat", "0ns");
        if ( !startat.hasUnits("s") ) {
            Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
                "ERROR: Statistic %s - param 'startat' = '%s'; must be in units of seconds; exiting...\n",
                getFullStatName().c_str(), startat.toStringBestSI().c_str());
        }
        info_->start_at_time_ = startat;
    }
    catch ( UnitAlgebra::UnitAlgebraException& exc ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
            "ERROR: Statistic %s - param 'startat' = '%s'; Exception occured. %s\n", getFullStatName().c_str(),
            stat_params.find<std::string>("startat", "0ns").c_str(), exc.what());
    }

    /* Parameter: stopat */
    try {
        UnitAlgebra stopat = stat_params.find<UnitAlgebra>("stopat", "0ns");
        if ( !stopat.hasUnits("s") ) {
            Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
                "ERROR: Statistic %s - param 'stopat' = '%s'; must be in units of seconds; exiting...\n",
                getFullStatName().c_str(), stopat.toStringBestSI().c_str());
        }
        info_->stop_at_time_ = stopat;
    }
    catch ( UnitAlgebra::UnitAlgebraException& exc ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
            "ERROR: Statistic %s - param 'stopat' = '%s'; Exception occured. %s\n", getFullStatName().c_str(),
            stat_params.find<std::string>("stopat", "0ns").c_str(), exc.what());
    }

    /* Parameter: rate */
    try {
        UnitAlgebra rate        = stat_params.find<UnitAlgebra>("rate", "0ns");
        // units are error checked by BaseComponent::configureCollectionMode
        info_->collection_rate_ = rate;
    }
    catch ( UnitAlgebra::UnitAlgebraException& exc ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
            "ERROR: Statistic %s - param 'rate' = '%s'; Exception occured. %s\n", getFullStatName().c_str(),
            stat_params.find<std::string>("rate", "0ns").c_str(), exc.what());
    }

    /* Parameter: resetOnOutput */
    info_->clear_data_on_output_ = stat_params.find<bool>("resetOnOutput", false);
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
    return component_->getName();
}

DISABLE_WARN_MISSING_NORETURN
void
Statistic<void>::outputStatisticFields(StatisticFieldsOutput* UNUSED(stat_output), bool UNUSED(end_of_sim_flag))
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
        "void statistic %s, type %s for component %s does not support outputing fields", getStatTypeName().c_str(),
        getFullStatName().c_str(), getComponent()->getName().c_str());
}

void
Statistic<void>::registerOutputFields(StatisticFieldsOutput* UNUSED(stat_output))
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
        "void statistic %s, type %s for component %s does not support outputing fields", getStatTypeName().c_str(),
        getFullStatName().c_str(), getComponent()->getName().c_str());
}
REENABLE_WARNING

void
StatisticBase::incrementCollectionCount(uint64_t increment)
{
    info_->current_collection_count_ += increment;
    info_->output_collection_count_ += increment;
    checkEventForOutput();
}

void
StatisticBase::setCollectionCount(uint64_t new_count)
{
    info_->current_collection_count_ = new_count;
    info_->output_collection_count_  = new_count;
    checkEventForOutput();
}

void
StatisticBase::resetCollectionCount()
{
    info_->output_collection_count_ = 0;
}

void
StatisticBase::setCollectionCountLimit(uint64_t new_limit)
{
    info_->collection_count_limit_ = new_limit;
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
    std::string stat_full_name_rtn;

    stat_full_name_rtn = comp_name + ".";
    stat_full_name_rtn += stat_name;
    if ( stat_sub_id != "" ) {
        stat_full_name_rtn += ".";
        stat_full_name_rtn += stat_sub_id;
    }
    return stat_full_name_rtn;
}

void
StatisticBase::checkEventForOutput()
{
    if ( (info_->registered_collection_mode_ == STAT_MODE_COUNT) &&
         (info_->output_collection_count_ >= info_->collection_count_limit_) &&
         (1 <= info_->collection_count_limit_) ) {
        // Dont output if CountLimit is zero
        component_->getStatEngine()->performStatisticOutput(this);
    }
}

bool
StatisticBase::operator==(StatisticBase& check_stat)
{
    return (getFullStatName() == check_stat.getFullStatName());
}

void
StatisticBase::serialize_order(SST::Core::Serialization::serializer& ser)
{
    /* Only serialize info if stat is non-null */
    if ( !isNullStatistic() ) {
        // Need to serialize info_ as a non-pointer because on UNPACK,
        // it will have already been initiailized and serializing as a
        // pointer causes it to overwrite what is already there
        // (serializer assumes it is uninitialized and creates a new
        // one before calling serialize_order)
        SST_SER(*info_);
    }

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

StatisticBase::StatisticInfo::StatisticInfo()
{
    stat_name_      = "";
    stat_sub_id_    = "";
    stat_type_name_ = "";
    stat_full_name_ = "";
    stat_enabled_   = true;

    group_ = nullptr;

    output_enabled_        = true;
    reset_count_on_output_ = false;
    clear_data_on_output_  = false;
    output_at_end_of_sim_  = true;
    output_delayed_        = false;
    collection_delayed_    = false;
    saved_stat_enabled_    = true;
    saved_output_enabled_  = true;

    // Since there is an instance of this class that is statically
    // initialized, we can't use any units as they aren't initialized
    // until main()
    start_at_time_   = UnitAlgebra("0");
    stop_at_time_    = UnitAlgebra("0");
    collection_rate_ = UnitAlgebra("0");

    current_collection_count_   = 0;
    output_collection_count_    = 0;
    collection_count_limit_     = 0;
    registered_collection_mode_ = STAT_MODE_UNDEFINED;
}

void
StatisticBase::StatisticInfo::serialize_order(SST::Core::Serialization::serializer& ser)
{
    // stat_name_ serialized by StatisticBase
    // stat_sub_id_ serialized by StatisticBase
    // group_ recreated on restart
    SST_SER(stat_type_name_);
    SST_SER(stat_full_name_);
    SST_SER(current_collection_count_);
    SST_SER(output_collection_count_);
    SST_SER(collection_count_limit_);
    SST_SER(registered_collection_mode_);
    SST_SER(start_at_time_);
    SST_SER(stop_at_time_);
    SST_SER(collection_rate_);
    SST_SER(stat_enabled_);
    SST_SER(output_enabled_);
    SST_SER(reset_count_on_output_);
    SST_SER(clear_data_on_output_);
    SST_SER(output_at_end_of_sim_);
    SST_SER(output_delayed_);
    SST_SER(saved_stat_enabled_);
    SST_SER(saved_output_enabled_);
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
