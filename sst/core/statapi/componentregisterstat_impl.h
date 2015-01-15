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


#ifndef _H_SST_CORE_COMP_REG_STAT__
#define _H_SST_CORE_COMP_REG_STAT__

// THIS IS THE IMPLEMENTATION OF Component::registerStatistic(...) code.
// It is placed in this implemenation file to provide a clean interface for 
// the class Component.

//template <typename T>
//Statistic<T>* registerStatistic(Statistic<T>* statistic)
//{
    const char*               fullStatName; 
    bool                      statGood = true;
    bool                      nameFound = false;
    Statistic<T>*             rtnStat = NULL;
    StatisticBase::StatMode_t statCollectionMode = StatisticBase::STAT_MODE_COUNT;
    Simulation::statList_t*   statEnableList;
    Simulation::statList_t*   statRateList;
    Output                    out = Simulation::getSimulation()->getSimulationOutput();

    UnitAlgebra               collectionRate;

    // Make sure statistic is not NULL
    if (NULL != statistic) {
        // Build a name to tast against
        fullStatName = statistic->getFullStatName().c_str();
        
        // Verify here that name of the stat is one of the registered
        // names of the component's ElementInfoStatistic.  
        if (false == doesComponentInfoStatisticExist(statistic->getStatName())) {
//            out.fatal(CALL_INFO, -1, " Error: Statistic %s is not found in ElementInfoStatistic, exiting...\n", fullStatName);
            printf("Error: Statistic %s is not found in ElementInfoStatistic, exiting...\n", fullStatName);
            exit(1);
            statGood = false;
        }

        // Verify that stat is in the group of enabled 
        // stat names identified in the python input file
        statEnableList = Simulation::getSimulation()->getComponentStatisticEnableList(getId());
        statRateList = Simulation::getSimulation()->getComponentStatisticRateList(getId());
        for (uint32_t x = 0; x < statEnableList->size(); x++) {
            // First check to see if the any entry in the StatEnableList is to enable all stats
            // Test the stat name vs the StatEnable list
            // Note "--ALL--" will always be a the top of the list
            if ((std::string("--ALL--") == statEnableList->at(x)) || (statistic->getStatName() == statEnableList->at(x))) {
                // We found an acceptible name, so get its defined rate
                collectionRate = statRateList->at(x);
                nameFound = true;
                break;
            }
        }

        // Did we find a matching enable name?
        if (false == nameFound) {
            out.verbose(CALL_INFO, 1, 0, " Warning: Statistic %s is not enabled in python script, statistic will not be enabled...\n", fullStatName);
            statGood = false;
        }

        // Check that the Collection Rate is a valid unit type that we can use
        if ((true == collectionRate.hasUnits("s")) || 
            (true == collectionRate.hasUnits("hz")) ) {
            // Rate is Periodic Based
            statCollectionMode = StatisticBase::STAT_MODE_PERIODIC;
        } else if (true == collectionRate.hasUnits("event")) {
            // Rate is Count Based
            statCollectionMode = StatisticBase::STAT_MODE_COUNT;
        } else if (0 == collectionRate.getValue()) {
            // Collection rate is zero and has no units, so make up a periodic flavor  
            collectionRate = UnitAlgebra("0ns");
            statCollectionMode = StatisticBase::STAT_MODE_PERIODIC;
        } else {
            // collectionRate is a unit type we dont recognize 
//            out.fatal(CALL_INFO, -1, " ERROR: Statistic %s - Collection Rate = %s not valid; exiting...\n", fullStatName, collectionRate.toString().c_str());
            printf("ERROR: Statistic %s - Collection Rate = %s not valid; exiting...\n", fullStatName, collectionRate.toString().c_str());
            exit(1);
        }
        
        // Check that the statistic supports this collection rate
        if (false == statistic->isStatModeSupported(statCollectionMode)) {
            if (StatisticBase::STAT_MODE_PERIODIC == statCollectionMode) {
                out.verbose(CALL_INFO, 1, 0, " Warning: Statistic %s Does not support Periodic Based Collections; Collection Rate = %s\n", fullStatName, collectionRate.toString().c_str());
            } else {
                out.verbose(CALL_INFO, 1, 0, " Warning: Statistic %s Does not support Event Based Collections; Collection Rate = %s\n", fullStatName, collectionRate.toString().c_str());
            }
            statGood = false;
        }
        
        // Tell the Statistic what collection mode it is in
        statistic->setRegisteredCollectionMode(statCollectionMode);

        // Check the Statistics Enable Level vs the system Load Level to decide
        // if this statistic can be used.
        uint8_t enableLevel = getComponentInfoStatisticEnableLevel(statistic->getStatName());
        uint8_t loadLevel = Simulation::getSimulation()->getStatisticsOutput()->getStatisticLoadLevel();
        if (0 == loadLevel) {
            out.verbose(CALL_INFO, 1, 0, " Warning: Statistic Load Level = 0 (all statistics disabled); statistic %s is disabled...\n", fullStatName);
            statGood = false;
        } else if (0 == enableLevel) {
            out.verbose(CALL_INFO, 1, 0, " Warning: Statistic %s Enable Level = %d, statistic is disabled by the ElementInfoStatistic...\n", fullStatName, enableLevel);
            statGood = false;
        } else if (enableLevel > loadLevel) {
            out.verbose(CALL_INFO, 1, 0, " Warning: Load Level %d is too low to enable Statistic %s with Enable Level %d, statistic will not be enabled...\n", loadLevel, fullStatName, enableLevel);
            statGood = false;
        }
        
        // If Stat is good, Add it to the Statistic Processing Engine
        if (true == statGood) {
            // If the mode is Periodic Based, the add the statistic to the 
            // StatisticProcessingEngine otherwise add it as an Event Based Stat.
            if (StatisticBase::STAT_MODE_PERIODIC == statCollectionMode) {
                if (false == Simulation::getSimulation()->getStatisticsProcessingEngine()->addPeriodicBasedStatistic(collectionRate, statistic)) {
                    statGood = false;
                }
            } else {
                if (false == Simulation::getSimulation()->getStatisticsProcessingEngine()->addEventBasedStatistic(collectionRate, statistic)) {
                    statGood = false;
                }
            }
        }
        
        // If Stats are still good, register its fields and   
        // add it to the array of registered statistics.
        if (true == statGood) {
            // The passed in Statistic is OK to use, register its fields
            StatisticOutput* statOutput = Simulation::getSimulation()->getStatisticsOutput();
            statOutput->startRegisterFields(statistic->getCompName().c_str(), statistic->getStatName().c_str());
            statistic->registerOutputFields(statOutput);
            statOutput->stopRegisterFields();
            
            // Set the statistic to return
            rtnStat = statistic;
        } else {
            // Delete the original statistic, and return a NULL statistic instead
            delete statistic;
            rtnStat = new NullStatistic<T>(this, "NULLSTAT", true);
        }
        
    } else {
        // The ptr to the statistic is NULL, generate an warning and return a NULL Statistic 
        out.output(CALL_INFO, " Warning: Statistic provided is NULL. Providing a NullStatistic instead\n");
        rtnStat = new NullStatistic<T>(this, "NULLSTAT", false);
    }
    
    return rtnStat;
//}

#endif
