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

#ifndef SST_CORE_STATAPI_STATUNIQUECOUNT_H
#define SST_CORE_STATAPI_STATUNIQUECOUNT_H

#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/warnmacros.h"

namespace SST {
class BaseComponent;
namespace Statistics {

/**
    \class UniqueCountStatistic

    Creates a Statistic which counts unique values provided to it.

    @tparam T A template for holding the main data type of this statistic
*/

template <typename T>
class UniqueCountStatistic : public Statistic<T>
{
public:
    SST_ELI_DECLARE_STATISTIC_TEMPLATE(
        UniqueCountStatistic,
        "sst",
        "UniqueCountStatistic",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Track unique occurrences of statistic",
        "SST::Statistic<T>")

    UniqueCountStatistic(
        BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& stat_params) :
        Statistic<T>(comp, stat_name, stat_sub_id, stat_params)
    {}

    ~UniqueCountStatistic() {};

    UniqueCountStatistic() : Statistic<T>() {}

    virtual const std::string& getStatTypeName() const override { return stat_type_; }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::Statistics::Statistic<T>::serialize_order(ser);
        ser& unique_set_;
        // unique_count_field_ will be reset by statistics output object
    }

protected:
    /**
    Present a new value to the Statistic to be included in the unique set
        @param data New data item to be included in the unique set
    */
    void addData_impl(T data) override { unique_set_.insert(data); }

private:
    void clearStatisticData() override { unique_set_.clear(); }

    void registerOutputFields(StatisticFieldsOutput* stat_output) override
    {
        unique_count_field_ = stat_output->registerField<uint64_t>("UniqueItems");
    }

    void outputStatisticFields(StatisticFieldsOutput* stat_output, bool UNUSED(end_of_sim_flag)) override
    {
        stat_output->outputField(unique_count_field_, (uint64_t)unique_set_.size());
    }

private:
    std::set<T>                     unique_set_;
    StatisticOutput::fieldHandle_t  unique_count_field_;
    inline static const std::string stat_type_ = "UniqueCount";
};

} // namespace Statistics
} // namespace SST

#endif // SST_CORE_STATAPI_STATUNIQUECOUNT_H
