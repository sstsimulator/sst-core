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


#ifndef _H_SST_CORE_COMP_REG_STAT__
#define _H_SST_CORE_COMP_REG_STAT__

// THIS IS THE IMPLEMENTATION OF Component::registerStatistic(...) code.
// It is placed in this implementation file to provide a clean interface for 
// the class Component.

//template <typename T>
//Statistic<T>* registerStatisticCore(std::string statName, std::string statSubId = 0)
//{
    std::string                     fullStatName; 
    bool                            statGood = true;
    bool                            nameFound = false;
    StatisticBase::StatMode_t       statCollectionMode = StatisticBase::STAT_MODE_COUNT;
    Output &                        out = getSimulation()->getSimulationOutput();
    StatisticProcessingEngine      *engine = StatisticProcessingEngine::getInstance();
    UnitAlgebra                     collectionRate;
    Params                          statParams;
    std::string                     statRateParam;
    std::string                     statTypeParam;
    Statistic<T>*                   statistic = NULL;


    // Build a name to report errors against
    fullStatName = StatisticBase::buildStatisticFullName(getName().c_str(), statName, statSubId);

    // Make sure that the wireup has not been completed
    if (true == getSimulation()->isWireUpFinished()) {
        // We cannot register statistics AFTER the wireup (after all components have been created) 
        out.fatal(CALL_INFO, 1, "ERROR: Statistic %s - Cannot be registered after the Components have been wired up.  Statistics must be registered on Component creation.; exiting...\n", fullStatName.c_str());
    }

    /* Create the statistic in the "owning" component.  That should just be us, 
     * in the case of 'modern' subcomponents.  For legacy subcomponents, that will
     * be the owning component.  We've got the ID of that component in 'my_info->getID()'
     */
    BaseComponent *owner = this->getStatisticOwner();

    // // Verify here that name of the stat is one of the registered
    // // names of the component's ElementInfoStatistic.
    // if (false == doesComponentInfoStatisticExist(statName)) {
    //     fprintf(stderr, "Error: Statistic %s name %s is not found in ElementInfoStatistic, exiting...\n", fullStatName.c_str(), statName.c_str());
    //     exit(1);
    // }

    // Check each entry in the StatEnableList (from the ConfigGraph via the 
    // Python File) to see if this Statistic is enabled, then check any of 
    // its critical parameters
    for ( auto & si : *my_info->getStatEnableList() ) {
        // First check to see if the any entry in the StatEnableList matches 
        // the Statistic Name or the STATALLFLAG.  If so, then this Statistic
        // will be enabled.  Then check any critical parameters   
        if ((std::string(STATALLFLAG) == si.name) || (statName == si.name)) {
            // Identify what keys are Allowed in the parameters
            Params::KeySet_t allowedKeySet;
            allowedKeySet.insert("type");
            allowedKeySet.insert("rate");
            allowedKeySet.insert("startat");
            allowedKeySet.insert("stopat");
            allowedKeySet.insert("resetOnRead");
            si.params.pushAllowedKeys(allowedKeySet);

            // We found an acceptable name... Now check its critical Parameters
            // Note: If parameter not found, defaults will be provided
            statTypeParam = si.params.find<std::string>("type", "sst.AccumulatorStatistic");
            statRateParam = si.params.find<std::string>("rate", "0ns");

            collectionRate = UnitAlgebra(statRateParam);
            statParams = si.params;
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
            out.fatal(CALL_INFO, 1, "ERROR: Statistic %s - Collection Rate = %s not valid; exiting...\n", fullStatName.c_str(), collectionRate.toString().c_str());
        }
    }

    if (true == statGood) {
        // Instantiate the Statistic here defined by the type here
        statistic = engine->createStatistic<T>(owner, statTypeParam, statName, statSubId, statParams);
        if (NULL == statistic) {
            out.fatal(CALL_INFO, 1, "ERROR: Unable to instantiate Statistic %s; exiting...\n", fullStatName.c_str());
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
        statGood = engine->registerStatisticWithEngine<T>(statistic);
    }

    if (false == statGood ) {
        // Delete the original statistic (if created), and return a NULL statistic instead
        if (NULL != statistic) {
            delete statistic;
        }

        // Instantiate the Statistic here defined by the type here
        statTypeParam = "sst.NullStatistic";
        statistic = engine->createStatistic<T>(owner, statTypeParam, statName, statSubId, statParams);
        if (NULL == statistic) {
            statGood = false;
            out.fatal(CALL_INFO, 1, "ERROR: Unable to instantiate Null Statistic %s; exiting...\n", fullStatName.c_str());
        }
        engine->registerStatisticWithEngine<T>(statistic);
    }

    // Register the new Statistic with the Statistic Engine
    return statistic;
//}

#endif //_H_SST_CORE_COMP_REG_STAT__
