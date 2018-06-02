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

#include <sst_config.h>

#include <sst/core/warnmacros.h>
#include <sst/core/output.h>
#include <sst/core/factory.h>
#include <sst/core/timeLord.h>
#include <sst/core/timeConverter.h>
#include <sst/core/simulation.h>
#include <sst/core/statapi/statbase.h>
#include <sst/core/statapi/statengine.h>
#include <sst/core/statapi/statoutput.h>
#include <sst/core/configGraph.h>
#include <sst/core/baseComponent.h>

namespace SST {
namespace Statistics {


void StatisticProcessingEngine::init(ConfigGraph *graph)
{
    StatisticProcessingEngine::instance = new StatisticProcessingEngine();
    instance->setup(graph);
}

StatisticProcessingEngine::StatisticProcessingEngine() :
    m_output(Output::getDefaultObject())
{
}

void StatisticProcessingEngine::setup(ConfigGraph *graph)
{
    m_SimulationStarted = false;
    m_statLoadLevel = graph->getStatLoadLevel();
    for ( auto & cfg : graph->getStatOutputs() ) {
        m_statOutputs.push_back(createStatisticOutput(cfg));
    }

    m_defaultGroup.output = m_statOutputs[0];
    for ( auto & cfg : graph->getStatGroups() ) {
        m_statGroups.emplace_back(cfg.second);

        /* Force component / statistic registration for Group stats*/
        for ( ComponentId_t compID : cfg.second.components ) {
            ConfigComponent *ccomp = graph->findComponent(compID);
            if ( ccomp ) { /* Should always be true */
                for ( auto &kv : cfg.second.statMap ) {
                    ccomp->enableStatistic(kv.first);
                    ccomp->setStatisticParameters(kv.first, kv.second);
                }
            }
        }
    }

}

StatisticProcessingEngine::~StatisticProcessingEngine()
{
    StatArray_t*     statArray;
    StatisticBase*   stat;

    // Destroy all the Statistics that have been created
    for (CompStatMap_t::iterator it_m = m_CompStatMap.begin(); it_m != m_CompStatMap.end(); it_m++) {
        // Get the Array for this Map Item    
        statArray = it_m->second;

        // Walk the stat Array and delete each stat
        for (StatArray_t::iterator it_v = statArray->begin(); it_v != statArray->end(); it_v++) {
            stat = *it_v;
            delete stat;
        }
    }
}



StatisticBase* StatisticProcessingEngine::createStatistic(BaseComponent* comp, const std::string &type,
            const std::string &statName, const std::string &statSubId,
            Params &params, StatisticFieldInfo::fieldType_t fieldType)
{
    return Factory::getFactory()->CreateStatistic(comp, type, statName, statSubId, params, fieldType);
}

bool StatisticProcessingEngine::registerStatisticCore(StatisticBase* stat)
{
    if ( stat->isNullStatistic() )
        return true;

    if ( 0 == m_statLoadLevel ) {
        m_output.verbose(CALL_INFO, 1, 0,
                " Warning: Statistic Load Level = 0 (all statistics disabled); statistic %s is disabled...\n",
                stat->getFullStatName().c_str());
        return false;
    }


    uint8_t enableLevel = stat->getComponent()->getComponentInfoStatisticEnableLevel(stat->getStatName());
    if ( enableLevel > m_statLoadLevel ) {
        m_output.verbose(CALL_INFO, 1, 0,
                " Warning: Load Level %d is too low to enable Statistic %s with Enable Level %d, statistic will not be enabled...\n",
                m_statLoadLevel, stat->getFullStatName().c_str(), enableLevel);
        return false;
    }

    StatisticGroup &group = getGroupForStatistic(stat);

    if ( group.isDefault ) {
        // If the mode is Periodic Based, the add the statistic to the
        // StatisticProcessingEngine otherwise add it as an Event Based Stat.
        UnitAlgebra collectionRate = stat->m_statParams.find<SST::UnitAlgebra>("rate", "0ns");
        if (StatisticBase::STAT_MODE_PERIODIC == stat->getRegisteredCollectionMode()) {
            if (false == addPeriodicBasedStatistic(collectionRate, stat)) {
                return false;
            }
        } else {
            if (false == addEventBasedStatistic(collectionRate, stat)) {
                return false;
            }
        }
    } else {
        /* Make sure it is periodic! */
        if ( StatisticBase::STAT_MODE_PERIODIC != stat->getRegisteredCollectionMode() ) {
            m_output.output("ERROR: Statistics in groups must be periodic in nature!\n");
            return false;
        }
    }

    /* All checks pass.  Add the stat */

    group.addStatistic(stat);

    if ( group.isDefault ) {
        getOutputForStatistic(stat)->registerStatistic(stat);
    }

    setStatisticStartTime(stat);
    setStatisticStopTime(stat);

    return true;
}



void StatisticProcessingEngine::finalizeInitialization()
{
    bool master = ( Simulation::getSimulation()->getRank().thread == 0 );
    if ( master ) {
        m_barrier.resize(Simulation::getSimulation()->getNumRanks().thread);
    }
    for ( auto & g : m_statGroups ) {
        if ( master ) {
            g.output->registerGroup(&g);
        }

        /* Register group clock, if rate is set */
        if ( g.outputFreq.getValue() != 0 ) {
            Simulation::getSimulation()->registerClock(
                    g.outputFreq,
                    new Clock::Handler<StatisticProcessingEngine, StatisticGroup*>(this,
                        &StatisticProcessingEngine::handleGroupClockEvent, &g),
                    STATISTICCLOCKPRIORITY);
        }
    }

}


void StatisticProcessingEngine::startOfSimulation()
{
    m_SimulationStarted = true;

    for ( auto &so : m_statOutputs ) {
        so->startOfSimulation();
    }
}


void StatisticProcessingEngine::endOfSimulation()
{

    // Output the Event based Statistics
    for ( StatisticBase * stat : m_EventStatisticArray ) {
        // Check to see if the Statistic is to output at end of sim
        if (true == stat->getFlagOutputAtEndOfSim()) {
            // Perform the output
           performStatisticOutputImpl(stat, true);
        }
    }

    // Output the Periodic Based Statistics
    for ( auto & it_m : m_PeriodicStatisticMap ) {
        // Get the array from the Map Iterator
        StatArray_t* statArray = it_m.second;

        for ( StatisticBase* stat : *statArray ) {
            // Check to see if the Statistic is to output at end of sim
            if (true == stat->getFlagOutputAtEndOfSim()) {
                // Perform the output
               performStatisticOutputImpl(stat, true);
            }
        }
    }

    for ( auto & sg : m_statGroups ) {
        performStatisticGroupOutputImpl(sg, true);
    }


    for ( auto &so : m_statOutputs ) {
        so->endOfSimulation();
    }
}



StatisticOutput* StatisticProcessingEngine::createStatisticOutput(const ConfigStatOutput &cfg)
{
    StatisticOutput *so = Factory::getFactory()->CreateStatisticOutput(cfg.type, cfg.params);
    if (NULL == so) {
        m_output.fatal(CALL_INFO, -1, " - Unable to instantiate Statistic Output %s\n", cfg.type.c_str());
    }

    if (false == so->checkOutputParameters()) {
        // If checkOutputParameters() fail, Tell the user how to use them and abort simulation
        m_output.output("Statistic Output (%s) :\n", so->getStatisticOutputName().c_str());
        so->printUsage();
        m_output.output("\n");

        m_output.output("Statistic Output Parameters Provided:\n");
        cfg.params.print_all_params(Output::getDefaultObject(), "  ");
        m_output.fatal(CALL_INFO, -1, " - Required Statistic Output Parameters not set\n");
    }
    return so;
}



StatisticOutput* StatisticProcessingEngine::getOutputForStatistic(const StatisticBase *stat) const
{
    return getGroupForStatistic(stat).output;
}


/* Return the group that would claim this stat */
StatisticGroup& StatisticProcessingEngine::getGroupForStatistic(const StatisticBase *stat) const
{
    for ( auto & g : m_statGroups ) {
        if ( g.claimsStatistic(stat) ) {
            return const_cast<StatisticGroup&>(g);
        }
    }
    return const_cast<StatisticGroup&>(m_defaultGroup);
}



bool StatisticProcessingEngine::addPeriodicBasedStatistic(const UnitAlgebra& freq, StatisticBase* stat)
{
    Simulation*         sim = Simulation::getSimulation();
    TimeConverter*      tcFreq = sim->getTimeLord()->getTimeConverter(freq);
    SimTime_t           tcFactor = tcFreq->getFactor();
    Clock::HandlerBase* ClockHandler;
    StatArray_t*        statArray;

    // See if the map contains an entry for this factor
    if (m_PeriodicStatisticMap.find(tcFactor) == m_PeriodicStatisticMap.end() ) {

        // Check to see if the freq is zero.  Only add a new clock if the freq is non zero
        if (0 != freq.getValue()) {

            // This tcFactor is not found in the map, so create a new clock handler.
            ClockHandler = new Clock::Handler<StatisticProcessingEngine, SimTime_t>(this,
                          &StatisticProcessingEngine::handleStatisticEngineClockEvent, tcFactor);

            // Set the clock priority so that normal clocks events will occur before
            // this clock event.
            sim->registerClock(freq, ClockHandler, STATISTICCLOCKPRIORITY);
        }

        // Also create a new Array of Statistics and relate it to the map
        statArray = new std::vector<StatisticBase*>();
        m_PeriodicStatisticMap[tcFactor] = statArray;
    }

    // The Statistic Map has the time factor registered.
    statArray = m_PeriodicStatisticMap[tcFactor];

    // Add the statistic to the lists of statistics to be called when the clock fires.
    statArray->push_back(stat);

    return true;
}

bool StatisticProcessingEngine::addEventBasedStatistic(const UnitAlgebra& count, StatisticBase* stat)
{
    if (0 != count.getValue()) {         
        // Set the Count Limit
        stat->setCollectionCountLimit(count.getRoundedValue());
    } else {
        stat->setCollectionCountLimit(0); 
    }
    stat->setFlagResetCountOnOutput(true);

    // Add the statistic to the Array of Event Based Statistics    
    m_EventStatisticArray.push_back(stat);
    return true;
}


UnitAlgebra StatisticProcessingEngine::getParamTime(StatisticBase *stat, const std::string& pName) const
{
    std::string pVal = stat->m_statParams.find<std::string>(pName);
    if ( pVal.empty() ) {
        pVal = "0ns";
    }

    UnitAlgebra uaVal = UnitAlgebra(pVal);

    if ( !uaVal.hasUnits("s") ) {
        m_output.fatal(CALL_INFO, 1,
                "ERROR: Statistic %s - param %s = %s; must be in units of seconds; exiting...\n",
                stat->getFullStatName().c_str(), pName.c_str(), pVal.c_str());
    }

    return uaVal;
}

void StatisticProcessingEngine::setStatisticStartTime(StatisticBase* stat)
{
    UnitAlgebra startTime = getParamTime(stat, "startat");
    Simulation*           sim = Simulation::getSimulation();
    TimeConverter*        tcStartTime = sim->getTimeLord()->getTimeConverter(startTime);
    SimTime_t             tcFactor = tcStartTime->getFactor();
    StatArray_t*          statArray;

    // Check to see if the time is zero, if it is we skip this work
    if (0 != startTime.getValue()) {
        // See if the map contains an entry for this factor
        if (m_StartTimeMap.find(tcFactor) == m_StartTimeMap.end()) {
            // This tcFactor is not found in the map, so create a new OneShot handler.
            OneShot::HandlerBase* OneShotHandler = new OneShot::Handler<StatisticProcessingEngine, SimTime_t>(this,
                    &StatisticProcessingEngine::handleStatisticEngineStartTimeEvent, tcFactor);

            // Set the OneShot priority so that normal events will occur before this event.
            sim->registerOneShot(startTime, OneShotHandler, STATISTICCLOCKPRIORITY);

            // Also create a new Array of Statistics and relate it to the map
            statArray = new std::vector<StatisticBase*>();
            m_StartTimeMap[tcFactor] = statArray;
        }

        // The Statistic Map has the time factor registered.
        statArray = m_StartTimeMap[tcFactor];

        // Add the statistic to the lists of statistics to be called when the OneShot fires.
        statArray->push_back(stat);

        // Disable the Statistic until the start time event occurs
        stat->disable();
    }
}

void StatisticProcessingEngine::setStatisticStopTime(StatisticBase* stat)
{
    UnitAlgebra stopTime = getParamTime(stat, "startat");
    Simulation*           sim = Simulation::getSimulation();
    TimeConverter*        tcStopTime = sim->getTimeLord()->getTimeConverter(stopTime);
    SimTime_t             tcFactor = tcStopTime->getFactor();
    StatArray_t*          statArray;

    // Check to see if the time is zero, if it is we skip this work
    if (0 != stopTime.getValue()) {
        // See if the map contains an entry for this factor
        if (m_StopTimeMap.find(tcFactor) == m_StopTimeMap.end()) {
            // This tcFactor is not found in the map, so create a new OneShot handler.
            OneShot::HandlerBase* OneShotHandler = new OneShot::Handler<StatisticProcessingEngine, SimTime_t>(this,
                    &StatisticProcessingEngine::handleStatisticEngineStopTimeEvent, tcFactor);

            // Set the OneShot priority so that normal events will occur before this event.
            sim->registerOneShot(stopTime, OneShotHandler, STATISTICCLOCKPRIORITY);

            // Also create a new Array of Statistics and relate it to the map
            statArray = new std::vector<StatisticBase*>();
            m_StopTimeMap[tcFactor] = statArray;
        }

        // The Statistic Map has the time factor registered.
        statArray = m_StopTimeMap[tcFactor];

        // Add the statistic to the lists of statistics to be called when the OneShot fires.
        statArray->push_back(stat);
    }
}

void StatisticProcessingEngine::performStatisticOutput(StatisticBase* stat, bool endOfSimFlag /*=false*/)
{
    if ( stat->getGroup()->isDefault )
        performStatisticOutputImpl(stat, endOfSimFlag);
    else
        performStatisticGroupOutputImpl(*const_cast<StatisticGroup*>(stat->getGroup()), endOfSimFlag);
}

void StatisticProcessingEngine::performStatisticOutputImpl(StatisticBase* stat, bool endOfSimFlag /*=false*/)
{

    StatisticOutput* statOutput = getOutputForStatistic(stat);

    // Has the simulation started?
    if (true == m_SimulationStarted) {
        // Is the Statistic Output Enabled?
        if (false == stat->isOutputEnabled()) {
            return;
        }

        statOutput->outputEntries(stat, endOfSimFlag);

        if (false == endOfSimFlag) {
            // Check to see if the Statistic Count needs to be reset
            if (true == stat->getFlagResetCountOnOutput()) {
                stat->resetCollectionCount();
            }

            // Check to see if the Statistic Data needs to be cleared
            if (true == stat->getFlagClearDataOnOutput()) {
                stat->clearStatisticData();
            }
        }
    }
}


void StatisticProcessingEngine::performStatisticGroupOutputImpl(StatisticGroup &group, bool endOfSimFlag /*=false*/)
{

    StatisticOutput* statOutput = group.output;

    // Has the simulation started?
    if (true == m_SimulationStarted) {

        statOutput->outputGroup(&group, endOfSimFlag);

        if (false == endOfSimFlag) {
            for ( auto & stat : group.stats ) {
                // Check to see if the Statistic Count needs to be reset
                if (true == stat->getFlagResetCountOnOutput()) {
                    stat->resetCollectionCount();
                }

                // Check to see if the Statistic Data needs to be cleared
                if (true == stat->getFlagClearDataOnOutput()) {
                    stat->clearStatisticData();
                }
            }
        }
    }
}


void StatisticProcessingEngine::performGlobalStatisticOutput(bool endOfSimFlag /*=false*/) 
{
    StatArray_t*    statArray;
    StatisticBase*  stat;

    // Output Event based statistics
    for (StatArray_t::iterator it_v = m_EventStatisticArray.begin(); it_v != m_EventStatisticArray.end(); it_v++) {
        stat = * it_v;
        performStatisticOutputImpl(stat, endOfSimFlag);
    }

    // Output Periodic based statistics 
    for (StatMap_t::iterator it_m = m_PeriodicStatisticMap.begin(); it_m != m_PeriodicStatisticMap.end(); it_m++) {
        statArray = it_m->second;

        for (StatArray_t::iterator it_v = statArray->begin(); it_v != statArray->end(); it_v++) {
            stat = *it_v;
            performStatisticOutputImpl(stat, endOfSimFlag);
        }
    }

    for ( auto & sg : m_statGroups ) {
        performStatisticGroupOutputImpl(sg, endOfSimFlag);
    }
}




bool StatisticProcessingEngine::handleStatisticEngineClockEvent(Cycle_t UNUSED(CycleNum), SimTime_t timeFactor) 
{
    StatArray_t*     statArray;
    StatisticBase*   stat;
    unsigned int     x;

    // Get the array for the timeFactor
    statArray = m_PeriodicStatisticMap[timeFactor];
    
    // Walk the array, and call the output method of each statistic
    for (x = 0; x < statArray->size(); x++) {
        stat = statArray->at(x);

        // Perform the output
        performStatisticOutputImpl(stat, false);
    }
    // Return false to keep the clock going
    return false;
}



bool StatisticProcessingEngine::handleGroupClockEvent(Cycle_t UNUSED(CycleNum), StatisticGroup *group)
{
    m_barrier.wait();
    if ( Simulation::getSimulation()->getRank().thread == 0 ) {
        performStatisticGroupOutputImpl(*group, false);
    }
    m_barrier.wait();
    return false;
}

void StatisticProcessingEngine::handleStatisticEngineStartTimeEvent(SimTime_t timeFactor)
{
    StatArray_t*     statArray;
    StatisticBase*   stat;
    unsigned int     x;

    // Get the array for the timeFactor
    statArray = m_StartTimeMap[timeFactor];
    
    // Walk the array, and call the output method of each statistic
    for (x = 0; x < statArray->size(); x++) {
        stat = statArray->at(x);

        // Enable the Statistic 
        stat->enable();
    }
}

void StatisticProcessingEngine::handleStatisticEngineStopTimeEvent(SimTime_t timeFactor) 
{
    StatArray_t*     statArray;
    StatisticBase*   stat;
    unsigned int     x;

    // Get the array for the timeFactor
    statArray = m_StopTimeMap[timeFactor];
    
    // Walk the array, and call the output method of each statistic
    for (x = 0; x < statArray->size(); x++) {
        stat = statArray->at(x);

        // Disable the Statistic 
        stat->disable();
    }
}

StatisticBase* StatisticProcessingEngine::isStatisticInCompStatMap(const std::string& compName, const ComponentId_t& compId, std::string& statName, std::string& statSubId, StatisticFieldInfo::fieldType_t fieldType)
{
    StatArray_t*        statArray;
    StatisticBase*      TestStat;
    
    // See if the map contains an entry for this Component ID
    if (m_CompStatMap.find(compId) == m_CompStatMap.end() ) {
        // Nope, this component ID has not been registered
        return NULL;
    }
    
    // The CompStatMap has Component ID registered, get the array associated with it    
    statArray = m_CompStatMap[compId];
    
    // Look for the Statistic in this array 
    for (StatArray_t::iterator it_v = statArray->begin(); it_v != statArray->end(); it_v++) {
        TestStat = *it_v;

        if ((TestStat->getCompName() == compName) && 
            (TestStat->getStatName() == statName) &&
            (TestStat->getStatSubId() == statSubId) &&
            (TestStat->getStatDataType() == fieldType)) {
            return TestStat;
        }
    }
    
    // We did not find the stat in this component
    return NULL;
}

void StatisticProcessingEngine::addStatisticToCompStatMap(StatisticBase* Stat, StatisticFieldInfo::fieldType_t UNUSED(fieldType))
{
    StatArray_t*        statArray;
    ComponentId_t compId = Stat->getComponent()->getId();

    // See if the map contains an entry for this Component ID
    if (m_CompStatMap.find(compId) == m_CompStatMap.end() ) {
        // Nope, Create a new Array of Statistics and relate it to the map
        statArray = new std::vector<StatisticBase*>(); 
        m_CompStatMap[compId] = statArray; 
    }

    // The CompStatMap has Component ID registered, get the array associated with it    
    statArray = m_CompStatMap[compId];

    // Add the statistic to the lists of statistics registered to this component   
    statArray->push_back(Stat);
}




StatisticProcessingEngine* StatisticProcessingEngine::instance = NULL;

} //namespace Statistics
} //namespace SST

