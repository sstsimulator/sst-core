// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_CORE_ACCUMULATOR_STATISTIC_
#define _H_SST_CORE_ACCUMULATOR_STATISTIC_

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <sst/core/statapi/statbase.h>

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
	\class Accumulator

	Allows the online gathering of statistical information about a single quantity. The basic 
	statistics are captured online removing the need to keep a copy of the values of interest.

	@tparam NumberBase A template for the basic numerical type of values
*/

template <typename NumberBase>
class AccumulatorStatistic : public Statistic<NumberBase> 
{
public:
    /**
        Create a new Accumulator class with initial values set to a zero count,
        zero sum statistic of interest.
    */
    AccumulatorStatistic(Component* comp, std::string statName) :
        Statistic<NumberBase>(comp, statName) 
    {
        m_sum = 0;
        m_sum_sq = 0;
        this->setCollectionCount(0);
    }
    
    /**
        Create a new Accumulator class with initial values set to a zero count,
        zero sum statistic of interest.
    */
    AccumulatorStatistic(Component* comp, const char* statName) :
        Statistic<NumberBase>(comp, statName) 
    {
        m_sum = 0;
        m_sum_sq = 0;
        this->setCollectionCount(0);
    }
    
    ~AccumulatorStatistic() {}

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
        Present an array of values to the class to be included in the statistics.
        @param values The array of values to be added to the statistics collection
        @param length The length of the array being presented
    */
    void addData(NumberBase* values, uint32_t length) 
    {
        if(true == this->isEnabled()) {
            for(uint32_t i = 0; i < length; ++i) {
                addData(values[i]);
            }
        }
    }

    /**
        Get the arithmetic mean of the values presented so far
        @return The arithmetic mean of the values presented so far.
    */
    NumberBase getArithmeticMean() 
    {
        uint64_t count = this->getCollectionCount();
        return (count > 0) ? (m_sum / (NumberBase) count) : 0;
    }

    /**
        Get the variance of the values presented so far
        @return The variance of the values presented so far
    */
    NumberBase getVariance() 
    {
        uint64_t count = this->getCollectionCount();
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

    void clearStatisticData()
    {
        m_sum = 0;
        m_sum_sq =0;
        this->setCollectionCount(0);
    }
    
    void registerOutputFields(StatisticOutput* statOutput)
    {
        Field1 = statOutput->registerField<NumberBase>("Sum");
        Field2 = statOutput->registerField<NumberBase>("SumSQ");  
        Field3 = statOutput->registerField<uint64_t>  ("CollectionCount");  
    }
    
    void outputStatisticData(StatisticOutput* statOutput, bool EndOfSimFlag)
    {
        statOutput->outputField(Field1, m_sum);
        statOutput->outputField(Field2, m_sum_sq);  
        statOutput->outputField(Field3, getCount());  
    }
    
protected:    
    /**
        Present a new value to the class to be included in the statistics.
        @param value New value to be presented
    */
    void addData_impl(NumberBase value) 
    {
        m_sum += value;
        m_sum_sq += (value * value);
    }
    
    bool isStatModeSupported(StatisticBase::StatMode_t mode) const 
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
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Statistic<NumberBase>);
        ar & BOOST_SERIALIZATION_NVP(m_sum);
        ar & BOOST_SERIALIZATION_NVP(m_sum_sq); 
    }
};

} //namespace Statistics
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Statistics::AccumulatorStatistic<uint32_t>)

#endif
