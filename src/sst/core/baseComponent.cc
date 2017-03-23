// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

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

using namespace SST::Statistics;

namespace SST {

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
    TimeConverter* tc = getSimulation()->registerClock(freq,handler);

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
    TimeConverter* tc = getSimulation()->registerClock(freq,handler);

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
    return getSimulation()->reregisterClock(freq,handler);
}

Cycle_t BaseComponent::getNextClockCycle( TimeConverter* freq ) {
    return getSimulation()->getNextClockCycle(freq);
}

void BaseComponent::unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler) {
    getSimulation()->unregisterClock(tc,handler);
}

TimeConverter* BaseComponent::registerOneShot( std::string timeDelay, OneShot::HandlerBase* handler) {
    return getSimulation()->registerOneShot(timeDelay, handler);
}

TimeConverter* BaseComponent::registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler) {
    return getSimulation()->registerOneShot(timeDelay, handler);
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
    tmp->setDefaultTimeBase(time_base);
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
    LinkMap* myLinks = my_info->getLinkMap();
    Link* tmp = myLinks->getLink(name);
    if ( tmp == NULL ) return NULL;

    // If no functor, this is a polling link
    if ( handler == NULL ) {
        tmp->setPolling();
    }
    tmp->setFunctor(handler);
#ifdef __SST_DEBUG_EVENT_TRACKING__
    tmp->setSendingComponentInfo(my_info->getName(), my_info->getType(), name);
#endif
    return tmp;
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

Link* BaseComponent::selfLink( std::string name __attribute__((unused)), Event::HandlerBase* handler )
{
    Link* link = new SelfLink();
    link->setLatency(0);
    link->setFunctor(handler);
    if ( handler == NULL ) {
        link->setPolling();
    }
    return link;
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
    ComponentInfo *sub_info = new ComponentInfo(type, &params, comp->my_info);
    ComponentInfo *oldLoadingSubCopmonent = getTrueComponent()->currentlyLoadingSubComponent;
    /* By "magic", the new component will steal ownership of this pointer */
    getTrueComponent()->currentlyLoadingSubComponent = sub_info;

    SubComponent* ret = Factory::getFactory()->CreateSubComponent(type,comp,params);
    sub_info->setComponent(ret);
    getTrueComponent()->currentlyLoadingSubComponent = oldLoadingSubCopmonent;
    return ret;
}

/* New ELI style */
SubComponent*
BaseComponent::loadNamedSubComponent(std::string name) {
    Params empty;
    return loadNamedSubComponent(name, empty);
}

SubComponent*
BaseComponent::loadNamedSubComponent(std::string name, Params& params)
{
    auto infoItr = my_info->getSubComponents().find(name);
    if ( infoItr == my_info->getSubComponents().end() ) return NULL;

    ComponentInfo *oldLoadingSubCopmonent = getTrueComponent()->currentlyLoadingSubComponent;
    ComponentInfo *sub_info = &(infoItr->second);
    getTrueComponent()->currentlyLoadingSubComponent = sub_info;

    Params myParams;
    if ( sub_info->getParams() != NULL )
        myParams.insert(*sub_info->getParams());
    myParams.insert(params);

    SubComponent* ret = Factory::getFactory()->CreateSubComponent(sub_info->getType(), getTrueComponent(), myParams);
    sub_info->setComponent(ret);

    getTrueComponent()->currentlyLoadingSubComponent = oldLoadingSubCopmonent;
    return ret;
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


} // namespace SST


