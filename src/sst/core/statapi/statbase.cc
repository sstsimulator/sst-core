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

#include <sst/core/baseComponent.h>
#include <sst/core/statapi/statbase.h>

namespace SST {
namespace Statistics {

StatisticBase::StatisticBase(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams)
{
    m_statName   = statName;
    m_statSubId  = statSubId;
    m_component  = comp;
    m_statParams = statParams;

    initializeProperties();

    m_resetCountOnOutput = statParams.find<bool>("resetOnRead", false);
    m_clearDataOnOutput = statParams.find<bool>("resetOnRead", false);

}

const std::string& StatisticBase::getCompName() const
{
    return m_component->getName();
}

void StatisticBase::incrementCollectionCount()  
{
    m_currentCollectionCount++;
    checkEventForOutput();
}                       

void StatisticBase::setCollectionCount(uint64_t newCount)
{
    m_currentCollectionCount = newCount;
    checkEventForOutput();
}   

void StatisticBase::resetCollectionCount() 
{
    m_currentCollectionCount = 0;
}

void StatisticBase::setCollectionCountLimit(uint64_t newLimit) 
{   
    m_collectionCountLimit = newLimit;
    checkEventForOutput();
}  

std::string StatisticBase::buildStatisticFullName(const char* compName, const char* statName, const char* statSubId)
{
    return buildStatisticFullName(std::string(compName), std::string(statName), std::string(statSubId));
}

std::string StatisticBase::buildStatisticFullName(const std::string& compName, const std::string& statName, const std::string& statSubId)
{
    std::string statFullNameRtn;
    
    statFullNameRtn = compName + ".";
    statFullNameRtn += statName;
    if (statSubId != "") {
        statFullNameRtn += ".";
        statFullNameRtn += statSubId;
    }
    return statFullNameRtn;
}

void StatisticBase::initializeProperties() 
{
    m_statFullName = buildStatisticFullName(getCompName(), m_statName, m_statSubId);
    m_registeredCollectionMode = STAT_MODE_UNDEFINED;
    m_statEnabled = true;
    m_outputEnabled = true;
    m_currentCollectionCount = 0;
    m_collectionCountLimit = 100;
    m_resetCountOnOutput = false;
    m_clearDataOnOutput = false;
    m_outputAtEndOfSim = true;
    m_outputDelayed = false;
    m_collectionDelayed = false;
    m_savedStatEnabled = true;
    m_savedOutputEnabled = true;
    m_outputDelayedHandler = new OneShot::Handler<StatisticBase>(this, &StatisticBase::delayOutputExpiredHandler);
    m_collectionDelayedHandler = new OneShot::Handler<StatisticBase>(this, &StatisticBase::delayCollectionExpiredHandler);
}

void StatisticBase::checkEventForOutput()
{
    if ( (m_registeredCollectionMode == STAT_MODE_COUNT) && 
         (m_currentCollectionCount >= m_collectionCountLimit) &&
         (1 <= m_collectionCountLimit) ) {
        // Dont output if CountLimit is zero
        StatisticProcessingEngine::getInstance()->performStatisticOutput(this);
    }
}

bool StatisticBase::operator==(StatisticBase& checkStat) 
{
    return (getFullStatName()  == checkStat.getFullStatName());
}

void StatisticBase::delayOutput(const char* delayTime)
{
    // Make sure only a single output delay is active
    if (false == m_outputDelayed) {

        // Save the Stat Output Enable setting and then disable the output 
        m_savedOutputEnabled = m_outputEnabled;
        m_outputEnabled = false;
        m_outputDelayed = true;

        Simulation::getSimulation()->registerOneShot(delayTime, m_outputDelayedHandler, STATISTICCLOCKPRIORITY);
    }
}

void StatisticBase::delayCollection(const char* delayTime) 
{
    // Make sure only a single collection delay is active
    if (false == m_collectionDelayed) {
    
        // Save the Stat Enable setting and then disable the Stat for collection
        m_savedStatEnabled = m_statEnabled;
        m_statEnabled = false;
        m_collectionDelayed = true;

        Simulation::getSimulation()->registerOneShot(delayTime, m_collectionDelayedHandler, STATISTICCLOCKPRIORITY);
    }
}

void StatisticBase::delayOutputExpiredHandler()
{
    // Restore the Output Enable to its stored value
    m_outputEnabled = m_savedOutputEnabled;
    m_outputDelayed = false;
}

void StatisticBase::delayCollectionExpiredHandler()
{
    // Restore the Statistic Enable to its stored value
    m_statEnabled = m_savedStatEnabled;
    m_collectionDelayed = false;
}

} //namespace Statistics
} //namespace SST
