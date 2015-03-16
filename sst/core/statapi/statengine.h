// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_CORE_STATISTICS_ENGINE
#define _H_SST_CORE_STATISTICS_ENGINE

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

namespace SST {
namespace Statistics {
    
class StatisticBase;

/**
	\class StatisticProcessingEngine

	An SST core component that handles timing and event processing informing
	all registered Statistics to generate their outputs at desired rates.
*/

class StatisticProcessingEngine
{
public:
    StatisticProcessingEngine();
    ~StatisticProcessingEngine();

    bool addPeriodicBasedStatistic(const UnitAlgebra& freq, StatisticBase* Stat);
    bool addEventBasedStatistic(const UnitAlgebra& count, StatisticBase* Stat);
    void performStatisticOutput(StatisticBase* stat, bool endOfSimFlag = false);
    void endOfSimulation();
    
private:
    void addStatisticClock(const UnitAlgebra& freq, StatisticBase* Stat);
    TimeConverter* registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority);
    bool handleStatisticEngineClockEvent(Cycle_t CycleNum, SimTime_t timeFactor); 
    bool isStatisticPreviouslyRegistered(StatisticBase* Stat);
    
private:
    typedef std::vector<StatisticBase*>       StatArray_t;
    typedef std::map<SimTime_t, StatArray_t*> StatMap_t;
    typedef std::map<SimTime_t, Clock*>       clockMap_t;     /*!< Map of times to clocks */
                                              
    StatArray_t                               m_EventStatisticArray;  /*!< Array of Event Based Statistics */
    StatMap_t                                 m_PeriodicStatisticMap; /*!< Map of Array's of Periodic Based Statistics */
    clockMap_t                                m_ClockMap;

    // Serialization
    friend class boost::serialization::access;
    template<class Archive> 
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(m_EventStatisticArray);
        ar & BOOST_SERIALIZATION_NVP(m_PeriodicStatisticMap);
        ar & BOOST_SERIALIZATION_NVP(m_ClockMap);
    }
};

} //namespace Statistics
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Statistics::StatisticProcessingEngine)

#endif
