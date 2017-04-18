// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_CORE_ACCUMULATOR_STATISTIC_
#define _H_SST_CORE_ACCUMULATOR_STATISTIC_

#include <cmath>

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>

#include <sst/core/statapi/statbase.h>
#include <sst/core/statapi/statoutput.h>

namespace SST {
namespace Statistics {

// NOTE: When calling base class members of classes derived from 
//       a templated base class.  The user must use "this->" in 
//       order to call base class members (to avoid a compilier 
//       error) because they are "nondependant named" and the 
//       templated base class is a "dependant named".  The 
//       compilier will not look in dependant named base classes 
//       when looking up independant names.
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

    AccumulatorStatistic(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams)
		: Statistic<NumberBase>(comp, statName, statSubId, statParams)
    {
        m_sum = 0;
        m_sum_sq = 0;

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
    }

public:
    /**
        Provides the sum of the values presented so far.
        @return The sum of values presented to the class so far.
    */
    NumberBase getSum() 
    {
        return m_sum;
    }

    /**
        Provides the sum of each value squared presented to the class so far.
        @return The sum of squared values presented to the class so far.
    */
    NumberBase getSumSquared() 
    {
        return m_sum_sq;
    }

    /**
        Get the arithmetic mean of the values presented so far
        @return The arithmetic mean of the values presented so far.
    */
    NumberBase getArithmeticMean() 
    {
        uint64_t count = getCount();
        return (count > 0) ? (m_sum / (NumberBase) count) : 0;
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
    NumberBase getStandardDeviation() 
    {
        return (NumberBase) std::sqrt( (double) getVariance() );
    }

    /**
        Get a count of the number of elements presented to the statistics collection so far.
        @return Count the number of values presented to the class.
    */
    uint64_t getCount() 
    {
        return this->getCollectionCount();
    }
    
    void clearStatisticData() override
    {
        m_sum = 0;
        m_sum_sq =0;
        this->setCollectionCount(0);
    }
    
    void registerOutputFields(StatisticOutput* statOutput) override
    {
        Field1 = statOutput->registerField<NumberBase>("Sum");
        Field2 = statOutput->registerField<NumberBase>("SumSQ");
        Field3 = statOutput->registerField<uint64_t>  ("Count");
    }
    
    void outputStatisticData(StatisticOutput* statOutput, bool UNUSED(EndOfSimFlag)) override
    {
        statOutput->outputField(Field1, m_sum);
        statOutput->outputField(Field2, m_sum_sq);  
        statOutput->outputField(Field3, getCount());  
    }
    
    bool isStatModeSupported(StatisticBase::StatMode_t mode) const override
    {
        if (mode == StatisticBase::STAT_MODE_COUNT) {
            return true;
        }
        if (mode == StatisticBase::STAT_MODE_PERIODIC) {
            return true;
        }
        return false;
    }

private:
    NumberBase m_sum;
    NumberBase m_sum_sq;

    StatisticOutput::fieldHandle_t Field1, Field2, Field3;
};

} //namespace Statistics
} //namespace SST

#endif
