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

StatisticBase::StatisticBase(
    BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params,
    bool null_stat = false)
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
            Simulation_impl::getSimulation()->getSimulationOutput().fatal(
                CALL_INFO, 1, "ERROR: Statistic %s - param 'startat' = '%s'; must be in units of seconds; exiting...\n",
                getFullStatName().c_str(), startat.toStringBestSI().c_str());
        }
        info_->start_at_time_ = startat;
    }
    catch ( UnitAlgebra::UnitAlgebraException& exc ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(
            CALL_INFO, 1, "ERROR: Statistic %s - param 'startat' = '%s'; Exception occured. %s\n",
            getFullStatName().c_str(), stat_params.find<std::string>("startat", "0ns").c_str(), exc.what());
    }

    /* Parameter: stopat */
    try {
        UnitAlgebra stopat = stat_params.find<UnitAlgebra>("stopat", "0ns");
        if ( !stopat.hasUnits("s") ) {
            Simulation_impl::getSimulation()->getSimulationOutput().fatal(
                CALL_INFO, 1, "ERROR: Statistic %s - param 'stopat' = '%s'; must be in units of seconds; exiting...\n",
                getFullStatName().c_str(), stopat.toStringBestSI().c_str());
        }
        info_->stop_at_time_ = stopat;
    }
    catch ( UnitAlgebra::UnitAlgebraException& exc ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(
            CALL_INFO, 1, "ERROR: Statistic %s - param 'stopat' = '%s'; Exception occured. %s\n",
            getFullStatName().c_str(), stat_params.find<std::string>("stopat", "0ns").c_str(), exc.what());
    }

    /* Parameter: rate */
    try {
        UnitAlgebra rate        = stat_params.find<UnitAlgebra>("rate", "0ns");
        // units are error checked by BaseComponent::configureCollectionMode
        info_->collection_rate_ = rate;
    }
    catch ( UnitAlgebra::UnitAlgebraException& exc ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(
            CALL_INFO, 1, "ERROR: Statistic %s - param 'rate' = '%s'; Exception occured. %s\n",
            getFullStatName().c_str(), stat_params.find<std::string>("rate", "0ns").c_str(), exc.what());
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

void
Statistic<void>::outputStatisticFields(StatisticFieldsOutput* UNUSED(stat_output), bool UNUSED(end_of_sim_flag))
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(
        CALL_INFO, 1, "void statistic %s, type %s for component %s does not support outputing fields",
        getStatTypeName().c_str(), getFullStatName().c_str(), getComponent()->getName().c_str());
}

void
Statistic<void>::registerOutputFields(StatisticFieldsOutput* UNUSED(stat_output))
{
    Simulation_impl::getSimulation()->getSimulationOutput().fatal(
        CALL_INFO, 1, "void statistic %s, type %s for component %s does not support outputing fields",
        getStatTypeName().c_str(), getFullStatName().c_str(), getComponent()->getName().c_str());
}

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

// GV_TODO: Potentially remove this. It doesn't work as intended and delays should be user-set, not component-set
void
StatisticBase::delayOutput(const char* delay_time)
{
    // Make sure only a single output delay is active
    if ( false == info_->output_delayed_ ) {

        // Save the Stat Output Enable setting and then disable the output
        info_->saved_output_enabled_ = info_->output_enabled_;
        info_->output_enabled_       = false;
        info_->output_delayed_       = true;

        Simulation_impl::getSimulation()->registerOneShot(
            delay_time, new OneShot::Handler<StatisticBase>(this, &StatisticBase::delayOutputExpiredHandler),
            STATISTICCLOCKPRIORITY);
    }
}

// GV_TODO: Potentially remove this. It doesn't work as intended and delays should be user-set, not component-set
void
StatisticBase::delayCollection(const char* delay_time)
{
    // Make sure only a single collection delay is active
    if ( false == info_->collection_delayed_ ) {

        // Save the Stat Enable setting and then disable the Stat for collection
        info_->saved_stat_enabled_ = info_->stat_enabled_;
        info_->stat_enabled_       = false;
        info_->collection_delayed_ = true;

        Simulation_impl::getSimulation()->registerOneShot(
            delay_time, new OneShot::Handler<StatisticBase>(this, &StatisticBase::delayCollectionExpiredHandler),
            STATISTICCLOCKPRIORITY);
    }
}

void
StatisticBase::delayOutputExpiredHandler()
{
    // Restore the Output Enable to its stored value
    info_->output_enabled_ = info_->saved_output_enabled_;
    info_->output_delayed_ = false;
}

void
StatisticBase::delayCollectionExpiredHandler()
{
    // Restore the Statistic Enable to its stored value
    info_->stat_enabled_       = info_->saved_stat_enabled_;
    info_->collection_delayed_ = false;
}

void
StatisticBase::serialize_order(SST::Core::Serialization::serializer& ser)
{
    /* Only serialize info if stat is non-null */
    if ( !isNullStatistic() ) { ser& info_; }

    /* Store/restore data type */
    if ( ser.mode() != SST::Core::Serialization::serializer::UNPACK ) {
        std::string name(StatisticFieldInfo::getFieldTypeShortName(stat_data_type_));
        ser&        name;
    }
    else {
        std::string name;
        ser&        name;
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

    start_at_time_   = UnitAlgebra("0ns");
    stop_at_time_    = UnitAlgebra("0ns");
    collection_rate_ = UnitAlgebra("0ns");

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
    ser& stat_type_name_;
    ser& stat_full_name_;
    ser& current_collection_count_;
    ser& output_collection_count_;
    ser& collection_count_limit_;
    ser& registered_collection_mode_;
    ser& start_at_time_;
    ser& stop_at_time_;
    ser& collection_rate_;
    ser& stat_enabled_;
    ser& output_enabled_;
    ser& reset_count_on_output_;
    ser& clear_data_on_output_;
    ser& output_at_end_of_sim_;
    ser& output_delayed_;
    ser& saved_stat_enabled_;
    ser& saved_output_enabled_;
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
