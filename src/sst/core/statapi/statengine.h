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

#ifndef _H_SST_CORE_STATISTICS_ENGINE
#define _H_SST_CORE_STATISTICS_ENGINE

#include <sst/core/sst_types.h>
#include <sst/core/statapi/statfieldinfo.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/clock.h>
#include <sst/core/oneshot.h>

namespace SST {
class BaseComponent;
class Simulation;

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
    /** Called by the Components and Subcomponent to perform a statistic Output.
      * @param stat - Pointer to the statistic.
      * @param EndOfSimFlag - Indicates that the output is occuring at the end of simulation.
      */
    void performStatisticOutput(StatisticBase* stat, bool endOfSimFlag = false);
    
    /** Called by the Components and Subcomponent to perform a global statistic Output.
     * This routine will force ALL Components and Subcomponents to output their statistic information.
     * This may lead to unexpected results if the statistic counts or data is reset on output.
     * @param endOfSimFlag - Indicates that the output is occurring at the end of simulation.
     */
    void performGlobalStatisticOutput(bool endOfSimFlag = false);

private:
    friend class SST::BaseComponent;
    friend class SST::Simulation;

    StatisticProcessingEngine();
    ~StatisticProcessingEngine();
   
    bool addPeriodicBasedStatistic(const UnitAlgebra& freq, StatisticBase* Stat);
    bool addEventBasedStatistic(const UnitAlgebra& count, StatisticBase* Stat);
    void setStatisticStartTime(const UnitAlgebra& startTime, StatisticBase* Stat);
    void setStatisticStopTime(const UnitAlgebra& stopTime, StatisticBase* Stat);

    void endOfSimulation();
    void startOfSimulation();
    
    template<typename T>
    void registerStatisticWithEngine(const ComponentId_t& compId, StatisticBase* Stat)
    {
        if (is_type_same<T, int32_t    >::value){return addStatisticToCompStatMap(compId, Stat, StatisticFieldInfo::INT32); }
        if (is_type_same<T, uint32_t   >::value){return addStatisticToCompStatMap(compId, Stat, StatisticFieldInfo::UINT32);}
        if (is_type_same<T, int64_t    >::value){return addStatisticToCompStatMap(compId, Stat, StatisticFieldInfo::INT64); }
        if (is_type_same<T, uint64_t   >::value){return addStatisticToCompStatMap(compId, Stat, StatisticFieldInfo::UINT64);}
        if (is_type_same<T, float      >::value){return addStatisticToCompStatMap(compId, Stat, StatisticFieldInfo::FLOAT); }
        if (is_type_same<T, double     >::value){return addStatisticToCompStatMap(compId, Stat, StatisticFieldInfo::DOUBLE);}

        // If we get here, this is actually an illegal type
        addStatisticToCompStatMap(compId, Stat, StatisticFieldInfo::UNDEFINED);
    }

    template<typename T>
    StatisticBase* isStatisticRegisteredWithEngine(const std::string& compName, const ComponentId_t& compId, std::string& statName, std::string& statSubId)
    {
        if (is_type_same<T, int32_t    >::value){return isStatisticInCompStatMap(compName, compId, statName, statSubId, StatisticFieldInfo::INT32); }
        if (is_type_same<T, uint32_t   >::value){return isStatisticInCompStatMap(compName, compId, statName, statSubId, StatisticFieldInfo::UINT32);}
        if (is_type_same<T, int64_t    >::value){return isStatisticInCompStatMap(compName, compId, statName, statSubId, StatisticFieldInfo::INT64); }
        if (is_type_same<T, uint64_t   >::value){return isStatisticInCompStatMap(compName, compId, statName, statSubId, StatisticFieldInfo::UINT64);}
        if (is_type_same<T, float      >::value){return isStatisticInCompStatMap(compName, compId, statName, statSubId, StatisticFieldInfo::FLOAT); }
        if (is_type_same<T, double     >::value){return isStatisticInCompStatMap(compName, compId, statName, statSubId, StatisticFieldInfo::DOUBLE);}

        // If we get here, this is actually an illegal type
        return isStatisticInCompStatMap(compName, compId, statName, statSubId, StatisticFieldInfo::UNDEFINED);
    }

private:
    void addStatisticClock(const UnitAlgebra& freq, StatisticBase* Stat);
    TimeConverter* registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority);
    TimeConverter* registerStartOneShot(const UnitAlgebra& startTime, OneShot::HandlerBase* handler, int priority);
    TimeConverter* registerStopOneShot(const UnitAlgebra& stopTime, OneShot::HandlerBase* handler, int priority);
    bool handleStatisticEngineClockEvent(Cycle_t CycleNum, SimTime_t timeFactor); 
    void handleStatisticEngineStartTimeEvent(SimTime_t timeFactor); 
    void handleStatisticEngineStopTimeEvent(SimTime_t timeFactor); 
    StatisticBase* isStatisticInCompStatMap(const std::string& compName, const ComponentId_t& compId, std::string& statName, std::string& statSubId, StatisticFieldInfo::fieldType_t fieldType);
    void addStatisticToCompStatMap(const ComponentId_t& compId, StatisticBase* Stat, StatisticFieldInfo::fieldType_t fieldType);
    
private:
    typedef std::vector<StatisticBase*>           StatArray_t;       /*!< Array of Statistics */
    typedef std::map<SimTime_t, StatArray_t*>     StatMap_t;         /*!< Map of simtimes to Statistic Arrays */
    typedef std::map<SimTime_t, Clock*>           ClockMap_t;        /*!< Map of simtimes to clocks */
    typedef std::map<SimTime_t, OneShot*>         OneShotMap_t;      /*!< Map of times to OneShots */
    typedef std::map<ComponentId_t, StatArray_t*> CompStatMap_t;     /*!< Map of ComponentId's to StatInfo Arrays */
                                              
    StatArray_t                               m_EventStatisticArray;  /*!< Array of Event Based Statistics */
    StatMap_t                                 m_PeriodicStatisticMap; /*!< Map of Array's of Periodic Based Statistics */
    StatMap_t                                 m_StartTimeMap;         /*!< Map of Array's of Statistics that are started at a sim time */
    StatMap_t                                 m_StopTimeMap;          /*!< Map of Array's of Statistics that are stopped at a sim time */
    ClockMap_t                                m_ClockMap;             /*!< Map of Clocks */
    OneShotMap_t                              m_StartTimeOneShotMap;  /*!< Map of OneShots for the Statistics start time */
    OneShotMap_t                              m_StopTimeOneShotMap;   /*!< Map of OneShots for the Statistics stop time */
    CompStatMap_t                             m_CompStatMap;          /*!< Map of Arrays of Statistics tied to Component Id's */  
    bool                                      m_SimulationStarted;    /*!< Flag showing if Simulation has started */
    
};

} //namespace Statistics
} //namespace SST

#endif
