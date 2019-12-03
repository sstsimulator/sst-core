// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/baseComponent.h"

#include "sst/core/warnmacros.h"

#include <string>

#include "sst/core/component.h"
#include "sst/core/subcomponent.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/factory.h"
#include "sst/core/link.h"
#include "sst/core/linkMap.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/sharedRegion.h"

namespace SST {


BaseComponent::BaseComponent(ComponentId_t id) :
    sim(Simulation::getSimulation()),
    loadedWithLegacyAPI(false),
    my_info(Simulation::getSimulation()->getComponentInfo(id)),
    currentlyLoadingSubComponent(nullptr),
    isExtension(false)
{
    if ( my_info->component == nullptr ) {
        // If it's already set, then this is a ComponentExtension and
        // we shouldn't reset it.
        my_info->component = this;
    }
}


BaseComponent::~BaseComponent()
{
    // Need to cleanup my ComponentInfo and delete all my children.

    // If my_info is nullptr, then we are being deleted by our
    // ComponentInfo object.  This happens at the end of execution
    // when the simulation destructor fires.
    if ( !my_info ) return;
    if ( isExtension ) return;

    // Start by deleting children    
    std::map<ComponentId_t,ComponentInfo>& subcomps = my_info->getSubComponents();
    for ( auto &ci : subcomps ) {
        // Delete the subcomponent

        // Remove the parent info from the child so that it won't try
        // to delete itself out of the map.  We'll clear the map
        // after deleting everything.
        ci.second.parent_info = nullptr;
        delete ci.second.component;
        ci.second.component = nullptr;
    }
    // Now clear the map.  This will delete all the ComponentInfo
    // objects; since the component field was set to nullptr, it will
    // not try to delete the component again.
    subcomps.clear();

    // Now for the tricky part, I need to remove myself from my
    // parent's subcomponent map (if I have a parent).
    my_info->component = nullptr;
    if ( my_info->parent_info ) {
        std::map<ComponentId_t,ComponentInfo>& parent_subcomps = my_info->parent_info->getSubComponents();
        size_t deleted = parent_subcomps.erase(my_info->id);
        if ( deleted != 1 ) {
            // This can't be checked while we still have backward
            // compatibility to the old subcomponent API.  Making
            // calls directly on a subcomponent/component makes the
            // structure imperfect.

            // // Should never happen, but issue warning just in case
            // Simulation::getSimulationOutput().
            //     output("Warning:  BaseComponent destructor failed to remove ComponentInfo from parent.\n");
        }
    }
}

void
BaseComponent::setDefaultTimeBaseForParentLinks(TimeConverter* tc) {
    LinkMap* myLinks = my_info->getLinkMap();
    if (nullptr != myLinks) {
        for ( std::pair<std::string,Link*> p : myLinks->getLinkMap() ) {
            // if ( nullptr == p.second->getDefaultTimeBase() ) {
            if ( nullptr == p.second->getDefaultTimeBase() && p.second->isConfigured() ) {
                p.second->setDefaultTimeBase(tc);
            }
        }
    }

    // Need to look through up through my parent chain, if I'm legacy
    // anonymous.
    if ( my_info->isLegacySubComponent() ) {
        my_info->parent_info->component->setDefaultTimeBaseForParentLinks(tc);
    }
}
void
BaseComponent::setDefaultTimeBaseForChildLinks(TimeConverter* tc) {
    LinkMap* myLinks = my_info->getLinkMap();
    if (nullptr != myLinks) {
        for ( std::pair<std::string,Link*> p : myLinks->getLinkMap() ) {
            // if ( nullptr == p.second->getDefaultTimeBase() ) {
            if ( nullptr == p.second->getDefaultTimeBase() && p.second->isConfigured() ) {
                p.second->setDefaultTimeBase(tc);
            }
        }
    }

    // Need to look through my child subcomponents and for all legacy
    // anonymously loaded subcomponents, set the default time base for
    // any links they have.  These links would have been moved from
    // the parent to the child.
    for ( auto &sub : my_info->subComponents ) {
        if ( sub.second.isLegacySubComponent() ) {
            sub.second.component->setDefaultTimeBaseForChildLinks(tc);
        }
    }    
}

void
BaseComponent::setDefaultTimeBaseForLinks(TimeConverter* tc) {
    LinkMap* myLinks = my_info->getLinkMap();
    if (nullptr != myLinks) {
        for ( std::pair<std::string,Link*> p : myLinks->getLinkMap() ) {
            // if ( nullptr == p.second->getDefaultTimeBase() ) {
            if ( nullptr == p.second->getDefaultTimeBase() && p.second->isConfigured() ) {
                p.second->setDefaultTimeBase(tc);
            }
        }
    }

    // Need to look through my child subcomponents and for all
    // anonymously loaded subcomponents, set the default time base for
    // any links they have.  These links would have been moved from
    // the parent to the child.
    for ( auto &sub : my_info->subComponents ) {
        if ( sub.second.isLegacySubComponent() ) {
            sub.second.component->setDefaultTimeBaseForLinks(tc);
        }
    }

    // Need to look through up through my parent chain, if I'm
    // anonymous.
    if ( my_info->isLegacySubComponent() ) {
        my_info->parent_info->component->setDefaultTimeBaseForParentLinks(tc);
    }

}

void
BaseComponent::pushValidParams(Params& params, const std::string& type)
{
    const Params::KeySet_t& keyset = Factory::getFactory()->getParamNames(type);
    params.pushAllowedKeys(keyset);
}


TimeConverter* BaseComponent::registerClock( const std::string& freq, Clock::HandlerBase* handler, bool regAll) {
    TimeConverter* tc = getSimulation()->registerClock(freq, handler, CLOCKPRIORITY);

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        setDefaultTimeBaseForLinks(tc);
        my_info->defaultTimeBase = tc;
    }
    return tc;
}

