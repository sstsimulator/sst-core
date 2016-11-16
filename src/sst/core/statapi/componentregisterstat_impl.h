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


#ifndef _H_SST_CORE_COMP_REG_STAT__
#define _H_SST_CORE_COMP_REG_STAT__

// THIS IS THE IMPLEMENTATION OF Component::registerStatistic(...) code.
// It is placed in this implemenation file to provide a clean interface for 
// the class Component.

//template <typename T>
//Statistic<T>* registerStatisticCore(std::string statName, std::string statSubId = 0)
//{
    std::string                     fullStatName; 
    bool                            statGood = true;
    bool                            nameFound = false;
    StatisticBase::StatMode_t       statCollectionMode = StatisticBase::STAT_MODE_COUNT;
    Simulation::statEnableList_t*   statEnableList;
    Simulation::statParamsList_t*   statParamsList;
    Output                          out = Simulation::getSimulation()->getSimulationOutput();
    UnitAlgebra                     collectionRate;
    UnitAlgebra                     startAtTime;
    UnitAlgebra                     stopAtTime;
    Params                          statParams;
    std::string                     statRateParam;
    std::string                     statTypeParam;
    std::string                     statStartAtTimeParam;
    std::string                     statStopAtTimeParam;
    Statistic<T>*                   statistic = NULL;    
    
    // Check to see if the Statistic is previously registered with the Statistics Engine
    StatisticBase* prevStat = Simulation::getSimulation()->getStatisticsProcessingEngine()->isStatisticRegisteredWithEngine<T>(getName(), getId(), statName, statSubId);
    if (NULL != prevStat) {
        // Dynamic cast the base stat to the expected type
        return dynamic_cast<Statistic<T>*>(prevStat);
    }
    
    // Build a name to report errors against
    fullStatName = StatisticBase::buildStatisticFullName(getName().c_str(), statName, statSubId);

    // Make sure that the wireup has not been completed
    if (true == Simulation::getSimulation()->isWireUpFinished()) {
        // We cannot register statistics AFTER the wireup (after all components have been created) 
        fprintf(stderr, "ERROR: Statistic %s - Cannot be registered after the Components have been wired up.  Statistics must be registered on Component creation.; exiting...\n", fullStatName.c_str());
        exit(1);
    }
    
    // // Verify here that name of the stat is one of the registered
    // // names of the component's ElementInfoStatistic.  
    // if (false == doesComponentInfoStatisticExist(statName)) {
    //     fprintf(stderr, "Error: Statistic %s name %s is not found in ElementInfoStatistic, exiting...\n", fullStatName.c_str(), statName.c_str());
    //     exit(1);
    // }

    // Get Component Statistic Information from the ConfigGraph data
    statEnableList = Simulation::getSimulation()->getComponentStatisticEnableList(getId());
    statParamsList = Simulation::getSimulation()->getComponentStatisticParamsList(getId());
    
    // Check each entry in the StatEnableList (from the ConfigGraph via the 
    // Python File) to see if this Statistic is enabled, then check any of 
    // its critical parameters
    for (uint32_t x = 0; x < statEnableList->size(); x++) {
        // First check to see if the any entry in the StatEnableList matches 
        // the Statistic Name or the STATALLFLAG.  If so, then this Statistic
        // will be enabled.  Then check any critical parameters   
        if ((std::string(STATALLFLAG) == statEnableList->at(x)) || (statName == statEnableList->at(x))) {
            // Identify what keys are Allowed in the parameters
            Params::KeySet_t allowedKeySet;
            allowedKeySet.insert("type");
            allowedKeySet.insert("rate");
            allowedKeySet.insert("startat");
            allowedKeySet.insert("stopat");
            statParamsList->at(x).pushAllowedKeys(allowedKeySet);

            // We found an acceptible name... Now check its critical Parameters
            // Note: If parameter not found, defaults will be provided
            statTypeParam = statParamsList->at(x).find<std::string>("type", "sst.AccumulatorStatistic");
            statRateParam = statParamsList->at(x).find<std::string>("rate", "0ns");
            statStartAtTimeParam = statParamsList->at(x).find<std::string>("startat", "0ns");
            statStopAtTimeParam = statParamsList->at(x).find<std::string>("stopat", "0ns");

            // Check for an empty string on the collection rate and start/stop times 
            if (true == statRateParam.empty()) {
                statRateParam = "0ns";
            }
            if (true == statStartAtTimeParam.empty()) {
                statStartAtTimeParam = "0ns";
            }
            if (true == statStopAtTimeParam.empty()) {
                statStopAtTimeParam = "0ns";
            }
            
            collectionRate = UnitAlgebra(statRateParam); 
            startAtTime = UnitAlgebra(statStartAtTimeParam);
            stopAtTime = UnitAlgebra(statStopAtTimeParam);
            statParams = statParamsList->at(x);
            nameFound = true;
            break;
        }
    }

    // Did we find a matching enable name?
    if (false == nameFound) {
        statGood = false;
        out.verbose(CALL_INFO, 1, 0, " Warning: Statistic %s is not enabled in python script, statistic will not be enabled...\n", fullStatName.c_str());
    }

    if (true == statGood) {
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
            fprintf(stderr, "ERROR: Statistic %s - Collection Rate = %s not valid; exiting...\n", fullStatName.c_str(), collectionRate.toString().c_str());
            exit(1);
        }
        
        // Check the startat and stopat is in units of seconds
        if ((true != startAtTime.hasUnits("s")) || (true != stopAtTime.hasUnits("s"))) {
            // startat or stopat has a unit type we dont allow 
            fprintf(stderr, "ERROR: Statistic %s - param startat = %s; stopat = %s must both be in units of seconds; exiting...\n", fullStatName.c_str(), startAtTime.toString().c_str(), stopAtTime.toString().c_str());
            exit(1);
        }
    }

    if (true == statGood) {
        // Instantiate the Statistic here defined by the type here
        statistic = CreateStatistic<T>(this, statTypeParam, statName, statSubId, statParams);
        if (NULL == statistic) {
            fprintf(stderr, "ERROR: Unable to instantiate Statistic %s; exiting...\n", fullStatName.c_str());
            exit(1);
        }

        // Check that the statistic supports this collection rate
        if (false == statistic->isStatModeSupported(statCollectionMode)) {
            if (StatisticBase::STAT_MODE_PERIODIC == statCollectionMode) {
                out.verbose(CALL_INFO, 1, 0, " Warning: Statistic %s Does not support Periodic Based Collections; Collection Rate = %s\n", fullStatName.c_str(), collectionRate.toString().c_str());
            } else {
                out.verbose(CALL_INFO, 1, 0, " Warning: Statistic %s Does not support Event Based Collections; Collection Rate = %s\n", fullStatName.c_str(), collectionRate.toString().c_str());
            }
            statGood = false;
        }
    
        // Tell the Statistic what collection mode it is in
        statistic->setRegisteredCollectionMode(statCollectionMode);
    }
    
    // If Stat is good, Add it to the Statistic Processing Engine
    if (true == statGood) {
        // Check the Statistics Enable Level vs the system Load Level to decide
        // if this statistic can be used.
        uint8_t enableLevel = getComponentInfoStatisticEnableLevel(statistic->getStatName());
        uint8_t loadLevel = Simulation::getStatisticsOutput()->getStatisticLoadLevel();
        if (0 == loadLevel) {
            out.verbose(CALL_INFO, 1, 0, " Warning: Statistic Load Level = 0 (all statistics disabled); statistic %s is disabled...\n", fullStatName.c_str());
            statGood = false;
        // } else if (0 == enableLevel) {
        //     out.verbose(CALL_INFO, 1, 0, " Warning: Statistic %s Enable Level = %d, statistic is disabled by the ElementInfoStatistic...\n", fullStatName.c_str(), enableLevel);
        //     statGood = false;
        } else if (enableLevel > loadLevel) {
            out.verbose(CALL_INFO, 1, 0, " Warning: Load Level %d is too low to enable Statistic %s with Enable Level %d, statistic will not be enabled...\n", loadLevel, fullStatName.c_str(), enableLevel);
            statGood = false;
        }
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
        statOutput->startRegisterFields(statistic);
        statistic->registerOutputFields(statOutput);
        statOutput->stopRegisterFields();

        // Set the start / stop times for the stat
        Simulation::getSimulation()->getStatisticsProcessingEngine()->setStatisticStartTime(startAtTime, statistic);
        Simulation::getSimulation()->getStatisticsProcessingEngine()->setStatisticStopTime(stopAtTime, statistic);
    } else {
        // Delete the original statistic (if created), and return a NULL statistic instead
        if (NULL != statistic) {
            delete statistic;
        }
        
        // Instantiate the Statistic here defined by the type here
        statTypeParam = "sst.NullStatistic";
        statistic = CreateStatistic<T>(this, statTypeParam, statName, statSubId, statParams);
        if (NULL == statistic) {
            statGood = false;
            fprintf(stderr, "ERROR: Unable to instantiate Null Statistic %s; exiting...\n", fullStatName.c_str());
            exit(1);
        }
    }

    // Register the new Statistic with the Statistic Engine
    Simulation::getSimulation()->getStatisticsProcessingEngine()->registerStatisticWithEngine<T>(getId(), statistic);
    return statistic;
//}

#endif //_H_SST_CORE_COMP_REG_STAT__
