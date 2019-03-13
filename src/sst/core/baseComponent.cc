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

#include <string>

#include <sst/core/baseComponent.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/factory.h>
#include <sst/core/link.h>
#include <sst/core/linkMap.h>
#include <sst/core/simulation.h>
#include <sst/core/timeConverter.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/sharedRegion.h>

namespace SST {


class SubComponentSlotInfo_impl : public SubComponentSlotInfo {
private:

    BaseComponent* comp;
    std::string slot_name;
    int max_slot_index;
    
public:

    SubComponent* protected_create(int slot_num, Params& params) const {
        if ( slot_num > max_slot_index ) return NULL;

        return comp->loadNamedSubComponent(slot_name, slot_num, params);
    }
    
    ~SubComponentSlotInfo_impl() {}
    
    SubComponentSlotInfo_impl(BaseComponent* comp, std::string slot_name) :
        comp(comp),
        slot_name(slot_name)
    {
        const std::vector<ComponentInfo>& subcomps = comp->my_info->getSubComponents();

        // Look for all subcomponents with the right slot name
        max_slot_index = -1;
        for ( auto &ci : subcomps ) {
            if ( ci.getSlotName() == slot_name ) {
                if ( ci.getSlotNum() > static_cast<int>(max_slot_index) ) {
                    max_slot_index = ci.getSlotNum();
                }
            }
        }
    }

    const std::string& getSlotName() const {
        return slot_name;
    }
    
    bool isPopulated(int slot_num) const {
        if ( slot_num > max_slot_index ) return false;
        if ( comp->my_info->findSubComponent(slot_name,slot_num) == NULL ) return false;
        return true;
    }
    
    bool isAllPopulated() const {
        for ( int i = 0; i < max_slot_index; ++i ) {
            if ( comp->my_info->findSubComponent(slot_name,i) == NULL ) return false;
        }
        return true;
    }

    int getMaxPopulatedSlotNumber() const {
        return max_slot_index;
    }
};

BaseComponent::BaseComponent() :
    defaultTimeBase(NULL), my_info(NULL),
    sim(Simulation::getSimulation()),
    currentlyLoadingSubComponent(NULL)
{
}


BaseComponent::~BaseComponent()
{
}



TimeConverter* BaseComponent::registerClock( std::string freq, Clock::HandlerBase* handler, bool regAll) {
    TimeConverter* tc = getSimulation()->registerClock(freq, handler, CLOCKPRIORITY);

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        LinkMap* myLinks = my_info->getLinkMap();
        if (NULL != myLinks) {
            for ( std::pair<std::string,Link*> p : myLinks->getLinkMap() ) {
                if ( NULL == p.second->getDefaultTimeBase() ) {
                    p.second->setDefaultTimeBase(tc);
                }
            }
        }
	defaultTimeBase = tc;
    }
    return tc;
}

TimeConverter* BaseComponent::registerClock( const UnitAlgebra& freq, Clock::HandlerBase* handler, bool regAll) {
    TimeConverter* tc = getSimulation()->registerClock(freq, handler, CLOCKPRIORITY);

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        LinkMap* myLinks = my_info->getLinkMap();
        if (NULL != myLinks) {
            for ( std::pair<std::string,Link*> p : myLinks->getLinkMap() ) {
                if ( NULL == p.second->getDefaultTimeBase() ) {
                    p.second->setDefaultTimeBase(tc);
                }
            }
        }
	defaultTimeBase = tc;
    }
    return tc;
}

Cycle_t BaseComponent::reregisterClock( TimeConverter* freq, Clock::HandlerBase* handler) {
    return getSimulation()->reregisterClock(freq, handler, CLOCKPRIORITY);
}

Cycle_t BaseComponent::getNextClockCycle( TimeConverter* freq ) {
    return getSimulation()->getNextClockCycle(freq, CLOCKPRIORITY);
}

void BaseComponent::unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler) {
    getSimulation()->unregisterClock(tc, handler, CLOCKPRIORITY);
}

TimeConverter* BaseComponent::registerOneShot( std::string timeDelay, OneShot::HandlerBase* handler) {
    return getSimulation()->registerOneShot(timeDelay, handler, ONESHOTPRIORITY);
}

TimeConverter* BaseComponent::registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler) {
    return getSimulation()->registerOneShot(timeDelay, handler, ONESHOTPRIORITY);
}

TimeConverter* BaseComponent::registerTimeBase( std::string base, bool regAll) {
    TimeConverter* tc = getSimulation()->getTimeLord()->getTimeConverter(base);

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        LinkMap* myLinks = my_info->getLinkMap();
        if (NULL != myLinks) {
            for ( std::pair<std::string,Link*> p : myLinks->getLinkMap() ) {
                if ( NULL == p.second->getDefaultTimeBase() ) {
                    p.second->setDefaultTimeBase(tc);
                }
            }
        }
	defaultTimeBase = tc;
    }
    return tc;
}

TimeConverter*
BaseComponent::getTimeConverter( const std::string& base )
{
    return getSimulation()->getTimeLord()->getTimeConverter(base);
}

