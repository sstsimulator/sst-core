// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_CORE_NULL_STATISTIC_
#define _H_SST_CORE_NULL_STATISTIC_

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>

#include <sst/core/statapi/statbase.h>

namespace SST {
namespace Statistics {

// NOTE: When calling base class members of classes derived from 
//       a templated base class.  The user must use "this->" in 
//       order to call base class members (to avoid a compiler 
//       error) because they are "nondependant named" and the 
//       templated base class is a "dependant named".  The 
//       compiler will not look in dependant named base classes 
//       when looking up indepenedent names.
// See: http://www.parashift.com/c++-faq-lite/nondependent-name-lookup-members.html

/**
	\class NullStatistic

	An empty statistic place holder.

	@tparam T A template for holding the main data type of this statistic
*/

template <typename T>
class NullStatistic : public Statistic<T>
{
public:
    NullStatistic(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams)
		: Statistic<T>(comp, statName, statSubId, statParams)
    {
        // Set the Name of this Statistic
        this->setStatisticTypeName("NULL");
    }

    ~NullStatistic(){};

    void clearStatisticData() override
    {
        // Do Nothing
    }

    void registerOutputFields(StatisticOutput* UNUSED(statOutput)) override
    {
        // Do Nothing
    }

    void outputStatisticData(StatisticOutput* UNUSED(statOutput), bool UNUSED(EndOfSimFlag)) override
    {
        // Do Nothing
    }

    bool isReady() const override
    {
        return true;
    }

    bool isNullStatistic() const override
    {
        return true;
    }

protected:
    void addData_impl(T UNUSED(data)) override
    {
        // Do Nothing
    }


private:
};

} //namespace Statistics
} //namespace SST

#endif
