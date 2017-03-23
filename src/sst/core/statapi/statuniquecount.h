// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_CORE_UNIQUE_COUNT_STATISTIC_
#define _H_SST_CORE_UNIQUE_COUNT_STATISTIC_

#include <sst/core/sst_types.h>

#include <sst/core/statapi/statbase.h>

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
private:
    friend class SST::BaseComponent;

    UniqueCountStatistic(BaseComponent* comp, std::string& statName, std::string& statSubId, Params& statParams)
		: Statistic<T>(comp, statName, statSubId, statParams)
    {
        // Set the Name of this Statistic
        this->setStatisticTypeName("UniqueCount");
    }

    ~UniqueCountStatistic(){};

protected:
    /**
	Present a new value to the Statistic to be included in the unique set
        @param data New data item to be included in the unique set
    */
    void addData_impl(T data) {
	uniqueSet.insert(data);
    }

private:
    void clearStatisticData()
    {
	uniqueSet.clear();
    }

    void registerOutputFields(StatisticOutput* statOutput)
    {
	uniqueCountField = statOutput->registerField<uint64_t>("UniqueItems");
    }

    void outputStatisticData(StatisticOutput* statOutput, bool EndOfSimFlag __attribute__((unused)))
    {
	statOutput->outputField(uniqueCountField, (uint64_t) uniqueSet.size());
    }

private:
    std::set<T> uniqueSet;
    StatisticOutput::fieldHandle_t uniqueCountField;

};

} //namespace Statistics
} //namespace SST

#endif