TimeConverter*
BaseComponent::getTimeConverter( const UnitAlgebra& base )
{
    return getSimulation()->getTimeLord()->getTimeConverter(base);
}


bool
BaseComponent::isPortConnected(const std::string &name) const
{
    return (my_info->getLinkMap()->getLink(name) != NULL);
}

Link*
BaseComponent::configureLink(std::string name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    LinkMap* myLinks = my_info->getLinkMap();
    Link* tmp = myLinks->getLink(name);
    if ( tmp == NULL ) return NULL;

    // If no functor, this is a polling link
    if ( handler == NULL ) {
        tmp->setPolling();
    }
    tmp->setFunctor(handler);
    if ( time_base != NULL ) tmp->setDefaultTimeBase(time_base);
    else tmp->setDefaultTimeBase(defaultTimeBase);
#ifdef __SST_DEBUG_EVENT_TRACKING__
    tmp->setSendingComponentInfo(my_info->getName(), my_info->getType(), name);
#endif
    return tmp;
}

Link*
BaseComponent::configureLink(std::string name, std::string time_base, Event::HandlerBase* handler)
{
    return configureLink(name,getSimulation()->getTimeLord()->getTimeConverter(time_base),handler);
}

Link*
BaseComponent::configureLink(std::string name, Event::HandlerBase* handler)
{
    return configureLink(name,NULL,handler);
}

void
BaseComponent::addSelfLink(std::string name)
{
    LinkMap* myLinks = my_info->getLinkMap();
    myLinks->addSelfPort(name);
    if ( myLinks->getLink(name) != NULL ) {
        printf("Attempting to add self link with duplicate name: %s\n",name.c_str());
	abort();
    }

    Link* link = new SelfLink();
    // Set default time base to the component time base
    link->setDefaultTimeBase(defaultTimeBase);
    myLinks->insertLink(name,link);

}

Link*
BaseComponent::configureSelfLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,time_base,handler);
}

Link*
BaseComponent::configureSelfLink( std::string name, std::string time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,time_base,handler);
}

Link*
BaseComponent::configureSelfLink( std::string name, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,handler);
}

SimTime_t BaseComponent::getCurrentSimTime(TimeConverter *tc) const {
    return tc->convertFromCoreTime(getSimulation()->getCurrentSimCycle());
}

SimTime_t BaseComponent::getCurrentSimTime(std::string base) {
    return getCurrentSimTime(getSimulation()->getTimeLord()->getTimeConverter(base));

}

SimTime_t BaseComponent::getCurrentSimTimeNano() const {
    return getCurrentSimTime(getSimulation()->getTimeLord()->getNano());
}

SimTime_t BaseComponent::getCurrentSimTimeMicro() const {
    return getCurrentSimTime(getSimulation()->getTimeLord()->getMicro());
}

SimTime_t BaseComponent::getCurrentSimTimeMilli() const {
    return getCurrentSimTime(getSimulation()->getTimeLord()->getMilli());
}



Module*
BaseComponent::loadModule(std::string type, Params& params)
{
    return Factory::getFactory()->CreateModule(type,params);
}

Module*
BaseComponent::loadModuleWithComponent(std::string type, Component* comp, Params& params)
{
    return Factory::getFactory()->CreateModuleWithComponent(type,comp,params);
}

/* Old ELI style */
SubComponent*
BaseComponent::loadSubComponent(std::string type, Component* comp, Params& params)
{
    /* Old Style SubComponents end up with their parent's Id, name, etc. */
    // ComponentInfo *sub_info = new ComponentInfo(type, &params, comp->my_info);
    ComponentInfo *sub_info = new ComponentInfo(type, &params, getTrueComponent()->currentlyLoadingSubComponent);
    ComponentInfo *oldLoadingSubcomponent = getTrueComponent()->currentlyLoadingSubComponent;
    /* By "magic", the new component will steal ownership of this pointer */
    getTrueComponent()->currentlyLoadingSubComponent = sub_info;

    SubComponent* ret = Factory::getFactory()->CreateSubComponent(type,comp,params);
    sub_info->setComponent(ret);
    getTrueComponent()->currentlyLoadingSubComponent = oldLoadingSubcomponent;
    return ret;
}

/* New ELI style */
SubComponent*
BaseComponent::loadNamedSubComponent(std::string name) {
    Params empty;
    return loadNamedSubComponent(name, empty);
}

SubComponent*
BaseComponent::loadNamedSubComponent(std::string name, Params& params) {
    // Get list of ComponentInfo objects and make sure that there is
    // only one SubComponent put into this slot
    const std::vector<ComponentInfo>& subcomps = my_info->getSubComponents();
    int sub_count = 0;
    for ( auto &ci : subcomps ) {
        if ( ci.getSlotName() == name ) {
            sub_count++;
        }
    }
    
    if ( sub_count > 1 ) {
        SST::Output outXX("SubComponentSlotWarning: ", 0, 0, Output::STDERR);
        outXX.fatal(CALL_INFO, 1, "Error: ComponentSlot \"%s\" in component \"%s\" only allows for one SubComponent, %d provided.\n",
                    name.c_str(), my_info->getType().c_str(), sub_count);
    }
    
    return loadNamedSubComponent(name, 0, params);
}