TimeConverter* BaseComponent::registerClock( const UnitAlgebra& freq, Clock::HandlerBase* handler, bool regAll) {
    TimeConverter* tc = getSimulation()->registerClock(freq, handler, CLOCKPRIORITY);

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        setDefaultTimeBaseForLinks(tc);
        my_info->defaultTimeBase = tc;
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

TimeConverter* BaseComponent::registerOneShot( const std::string& timeDelay, OneShot::HandlerBase* handler) {
    return getSimulation()->registerOneShot(timeDelay, handler, ONESHOTPRIORITY);
}

TimeConverter* BaseComponent::registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler) {
    return getSimulation()->registerOneShot(timeDelay, handler, ONESHOTPRIORITY);
}

TimeConverter* BaseComponent::registerTimeBase( const std::string& base, bool regAll) {
    TimeConverter* tc = getSimulation()->getTimeLord()->getTimeConverter(base);

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        setDefaultTimeBaseForLinks(tc);
        my_info->defaultTimeBase = tc;
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
BaseComponent::isPortConnected(const std::string& name) const
{
    return (my_info->getLinkMap()->getLink(name) != nullptr);
}


// Looks at parents' shared ports and returns the link connected to
// the port of the correct name in one of my parents. If I find the
// correct link, and it hasn't been configured yet, I return it to the
// child and remove it from my linkmap.  The child will insert it into
// their link map.
Link*
BaseComponent::getLinkFromParentSharedPort(const std::string& port)
{
    LinkMap* myLinks = my_info->getLinkMap();
    
    // See if the link is found, and if not see if my parent shared
    // their ports with me
    
    if ( nullptr != myLinks ) {
        Link* tmp = myLinks->getLink(port);
        if ( nullptr != tmp ) {
            // Found the link in my linkmap

            // Check to see if it has been configured.  If not, remove
            // it from my link map and return it to the child.
            if ( !tmp->isConfigured() ) {
                myLinks->removeLink(port);
                return tmp;
            }
        }
    }

    // If we get here, we didn't find the link.  Check to see if my
    // parent shared with me and if so, call
    // getLinkFromParentSharedPort on them
        
    if ( my_info->sharesPorts() ) {
        return my_info->parent_info->component->getLinkFromParentSharedPort(port);
    }
    else {
        return nullptr;
    }    
}


Link*
BaseComponent::configureLink(const std::string& name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    LinkMap* myLinks = my_info->getLinkMap();

    Link* tmp = nullptr;
    
    // If I have a linkmap, check to see if a link was connected to
    // port "name"
    if ( nullptr != myLinks ) {
        tmp = myLinks->getLink(name);
    }
    // If tmp is nullptr, then I didn't have the port connected, check
    // with parents if sharing is turned on
    if ( nullptr == tmp ) {
        if ( my_info->sharesPorts() ) {
            tmp = my_info->parent_info->component->getLinkFromParentSharedPort(name);
            // If I got a link from my parent, I need to put it in my
            // link map
            if ( nullptr != tmp ) {
                if ( nullptr == myLinks ) {
                    myLinks = new LinkMap();
                    my_info->link_map = myLinks;
                }
                myLinks->insertLink(name,tmp);
                // Need to set the link's defaultTimeBase to nullptr,
                // except in the case of this being an Anonymously
                // loadeed SubComponent, then for backward
                // compatibility, we leave it as is.
                if ( !my_info->isLegacySubComponent() ) {
                    tmp->setDefaultTimeBase(nullptr);
                }
            }
        }
    }

    // If I got a link, configure it
    if ( nullptr != tmp ) {
        
        // If no functor, this is a polling link
        if ( handler == nullptr ) {
            tmp->setPolling();
        }
        tmp->setFunctor(handler);
        if ( nullptr != time_base ) tmp->setDefaultTimeBase(time_base);
        else tmp->setDefaultTimeBase(my_info->defaultTimeBase);
        tmp->setAsConfigured();
#ifdef __SST_DEBUG_EVENT_TRACKING__
        tmp->setSendingComponentInfo(my_info->getName(), my_info->getType(), name);
#endif        
    }
    return tmp;
}

Link*
BaseComponent::configureLink(const std::string& name, const std::string& time_base, Event::HandlerBase* handler)
{
    return configureLink(name,getSimulation()->getTimeLord()->getTimeConverter(time_base),handler);
}

Link*
BaseComponent::configureLink(const std::string& name, Event::HandlerBase* handler)
{
    return configureLink(name,nullptr,handler);
}

void
BaseComponent::addSelfLink(const std::string& name)
{
    LinkMap* myLinks = my_info->getLinkMap();
    myLinks->addSelfPort(name);
    if ( myLinks->getLink(name) != nullptr ) {
        printf("Attempting to add self link with duplicate name: %s\n",name.c_str());
        abort();
    }

    Link* link = new SelfLink();
    // Set default time base to the component time base
    link->setDefaultTimeBase(my_info->defaultTimeBase);
    myLinks->insertLink(name,link);

}

Link*
BaseComponent::configureSelfLink( const std::string& name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,time_base,handler);
}

