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


#ifndef _H_SST_CORE_NULL_STATISTIC_
#define _H_SST_CORE_NULL_STATISTIC_

#include <sst/core/sst_types.h>

#include <sst/core/component.h>
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
	\class NullStatistic

	An empty statistic place holder.

	@tparam T A template for holding the main data type of this statistic
*/

template <typename T>
class NullStatistic : public Statistic<T>
{
private:    
    friend class SST::Component;
    
    NullStatistic(Component* comp, std::string& statName, std::string& statSubId, Params& statParams) 
		: Statistic<T>(comp, statName, statSubId, statParams)
    {
        // Set the Name of this Statistic
        this->setStatisticTypeName("NULL");
    }

    ~NullStatistic(){};

protected:    
    void addData_impl(T data)
    {
        // Do Nothing
    }

private:    
    void clearStatisticData()
    {
        // Do Nothing
    }
    
    void registerOutputFields(StatisticOutput* statOutput)
    {
        // Do Nothing
    }
    
    void outputStatisticData(StatisticOutput* statOutput, bool EndOfSimFlag)
    {
        // Do Nothing
    }
    
    bool isReady() const
    {
        return true;
    }

    bool isNullStatistic() const
    {
        return true;
    }

private:
};

} //namespace Statistics
} //namespace SST

#endif