SubComponent*
BaseComponent::loadNamedSubComponent(std::string name, int slot_num) {
    Params empty;
    return loadNamedSubComponent(name, slot_num, empty);
}

SubComponent*
BaseComponent::loadNamedSubComponent(std::string name, int slot_num, Params& params)
{
    // auto infoItr = my_info->getSubComponents().find(name);
    // if ( infoItr == my_info->getSubComponents().end() ) return NULL;
    if ( !Factory::getFactory()->DoesSubComponentSlotExist(my_info->type, name) ) {
        SST::Output outXX("SubComponentSlotWarning: ", 0, 0, Output::STDERR);
        outXX.output(CALL_INFO, "Warning: SubComponentSlot \"%s\" is undocumented.\n", name.c_str());
    }
    
    ComponentInfo* sub_info = my_info->findSubComponent(name,slot_num);
    if ( sub_info == NULL ) return NULL;
    
    ComponentInfo *oldLoadingSubcomponent = getTrueComponent()->currentlyLoadingSubComponent;
    // ComponentInfo *sub_info = &(infoItr->second);
    getTrueComponent()->currentlyLoadingSubComponent = sub_info;

    Params myParams;
    if ( sub_info->getParams() != NULL )
        myParams.insert(*sub_info->getParams());
    myParams.insert(params);

    SubComponent* ret = Factory::getFactory()->CreateSubComponent(sub_info->getType(), getTrueComponent(), myParams);
    sub_info->setComponent(ret);

    getTrueComponent()->currentlyLoadingSubComponent = oldLoadingSubcomponent;
    return ret;
}

SubComponentSlotInfo*
BaseComponent::getSubComponentSlotInfo(std::string name, bool fatalOnEmptyIndex) {
    SubComponentSlotInfo_impl* info = new SubComponentSlotInfo_impl(this, name);
    if ( info->getMaxPopulatedSlotNumber() < 0 ) {
        // Nothing registered on this slot
        delete info;
        return NULL;
    }
    if ( !info->isAllPopulated() && fatalOnEmptyIndex ) {
        Simulation::getSimulationOutput().
            fatal(CALL_INFO,1,
                  "SubComponent slot %s requires a dense allocation of SubComponents and did not get one.\n",
                  name.c_str());
    }
    return info;
}


SharedRegion* BaseComponent::getLocalSharedRegion(const std::string &key, size_t size)
{
    SharedRegionManager *mgr = Simulation::getSharedRegionManager();
    return mgr->getLocalSharedRegion(key, size);
}


SharedRegion* BaseComponent::getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger)
{
    SharedRegionManager *mgr = Simulation::getSharedRegionManager();
    return mgr->getGlobalSharedRegion(key, size, merger);
}



uint8_t BaseComponent::getComponentInfoStatisticEnableLevel(const std::string &statisticName) const
{
    const std::string& type = getStatisticOwner()->my_info->getType();
    return Factory::getFactory()->GetComponentInfoStatisticEnableLevel(type, statisticName);
}

std::string BaseComponent::getComponentInfoStatisticUnits(const std::string &statisticName) const
{
    const std::string& type = getStatisticOwner()->my_info->getType();
    return Factory::getFactory()->GetComponentInfoStatisticUnits(type, statisticName);
}



StatisticBase* 
BaseComponent::registerStatisticCore(SST::Params& params, const std::string& statName, const std::string& statSubId,
                                     fieldType_t fieldType, CreateFxn&& create)
{
    SST::Params statParams = params.find_prefix_params(statName);
    std::string                     fullStatName; 
    bool                            statGood = true;
    bool                            nameFound = false;
    StatisticBase::StatMode_t       statCollectionMode = StatisticBase::STAT_MODE_COUNT;
    Output &                        out = getSimulation()->getSimulationOutput();
    StatisticProcessingEngine      *engine = StatisticProcessingEngine::getInstance();
    UnitAlgebra                     collectionRate;
    std::string                     statRateParam;
    std::string                     statTypeParam;
    StatisticBase*                   statistic = NULL;


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
            statParams.insert(si.params);
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
        //statistic = engine->createStatistic<T>(owner, statTypeParam, statName, statSubId, statParams);
        statistic = create(statTypeParam, owner, statName, statSubId, statParams);
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
        statGood = engine->registerStatisticWithEngine(statistic,fieldType);
    }

    if (false == statGood ) {
        // Delete the original statistic (if created), and return a NULL statistic instead
        if (NULL != statistic) {
            delete statistic;
        }

        // Instantiate the Statistic here defined by the type here
        statTypeParam = "sst.NullStatistic";
        statistic = create(statTypeParam, owner, statName, statSubId, statParams);
        if (NULL == statistic) {
            statGood = false;
            out.fatal(CALL_INFO, 1, "ERROR: Unable to instantiate Null Statistic %s; exiting...\n", fullStatName.c_str());
        }
        engine->registerStatisticWithEngine(statistic, fieldType);
    }

    // Register the new Statistic with the Statistic Engine
    return statistic;
}

} // namespace SST