Link*
BaseComponent::configureSelfLink( const std::string& name,  const std::string& time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,time_base,handler);
}

Link*
BaseComponent::configureSelfLink( const std::string& name, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,handler);
}

SimTime_t BaseComponent::getCurrentSimTime(TimeConverter *tc) const {
    return tc->convertFromCoreTime(getSimulation()->getCurrentSimCycle());
}

SimTime_t BaseComponent::getCurrentSimTime(const std::string& base) {
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

bool BaseComponent::doesComponentInfoStatisticExist(const std::string& statisticName) const
{
    const std::string& type = my_info->getType();
    return Factory::getFactory()->DoesComponentInfoStatisticNameExist(type, statisticName);
}


Module*
BaseComponent::loadModule(const std::string& type, Params& params)
{
    return Factory::getFactory()->CreateModule(type,params);
}

Module*
BaseComponent::loadModuleWithComponent(const std::string& type, Component* comp, Params& params)
{
    return Factory::getFactory()->CreateModuleWithComponent(type,comp,params);
}

/* Old ELI style */
SubComponent*
BaseComponent::loadSubComponent(const std::string& type, Component* comp, Params& params)
{
    // /* Old Style SubComponents end up with their parent's Id, name, etc. */
    // ComponentInfo *sub_info = new ComponentInfo(type, &params, my_info);

    // /* By "magic", the new component will steal ownership of this pointer */
    // currentlyLoadingSubComponent = sub_info;
    ComponentId_t cid = comp->currentlyLoadingSubComponentID;
    comp->currentlyLoadingSubComponentID = my_info->addAnonymousSubComponent(my_info, type, "LEGACY", 0,
          ComponentInfo::SHARE_PORTS | ComponentInfo::SHARE_STATS | ComponentInfo::INSERT_STATS | ComponentInfo::IS_LEGACY_SUBCOMPONENT);
    
    SubComponent* ret = Factory::getFactory()->CreateSubComponent(type,comp,params);
    comp->currentlyLoadingSubComponentID = cid;

    return ret;
}

SubComponent*
BaseComponent::loadLegacySubComponentPrivate(ComponentId_t cid, const std::string& type, Params& params) {
    Component* comp = getTrueComponentPrivate();
    ComponentId_t old_cid = comp->currentlyLoadingSubComponentID;
    comp->currentlyLoadingSubComponentID = cid;
    
    SubComponent* ret = Factory::getFactory()->CreateSubComponent(type,comp,params);
    comp->currentlyLoadingSubComponentID = old_cid;
    return ret;
}

Component*
BaseComponent::getTrueComponent() const {
    // Walk up the parent tree until we hit the base Component.  We
    // know we're the base Component when parent is nullptr.
    ComponentInfo* info = my_info;
    while ( info->parent_info != nullptr ) info = info->parent_info;
    return static_cast<Component* const>(info->component);
}

Component*
BaseComponent::getTrueComponentPrivate() const {
    // Walk up the parent tree until we hit the base Component.  We
    // know we're the base Component when parent is nullptr.
    ComponentInfo* info = my_info;
    while ( info->parent_info != nullptr ) info = info->parent_info;
    return static_cast<Component* const>(info->component);
}

/* New ELI style */
SubComponent*
BaseComponent::loadNamedSubComponent(const std::string& name) {
    Params empty;
    return loadNamedSubComponent(name, empty);
}

SubComponent*
BaseComponent::loadNamedSubComponent(const std::string& name, Params& params) {
    // Get list of ComponentInfo objects and make sure that there is
    // only one SubComponent put into this slot
    const std::map<ComponentId_t,ComponentInfo>& subcomps = my_info->getSubComponents();
    int sub_count = 0;
    for ( auto &ci : subcomps ) {
        if ( ci.second.getSlotName() == name ) {
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
BaseComponent::loadNamedSubComponent(const std::string& name, int slot_num) {
    Params empty;
    return loadNamedSubComponent(name, slot_num, empty);
}

// Private
SubComponent*
BaseComponent::loadNamedSubComponent(const std::string& name, int slot_num, Params& params)
{
    if ( !Factory::getFactory()->DoesSubComponentSlotExist(my_info->type, name) ) {
        SST::Output outXX("SubComponentSlotWarning: ", 0, 0, Output::STDERR);
        outXX.output(CALL_INFO, "Warning: SubComponentSlot \"%s\" is undocumented.\n", name.c_str());
    }

    ComponentInfo* sub_info = my_info->findSubComponent(name,slot_num);
    if ( sub_info == nullptr ) return nullptr;
    sub_info->share_flags = ComponentInfo::SHARE_NONE;
    sub_info->parent_info = my_info;
    
    // ComponentInfo *oldLoadingSubcomponent = getTrueComponentPrivate()->currentlyLoadingSubComponent;
    // // ComponentInfo *sub_info = &(infoItr->second);
    // getTrueComponentPrivate()->currentlyLoadingSubComponent = sub_info;

    ComponentId_t cid = getTrueComponentPrivate()->currentlyLoadingSubComponentID;
    getTrueComponentPrivate()->currentlyLoadingSubComponentID = sub_info->id;
        
    Params myParams;
    if ( sub_info->getParams() != nullptr )
        myParams.insert(*sub_info->getParams());
    myParams.insert(params);

    SubComponent* ret = Factory::getFactory()->CreateSubComponent(sub_info->getType(), getTrueComponentPrivate(), myParams);
    sub_info->setComponent(ret);

    getTrueComponentPrivate()->currentlyLoadingSubComponentID = cid;
    return ret;
}

SubComponent*
BaseComponent::loadNamedSubComponentLegacyPrivate(ComponentInfo* sub_info, Params& params)
{
    ComponentId_t cid = getTrueComponentPrivate()->currentlyLoadingSubComponentID;
    getTrueComponentPrivate()->currentlyLoadingSubComponentID = sub_info->id;
    
    Params myParams;
    if ( sub_info->getParams() != nullptr )
        myParams.insert(*sub_info->getParams());
    myParams.insert(params);
    
    SubComponent* ret = Factory::getFactory()->CreateSubComponent(sub_info->getType(), getTrueComponentPrivate(), myParams);
    sub_info->setComponent(ret);
    
    getTrueComponentPrivate()->currentlyLoadingSubComponentID = cid;
    return ret;
}

SubComponentSlotInfo*
BaseComponent::getSubComponentSlotInfo(const std::string& name, bool fatalOnEmptyIndex) {
    SubComponentSlotInfo* info = new SubComponentSlotInfo(this, name);
    if ( info->getMaxPopulatedSlotNumber() < 0 ) {
        // Nothing registered on this slot
        delete info;
        return nullptr;
    }
    if ( !info->isAllPopulated() && fatalOnEmptyIndex ) {
        Simulation::getSimulationOutput().
            fatal(CALL_INFO,1,
                  "SubComponent slot %s requires a dense allocation of SubComponents and did not get one.\n",
                  name.c_str());
    }
    return info;
}

bool
BaseComponent::doesSubComponentExist(const std::string& type)
{
    return Factory::getFactory()->doesSubComponentExist(type);
}

SharedRegion* BaseComponent::getLocalSharedRegion(const std::string& key, size_t size)
{
    SharedRegionManager *mgr = Simulation::getSharedRegionManager();
    return mgr->getLocalSharedRegion(key, size);
}


SharedRegion* BaseComponent::getGlobalSharedRegion(const std::string& key, size_t size, SharedRegionMerger *merger)
{
    SharedRegionManager *mgr = Simulation::getSharedRegionManager();
    return mgr->getGlobalSharedRegion(key, size, merger);
}



uint8_t BaseComponent::getComponentInfoStatisticEnableLevel(const std::string& statisticName) const
{
    return Factory::getFactory()->GetComponentInfoStatisticEnableLevel(my_info->type, statisticName);
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
    StatisticBase*                   statistic = nullptr;


    // First check to see if this is an "inserted" statistic that has
    // already been created.

    // Don't need to look at my cache, that was already done.  Start
    // with my parent.
    ComponentInfo* curr_info = my_info;
    while (curr_info->canInsertStatistics() ) {
        // Check to see if this was already created, and
        // if so return the cached copy
        StatisticBase* prevStat = StatisticProcessingEngine::getInstance()->isStatisticRegisteredWithEngine(
            curr_info->parent_info->getName(), curr_info->parent_info->getID(), statName, statSubId, fieldType);
        if (nullptr != prevStat) {
            // Dynamic cast the base stat to the expected type
            return prevStat;
        }
        curr_info = curr_info->parent_info;
    }


    // Build a name to report errors against
    fullStatName = StatisticBase::buildStatisticFullName(getName().c_str(), statName, statSubId);

    // Make sure that the wireup has not been completed
    if (true == getSimulation()->isWireUpFinished()) {
        // We cannot register statistics AFTER the wireup (after all components have been created) 
        out.fatal(CALL_INFO, 1, "ERROR: Statistic %s - Cannot be registered after the Components have been wired up.  Statistics must be registered on Component creation.; exiting...\n", fullStatName.c_str());
    }


    // Need to check my enabled statistics and if it's not there, then
    // I need to check up my parent tree as long as insertStatistics
    // is enabled.
    curr_info = my_info;
    ComponentInfo* next_info = my_info;
    // uint8_t stat_load_level;
    do {
        curr_info = next_info;
        // Check each entry in the StatEnableList (from the ConfigGraph via the 
        // Python File) to see if this Statistic is enabled, then check any of 
        // its critical parameters

        // Only check for stat enables if I'm a not a component
        // defined SubComponet, because component defined
        // SubComponents don't have a stat enable list.
        if ( !curr_info->isAnonymous() ) {
            for ( auto & si : *curr_info->getStatEnableList() ) {
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
                    // Get the load level from the component
                    // stat_load_level = curr_info->component->getComponentInfoStatisticEnableLevel(si.name);
                    nameFound = true;
                    break;
                }
            }
        }
        next_info = curr_info->parent_info;
    } while ( curr_info->canInsertStatistics() );

    
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
        statistic = create(statTypeParam, curr_info->component, statName, statSubId, statParams);
        if (nullptr == statistic) {
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
        if (nullptr != statistic) {
            delete statistic;
        }

        // Instantiate the Statistic here defined by the type here
        statTypeParam = "sst.NullStatistic";
        statistic = create(statTypeParam, curr_info->component, statName, statSubId, statParams);
        if (nullptr == statistic) {
            statGood = false;
            out.fatal(CALL_INFO, 1, "ERROR: Unable to instantiate Null Statistic %s; exiting...\n", fullStatName.c_str());
        }
        engine->registerStatisticWithEngine(statistic, fieldType);
    }

    // Register the new Statistic with the Statistic Engine
    return statistic;
}

} // namespace SST


