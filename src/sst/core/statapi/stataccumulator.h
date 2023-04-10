// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_STATAPI_STATACCUMULATOR_H
#define SST_CORE_STATAPI_STATACCUMULATOR_H

#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/warnmacros.h"

#include <cmath>
#include <limits>

namespace SST {
namespace Statistics {

// NOTE: When calling base class members of classes derived from
//       a templated base class.  The user must use "this->" in
//       order to call base class members (to avoid a compiler
//       error) because they are "nondependant named" and the
//       templated base class is a "dependant named".  The
//       compiler will not look in dependant named base classes
//       when looking up independent names.
// See: http://www.parashift.com/c++-faq-lite/nondependent-name-lookup-members.html

/**
    \class AccumulatorStatistic

    Allows the online gathering of statistical information about a single quantity. The basic
    statistics are captured online removing the need to keep a copy of the values of interest.

    @tparam NumberBase A template for the basic numerical type of values
*/

template <typename NumberBase>
class AccumulatorStatistic : public Statistic<NumberBase>
{
public:
    SST_ELI_DECLARE_STATISTIC_TEMPLATE(
      AccumulatorStatistic,
      "sst",
      "AccumulatorStatistic",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Accumulate all contributions to a statistic",
      "SST::Statistic<T>")

    AccumulatorStatistic(
        BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        Statistic<NumberBase>(comp, statName, statSubId, statParams)
    {
        m_sum    = static_cast<NumberBase>(0);
        m_sum_sq = static_cast<NumberBase>(0);
        m_min    = std::numeric_limits<NumberBase>::max();
        m_max    = std::numeric_limits<NumberBase>::min();

        // Set the Name of this Statistic
        this->setStatisticTypeName("Accumulator");
    }

    ~AccumulatorStatistic() {}

protected:
    /**
        Present a new value to the class to be included in the statistics.
        @param value New value to be presented
    */
    void addData_impl(NumberBase value) override
    {
        m_sum += value;
        m_sum_sq += (value * value);
        m_min = (value < m_min) ? value : m_min;
        m_max = (value > m_max) ? value : m_max;
    }

    void addData_impl_Ntimes(uint64_t N, NumberBase value) override
    {
        m_sum += N * value;
        m_sum_sq += N * value * value;
        m_min = (value < m_min) ? value : m_min;
        m_max = (value > m_max) ? value : m_max;
    }

public:
    /**
        Provides the sum of the values presented so far.
        @return The sum of values presented to the class so far.
    */
    NumberBase getSum() { return m_sum; }

    /**
    Provides the maxmimum value presented so far.
    @return The maximum of values presented to the class so far
    */
    NumberBase getMax() { return m_max; }

    /**
    Provides the minimum value presented so far.
    @return The minimum of values presented to the class so far
    */
    NumberBase getMin() { return m_min; }

    /**
        Provides the sum of each value squared presented to the class so far.
        @return The sum of squared values presented to the class so far.
    */
    NumberBase getSumSquared() { return m_sum_sq; }

    /**
        Get the arithmetic mean of the values presented so far
        @return The arithmetic mean of the values presented so far.
    */
    NumberBase getArithmeticMean()
    {
        uint64_t count = getCount();
        return (count > 0) ? (m_sum / (NumberBase)count) : 0;
    }

    /**
        Get the variance of the values presented so far
        @return The variance of the values presented so far
    */
    NumberBase getVariance()
    {
        uint64_t count = getCount();
        return (count > 0) ? (m_sum_sq * count) - (m_sum * m_sum) : 0;
    }

    /**
        Get the standard deviation of the values presented so far
        @return The standard deviation of the values presented so far
    */
    NumberBase getStandardDeviation() { return (NumberBase)std::sqrt((double)getVariance()); }

    /**
        Get a count of the number of elements presented to the statistics collection so far.
        @return Count the number of values presented to the class.
    */
    uint64_t getCount() { return this->getCollectionCount(); }

    void clearStatisticData() override
    {
        m_sum    = 0;
        m_sum_sq = 0;
        m_min    = std::numeric_limits<NumberBase>::max();
        m_max    = std::numeric_limits<NumberBase>::min();
        this->setCollectionCount(0);
    }

    void registerOutputFields(StatisticFieldsOutput* statOutput) override
    {
        h_sum   = statOutput->registerField<NumberBase>("Sum");
        h_sumsq = statOutput->registerField<NumberBase>("SumSQ");
        h_count = statOutput->registerField<uint64_t>("Count");
        h_min   = statOutput->registerField<NumberBase>("Min");
        h_max   = statOutput->registerField<NumberBase>("Max");
    }

    void outputStatisticFields(StatisticFieldsOutput* statOutput, bool UNUSED(EndOfSimFlag)) override
    {
        statOutput->outputField(h_sum, m_sum);
        statOutput->outputField(h_sumsq, m_sum_sq);
        statOutput->outputField(h_count, getCount());

        if ( 0 == getCount() ) {
            statOutput->outputField(h_min, 0);
            statOutput->outputField(h_max, 0);
        }
        else {
            statOutput->outputField(h_min, m_min);
            statOutput->outputField(h_max, m_max);
        }
    }

    bool isStatModeSupported(StatisticBase::StatMode_t mode) const override
    {
        switch ( mode ) {
        case StatisticBase::STAT_MODE_COUNT:
        case StatisticBase::STAT_MODE_PERIODIC:
        case StatisticBase::STAT_MODE_DUMP_AT_END:
            return true;
        default:
            return false;
        }
        return false;
    }

private:
    NumberBase m_sum;
    NumberBase m_sum_sq;
    NumberBase m_min;
    NumberBase m_max;

    StatisticOutput::fieldHandle_t h_sum;
    StatisticOutput::fieldHandle_t h_sumsq;
    StatisticOutput::fieldHandle_t h_count;
    StatisticOutput::fieldHandle_t h_max;
    StatisticOutput::fieldHandle_t h_min;
};

} // namespace Statistics
} // namespace SST

#endif // SST_CORE_STATAPI_STATACCUMULATOR_H
