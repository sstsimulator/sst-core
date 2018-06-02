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

#ifndef _H_SST_CORE_STATISTICS_ENGINE
#define _H_SST_CORE_STATISTICS_ENGINE

#include <sst/core/sst_types.h>
#include <sst/core/statapi/statfieldinfo.h>
#include <sst/core/statapi/statgroup.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/clock.h>
#include <sst/core/oneshot.h>
#include <sst/core/threadsafe.h>

/* Forward declare for Friendship */
extern int main(int argc, char **argv);
extern void finalize_statEngineConfig(void);


namespace SST {
class BaseComponent;
class Simulation;
class ConfigGraph;
class ConfigStatGroup;
class ConfigStatOutput;
class Params;

namespace Statistics {

template<typename T> class Statistic;
class StatisticBase;
class StatisticOutput;

/**
	\class StatisticProcessingEngine

	An SST core component that handles timing and event processing informing
	all registered Statistics to generate their outputs at desired rates.
*/

class StatisticProcessingEngine
{
    static StatisticProcessingEngine* instance;

public:
    static StatisticProcessingEngine* getInstance() { return instance; }


    /** Called by the Components and Subcomponent to perform a statistic Output.
      * @param stat - Pointer to the statistic.
      * @param EndOfSimFlag - Indicates that the output is occurring at the end of simulation.
      */
    void performStatisticOutput(StatisticBase* stat, bool endOfSimFlag = false);

    /** Called by the Components and Subcomponent to perform a global statistic Output.
     * This routine will force ALL Components and Subcomponents to output their statistic information.
     * This may lead to unexpected results if the statistic counts or data is reset on output.
     * @param endOfSimFlag - Indicates that the output is occurring at the end of simulation.
     */
    void performGlobalStatisticOutput(bool endOfSimFlag = false);


    template<typename T>
    Statistic<T>* createStatistic(BaseComponent *comp, const std::string &type, const std::string &statName, const std::string &statSubId, Params &params)
    {
        StatisticFieldInfo::fieldType_t fieldType = StatisticFieldInfo::getFieldTypeFromTemplate<T>();
        return dynamic_cast<Statistic<T>*>(createStatistic(comp, type, statName, statSubId, params, fieldType));
    }

    template<typename T>
    bool registerStatisticWithEngine(StatisticBase* stat)
    {
        bool ok;
        if ( true == (ok = registerStatisticCore(stat)) ) {
            StatisticFieldInfo::fieldType_t fieldType = StatisticFieldInfo::getFieldTypeFromTemplate<T>();
            addStatisticToCompStatMap(stat, fieldType);
        }
        return ok;
    }

    template<typename T>
    StatisticBase* isStatisticRegisteredWithEngine(const std::string& compName, const ComponentId_t& compId, std::string& statName, std::string& statSubId)
    {
        StatisticFieldInfo::fieldType_t fieldType = StatisticFieldInfo::getFieldTypeFromTemplate<T>();
        return isStatisticInCompStatMap(compName, compId, statName, statSubId, fieldType);
    }


    const std::vector<StatisticOutput*>& getStatOutputs() const { return m_statOutputs; }

private:
    friend class SST::Simulation;
    friend int ::main(int argc, char **argv);
    friend void ::finalize_statEngineConfig(void);

    StatisticProcessingEngine();
    void setup(ConfigGraph *graph);
    ~StatisticProcessingEngine();

    static void init(ConfigGraph *graph);

    StatisticOutput* createStatisticOutput(const ConfigStatOutput &cfg);

    StatisticBase* createStatistic(BaseComponent* comp, const std::string &type,
            const std::string &statName, const std::string &statSubId,
            Params &params, StatisticFieldInfo::fieldType_t fieldType);
    bool registerStatisticCore(StatisticBase* stat);

    StatisticOutput* getOutputForStatistic(const StatisticBase *stat) const;
    StatisticGroup& getGroupForStatistic(const StatisticBase *stat) const;
    bool addPeriodicBasedStatistic(const UnitAlgebra& freq, StatisticBase* Stat);
    bool addEventBasedStatistic(const UnitAlgebra& count, StatisticBase* Stat);
    UnitAlgebra getParamTime(StatisticBase *stat, const std::string& pName) const;
    void setStatisticStartTime(StatisticBase* Stat);
    void setStatisticStopTime(StatisticBase* Stat);

    void finalizeInitialization(); /* Called when performWireUp() finished */
    void startOfSimulation();
    void endOfSimulation();


    void performStatisticOutputImpl(StatisticBase* stat, bool endOfSimFlag);
    void performStatisticGroupOutputImpl(StatisticGroup& group, bool endOfSimFlag);

    bool handleStatisticEngineClockEvent(Cycle_t CycleNum, SimTime_t timeFactor);
    bool handleGroupClockEvent(Cycle_t CycleNum, StatisticGroup* group);
    void handleStatisticEngineStartTimeEvent(SimTime_t timeFactor);
    void handleStatisticEngineStopTimeEvent(SimTime_t timeFactor);
    StatisticBase* isStatisticInCompStatMap(const std::string& compName, const ComponentId_t& compId, std::string& statName, std::string& statSubId, StatisticFieldInfo::fieldType_t fieldType);
    void addStatisticToCompStatMap(StatisticBase* Stat, StatisticFieldInfo::fieldType_t fieldType);

private:
    typedef std::vector<StatisticBase*>           StatArray_t;       /*!< Array of Statistics */
    typedef std::map<SimTime_t, StatArray_t*>     StatMap_t;         /*!< Map of simtimes to Statistic Arrays */
    typedef std::map<ComponentId_t, StatArray_t*> CompStatMap_t;     /*!< Map of ComponentId's to StatInfo Arrays */

    StatArray_t                               m_EventStatisticArray;  /*!< Array of Event Based Statistics */
    StatMap_t                                 m_PeriodicStatisticMap; /*!< Map of Array's of Periodic Based Statistics */
    StatMap_t                                 m_StartTimeMap;         /*!< Map of Array's of Statistics that are started at a sim time */
    StatMap_t                                 m_StopTimeMap;          /*!< Map of Array's of Statistics that are stopped at a sim time */
    CompStatMap_t                             m_CompStatMap;          /*!< Map of Arrays of Statistics tied to Component Id's */  
    bool                                      m_SimulationStarted;    /*!< Flag showing if Simulation has started */

    Output & m_output;
    uint8_t                                   m_statLoadLevel;
    std::vector<StatisticOutput*>             m_statOutputs;
    StatisticGroup                            m_defaultGroup;
    std::vector<StatisticGroup>               m_statGroups;
    Core::ThreadSafe::Barrier                 m_barrier;

};

} //namespace Statistics
} //namespace SST

#endif
