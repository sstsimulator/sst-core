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

#include <sst_config.h>
#include <sst/core/serialization.h>

#include <sst/core/timeLord.h>
#include <sst/core/timeConverter.h>
#include <sst/core/simulation.h>
#include <sst/core/statapi/statbase.h>
#include <sst/core/statapi/statengine.h>

namespace SST {
namespace Statistics {
    
StatisticProcessingEngine::StatisticProcessingEngine()
{
    m_SimulationStarted = false;
}

StatisticProcessingEngine::~StatisticProcessingEngine()
{
    StatArray_t*     statArray;
    StatisticBase*   stat;

    // Clocks already got deleted by timeVortex, simply clear the clockMap
    m_ClockMap.clear();
    
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

bool StatisticProcessingEngine::addPeriodicBasedStatistic(const UnitAlgebra& freq, StatisticBase* stat)
{
    // Add the statistic to the Map of Periodic Based Statistics   
    addStatisticClock(freq, stat);
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

void StatisticProcessingEngine::performStatisticOutput(StatisticBase* stat, bool endOfSimFlag /*=false*/)
{
    StatisticOutput* statOutput = Simulation::getSimulation()->getStatisticsOutput();

    // Has the simulation started?
    if (true == m_SimulationStarted) {
        // Is the Statistic Output Enabled?
        if (false == stat->isOutputEnabled()) {
            return;
        }
        statOutput->startOutputEntries(stat);
        stat->outputStatisticData(statOutput, endOfSimFlag);
        statOutput->stopOutputEntries();
        
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

void StatisticProcessingEngine::endOfSimulation()
{
    StatArray_t*     statArray;
    StatisticBase*   stat;

    // Output the Event based Statistics
    for (StatArray_t::iterator it_v = m_EventStatisticArray.begin(); it_v != m_EventStatisticArray.end(); it_v++) {
        stat = *it_v;

        // Check to see if the Statistic is to output at end of sim
        if (true == stat->getFlagOutputAtEndOfSim()) {
        
            // Perform the output
           performStatisticOutput(stat, true);
        }
    }
    
    // Output the Periodic Based Statistics
    for (StatMap_t::iterator it_m = m_PeriodicStatisticMap.begin(); it_m != m_PeriodicStatisticMap.end(); it_m++) {
        // Get the array from the Map Iterator

        statArray = it_m->second;

        for (StatArray_t::iterator it_v = statArray->begin(); it_v != statArray->end(); it_v++) {
            stat = *it_v;
            // Check to see if the Statistic is to output at end of sim
            if (true == stat->getFlagOutputAtEndOfSim()) {
            
                // Perform the output
               performStatisticOutput(stat, true);
            }
        }
    }
}

void StatisticProcessingEngine::startOfSimulation()
{
    m_SimulationStarted = true;
}

TimeConverter* StatisticProcessingEngine::registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority)
{
    Simulation* sim = Simulation::getSimulation();
    TimeConverter* tcFreq = sim->getTimeLord()->getTimeConverter(freq);
    
    if ( m_ClockMap.find( tcFreq->getFactor() ) == m_ClockMap.end() ) {
        Clock* ce = new Clock( tcFreq, priority );
        m_ClockMap[ tcFreq->getFactor() ] = ce; 

        ce->schedule();
    }
    m_ClockMap[ tcFreq->getFactor() ]->registerHandler( handler );
    return tcFreq;
}

void StatisticProcessingEngine::addStatisticClock(const UnitAlgebra& freq, StatisticBase* stat)
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
            registerClock(freq, ClockHandler, STATISTICCLOCKPRIORITY);
        }

        // Also create a new Array of Statistics and relate it to the map
        statArray = new std::vector<StatisticBase*>(); 
        m_PeriodicStatisticMap[tcFactor] = statArray; 
    }
        
    // The Statistic Map has the time factor registered.    
    statArray = m_PeriodicStatisticMap[tcFactor];

    // Add the statistic to the lists of statistics to be called when the clock fires.    
    statArray->push_back(stat);
}

bool StatisticProcessingEngine::handleStatisticEngineClockEvent(Cycle_t CycleNum, SimTime_t timeFactor) 
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
        performStatisticOutput(stat);
    }
    // Return false to keep the clock going
    return false;
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
    
    // The CompStatMap has Component ID registered, get the array assocated with it    
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

void StatisticProcessingEngine::addStatisticToCompStatMap(const ComponentId_t& compId, StatisticBase* Stat, StatisticFieldInfo::fieldType_t fieldType)
{
    StatArray_t*        statArray;
    
    // See if the map contains an entry for this Component ID
    if (m_CompStatMap.find(compId) == m_CompStatMap.end() ) {
        // Nope, Create a new Array of Statistics and relate it to the map
        statArray = new std::vector<StatisticBase*>(); 
        m_CompStatMap[compId] = statArray; 
    }
        
    // The CompStatMap has Component ID registered, get the array assocated with it    
    statArray = m_CompStatMap[compId];

    // Add the statistic to the lists of statistics registered to this component   
    statArray->push_back(Stat);
}

} //namespace Statistics
} //namespace SST

//BOOST_CLASS_EXPORT_IMPLEMENT(SST::Statistics::StatisticProcessingEngine);


