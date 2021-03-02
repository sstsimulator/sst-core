// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "sst/core/configGraph.h"
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

#include "sst/core/statapi/statoutput.h"

namespace SST {


BaseComponent::BaseComponent(ComponentId_t id) :
    sim(Simulation::getSimulation()),
    my_info(Simulation::getSimulation()->getComponentInfo(id)),
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

TimeConverter* BaseComponent::registerClock( TimeConverter* tc, Clock::HandlerBase* handler, bool regAll) {
    TimeConverter* tcRet = getSimulation()->registerClock(tc, handler, CLOCKPRIORITY);

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        setDefaultTimeBaseForLinks(tcRet);
        my_info->defaultTimeBase = tcRet;
    }
    return tcRet;
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
BaseComponent::getTimeConverter( const std::string& base ) const
{
    return getSimulation()->getTimeLord()->getTimeConverter(base);
}

TimeConverter*
BaseComponent::getTimeConverter( const UnitAlgebra& base ) const
{
    return getSimulation()->getTimeLord()->getTimeConverter(base);
}

TimeConverter*
BaseComponent::getTimeConverterNano() const
{
    return Simulation::getSimulation()->getTimeLord()->getNano();
}

TimeConverter*
BaseComponent::getTimeConverterMicro() const
{
    return Simulation::getSimulation()->getTimeLord()->getMicro();
}

TimeConverter*
BaseComponent::getTimeConverterMilli() const
{
    return Simulation::getSimulation()->getTimeLord()->getMilli();
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
                // Need to set the link's defaultTimeBase to nullptr
                tmp->setDefaultTimeBase(nullptr);
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

SimTime_t
BaseComponent::getCurrentSimCycle() const
{
    return Simulation::getSimulation()->getCurrentSimCycle();
}

int
BaseComponent::getCurrentPriority() const
{
    return Simulation::getSimulation()->getCurrentPriority();
}

UnitAlgebra
BaseComponent::getElapsedSimTime() const
{
    return Simulation::getSimulation()->getElapsedSimTime();
}

UnitAlgebra
BaseComponent::getFinalSimTime() const
{
    return Simulation::getSimulation()->getFinalSimTime();
}

RankInfo
BaseComponent::getRank() const
{
    return Simulation::getSimulation()->getRank();
}

RankInfo
BaseComponent::getNumRanks() const
{
    return Simulation::getSimulation()->getNumRanks();
}

Output&
BaseComponent::getSimulationOutput() const
{
    return Simulation::getSimulation()->getSimulationOutput();
}


SimTime_t BaseComponent::getCurrentSimTime(TimeConverter *tc) const {
    return tc->convertFromCoreTime(getSimulation()->getCurrentSimCycle());
}

SimTime_t BaseComponent::getCurrentSimTime(const std::string& base) const {
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

void
BaseComponent::vfatal(uint32_t line, const char* file, const char* func,
                    int exit_code,
                    const char* format, va_list arg)    const
{
    Output abort("Rank: @R,@I, time: @t - called in file: @f, line: @l, function: @p", 5, -1, Output::STDOUT);

    // Get info about the simulation
    std::string name = my_info->getName();
    std::string type = my_info->getType();
    // Build up the full list of types all the way to parent component
    std::string type_tree = my_info->getType();
    ComponentInfo* parent = my_info->parent_info;
    while ( parent != nullptr ) {
        type_tree = parent->type + "/" + type_tree;
        parent = parent->parent_info;
    }

    char new_format[256];

    snprintf(new_format,256,"\nElement name: %s,  type: %s (full type tree: %s)\n%s",
             name.c_str(),type.c_str(),type_tree.c_str(),format);

    char buf[512];
    vsnprintf(buf,512,new_format,arg);
    abort.fatal(line,file,func,exit_code,"%s",buf);
}

void
BaseComponent::fatal(uint32_t line, const char* file, const char* func,
                    int exit_code,
                    const char* format, ...)    const
{
    va_list arg;
    va_start(arg,format);
    vfatal(line,file,func,exit_code,format,arg);
    va_end(arg);
}


void
BaseComponent::sst_assert(bool condition, uint32_t line, const char* file, const char* func,
                          int exit_code,
                          const char* format, ...)    const
{
    if ( !condition ) {
        va_list arg;
        va_start(arg, format);
        vfatal(line,file,func,exit_code,format,arg);
        va_end(arg);
    }
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

void
BaseComponent::configureCollectionMode(Statistics::StatisticBase* statistic, const SST::Params& params,
                                       const std::string& name) {
    StatisticBase::StatMode_t statCollectionMode = StatisticBase::STAT_MODE_COUNT;
    Output& out = getSimulation()->getSimulationOutput();
    std::string statRateParam = params.find<std::string>("rate", "0ns");
    UnitAlgebra collectionRate(statRateParam);

    // make sure we have a valid collection rate
    // Check that the Collection Rate is a valid unit type that we can use
    if (collectionRate.hasUnits("s") || collectionRate.hasUnits("hz")) {
        // Rate is Periodic Based
        statCollectionMode = StatisticBase::STAT_MODE_PERIODIC;
    } else if (collectionRate.hasUnits("event")) {
        // Rate is Count Based
        statCollectionMode = StatisticBase::STAT_MODE_COUNT;
    } else if (collectionRate.getValue() == 0) {
        // Collection rate is zero
        // so we just dump at beginning and end
        collectionRate = UnitAlgebra("0ns");
        statCollectionMode = StatisticBase::STAT_MODE_PERIODIC;
    } else {
        // collectionRate is a unit type we dont recognize
        out.fatal(CALL_INFO, 1, "ERROR: Statistic %s - Collection Rate = %s not valid; exiting...\n", name.c_str(),
                  collectionRate.toString().c_str());
    }

    if (!statistic->isStatModeSupported(statCollectionMode)) {
        if (StatisticBase::STAT_MODE_PERIODIC == statCollectionMode) {
            out.fatal(CALL_INFO, 1,
                      " Warning: Statistic %s Does not support Periodic Based Collections; Collection Rate = %s\n",
                      name.c_str(), collectionRate.toString().c_str());
        } else {
            out.fatal(CALL_INFO, 1,
                      " Warning: Statistic %s Does not support Event Based Collections; Collection Rate = %s\n",
                      name.c_str(), collectionRate.toString().c_str());
        }
    }

    statistic->setRegisteredCollectionMode(statCollectionMode);
}

Statistics::StatisticBase*
BaseComponent::createStatistic(Params& cpp_params, const Params& python_params, const std::string& name,
                               const std::string& subId, StatCreateFunction fxn) {
    auto* engine = Statistics::StatisticProcessingEngine::getInstance();

    uint8_t my_load_level = getStatisticLoadLevel();
    uint8_t stat_load_level
        = my_load_level == STATISTICLOADLEVELUNINITIALIZED ? engine->statLoadLevel() : my_load_level;
    if (stat_load_level == 0) {
        getSimulation()->getSimulationOutput().verbose(
            CALL_INFO, 1, 0,
            " Warning: Statistic Load Level = 0 (all statistics disabled); statistic '%s' is disabled...\n",
            name.c_str());
        return fxn(this, engine, "sst.NullStatistic", name, subId, cpp_params);
    }

    uint8_t stat_enable_level = getComponentInfoStatisticEnableLevel(name);
    if (stat_enable_level > stat_load_level) {
        getSimulation()->getSimulationOutput().verbose(CALL_INFO, 1, 0,
                                                       " Warning: Load Level %d is too low to enable Statistic '%s' "
                                                       "with Enable Level %d, statistic will not be enabled...\n",
                                                       int(stat_load_level), name.c_str(), int(stat_enable_level));
        return fxn(this, engine, "sst.NullStatistic", name, subId, cpp_params);
    }

    // this is enabled
    configureAllowedStatParams(cpp_params);
    cpp_params.insert(python_params);
    std::string type = cpp_params.find<std::string>("type", "sst.AccumulatorStatistic");
    auto* stat = fxn(this, engine, type, name, subId, cpp_params);
    configureCollectionMode(stat, cpp_params, name);
    engine->registerStatisticWithEngine(stat);
    return stat;
}

Statistics::StatisticBase*
BaseComponent::createEnabledAllStatistic(Params& params, const std::string& name, const std::string& statSubId,
                                         StatCreateFunction fxn) {
    auto iter = m_enabledAllStats.find(name);
    if (iter != m_enabledAllStats.end()) {
        auto& submap = iter->second;
        auto subiter = submap.find(statSubId);
        if (subiter != submap.end()) {
            return subiter->second;
        }
    }

    // a matching statistic was not found
    auto* stat = createStatistic(params, my_info->allStatConfig->params, name, statSubId, std::move(fxn));
    m_enabledAllStats[name][statSubId] = stat;
    return stat;
}

Statistics::StatisticBase*
BaseComponent::createExplicitlyEnabledStatistic(Params& params, StatisticId_t id, const std::string& name,
                                                const std::string& statSubId, StatCreateFunction fxn) {
    Output& out = getSimulation()->getSimulationOutput();
    if (my_info->parent_info) {
        out.fatal(CALL_INFO, 1, 0, "Creating explicitly enabled statistic '%s' should only happen in parent component",
                  name.c_str());
    }

    auto piter = my_info->statConfigs->find(id);
    if (piter == my_info->statConfigs->end()) {
        out.fatal(CALL_INFO, 1, 0, "Explicitly enabled statistic '%s' does not have parameters mapped to its ID",
                  name.c_str());
    }
    auto& cfg = piter->second;
    if (cfg.shared) {
        auto iter = m_explicitlyEnabledSharedStats.find(id);
        if (iter != m_explicitlyEnabledSharedStats.end()) {
            return iter->second;
        } else {
            // no subid
            auto* stat = createStatistic(params, cfg.params, cfg.name, "", std::move(fxn));
            m_explicitlyEnabledSharedStats[id] = stat;
            return stat;
        }
    } else {
        auto iter = m_explicitlyEnabledUniqueStats.find(id);
        if (iter != m_explicitlyEnabledUniqueStats.end()) {
            auto& map = iter->second;
            auto subiter = map.find(name);
            if (subiter != map.end()) {
                auto& submap = subiter->second;
                auto subsubiter = submap.find(statSubId);
                if (subsubiter != submap.end()) {
                    return subsubiter->second;
                }
            }
        }
        // stat does not exist yet
        auto* stat = createStatistic(params, cfg.params, name, statSubId, std::move(fxn));
        m_explicitlyEnabledUniqueStats[id][name][statSubId] = stat;
        return stat;
    }

    // unreachable
    return nullptr;
}

void
BaseComponent::configureAllowedStatParams(SST::Params& params) {
    // Identify what keys are Allowed in the parameters
    Params::KeySet_t allowedKeySet;
    allowedKeySet.insert("type");
    allowedKeySet.insert("rate");
    allowedKeySet.insert("startat");
    allowedKeySet.insert("stopat");
    allowedKeySet.insert("resetOnRead");
    params.pushAllowedKeys(allowedKeySet);
}

void
BaseComponent::performStatisticOutput(StatisticBase* stat)
{
    Simulation::getSimulation()->getStatisticsProcessingEngine()->performStatisticOutput(stat);
}

void
BaseComponent::performGlobalStatisticOutput()
{
    Simulation::getSimulation()->getStatisticsProcessingEngine()->performGlobalStatisticOutput(false);
}


} // namespace SST


