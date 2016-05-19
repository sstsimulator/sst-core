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

#include <sst_config.h>
#include "sst/core/serialization.h"
#include "sst/core/component.h"
#include "sst/core/unitAlgebra.h"

#include <boost/foreach.hpp>
#include <string>

//#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/factory.h"
#include "sst/core/link.h"
#include "sst/core/linkMap.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/sharedRegion.h"

using namespace SST::Statistics;

namespace SST {

Component::Component(ComponentId_t id) :
    defaultTimeBase(NULL), id(id)//, my_info(Simulation::getSimulation()->getComponentInfoMap()[id])
{
    sim = Simulation::getSimulation();
    my_info = sim->getComponentInfo(id);
    currentlyLoadingSubComponent = "";
}

Component::Component()
{
}

Component::~Component() 
{
}

bool checkPort(const char *def, const char *offered)
{
    const char * x = def;
    const char * y = offered;
    
    /* Special case.  Name of '*' matches everything */
    if ( *x == '*' && *(x+1) == '\0' ) return true;
    
    do {
        if ( *x == '%' && (*(x+1) == '(' || *(x+1) == 'd') ) {
            // We have a %d or %(var)d to eat
            x++;
            if ( *x == '(' ) {
                while ( *x && (*x != ')') ) x++;
                x++;  /* *x should now == 'd' */
            }
            if ( *x != 'd') /* Malformed string.  Fail all the things */
                return false;
            x++; /* Finish eating the variable */
            /* Now, eat the corresponding digits of y */
            while ( *y && isdigit(*y) ) y++;
        }
        if ( *x != *y ) return false;
        if ( *x == '\0' ) return true;
        x++;
        y++;
    } while ( *x && *y );
    if ( *x != *y ) return false; // aka, both NULL
    return true;
}
    
    
bool
Component::isPortValidForComponent(const std::string& comp_name, const std::string& port_name) {
    const std::vector<std::string>* allowedPorts = Factory::getFactory()->GetComponentAllowedPorts(comp_name);

    const char *x = port_name.c_str();
    bool found = false;
    if ( NULL != allowedPorts ) {
        for ( std::vector<std::string>::const_iterator i = allowedPorts->begin() ; i != allowedPorts->end() ; ++i ) {
            /* Compare name with stored name, which may have wildcards */
            if ( checkPort(i->c_str(), x) ) {
                found = true;
                break;
            }
        }
    }
        
    return found;
    
}


TimeConverter* Component::registerClock( std::string freq, Clock::HandlerBase* handler, bool regAll) {
    TimeConverter* tc = getSimulation()->registerClock(freq,handler);
    
    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        LinkMap* myLinks = my_info->getLinkMap();
        if (NULL != myLinks) {
            std::pair<std::string,Link*> p;
            BOOST_FOREACH( p, myLinks->getLinkMap() ) {
                if ( NULL == p.second->getDefaultTimeBase() ) {
                    p.second->setDefaultTimeBase(tc);
                }
            }
        }
	defaultTimeBase = tc;
    }
    return tc;
}

TimeConverter* Component::registerClock( const UnitAlgebra& freq, Clock::HandlerBase* handler, bool regAll) {
    TimeConverter* tc = getSimulation()->registerClock(freq,handler);
    
    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        LinkMap* myLinks = my_info->getLinkMap();
        if (NULL != myLinks) {
            std::pair<std::string,Link*> p;
            BOOST_FOREACH( p, myLinks->getLinkMap() ) {
                if ( NULL == p.second->getDefaultTimeBase() ) {
                    p.second->setDefaultTimeBase(tc);
                }
            }
        }
	defaultTimeBase = tc;
    }
    return tc;
}

Cycle_t Component::reregisterClock( TimeConverter* freq, Clock::HandlerBase* handler) {
    return getSimulation()->reregisterClock(freq,handler);
}

Cycle_t Component::getNextClockCycle( TimeConverter* freq ) {
    return getSimulation()->getNextClockCycle(freq);
}

void Component::unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler) {
    getSimulation()->unregisterClock(tc,handler);
}

TimeConverter* Component::registerOneShot( std::string timeDelay, OneShot::HandlerBase* handler) {
    return getSimulation()->registerOneShot(timeDelay, handler);
}

TimeConverter* Component::registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler) {
    return getSimulation()->registerOneShot(timeDelay, handler);
}

TimeConverter* Component::registerTimeBase( std::string base, bool regAll) {
    TimeConverter* tc = getSimulation()->getTimeLord()->getTimeConverter(base);
    
    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        LinkMap* myLinks = my_info->getLinkMap();
        if (NULL != myLinks) {
            std::pair<std::string,Link*> p;
            BOOST_FOREACH( p, myLinks->getLinkMap() ) {
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
Component::getTimeConverter( const std::string& base )
{
    return getSimulation()->getTimeLord()->getTimeConverter(base);
}
    
TimeConverter*
Component::getTimeConverter( const UnitAlgebra& base )
{
    return getSimulation()->getTimeLord()->getTimeConverter(base);
}

    
bool
Component::isPortConnected(const std::string &name) const
{
    return (my_info->getLinkMap()->getLink(name) != NULL);
}

Link*
Component::configureLink(std::string name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    LinkMap* myLinks = my_info->getLinkMap();
    const std::string& type = my_info->getType();
    if ( !isPortValidForComponent(type,name) && !myLinks->isSelfPort(name) ) {
#ifdef USE_PARAM_WARNINGS
            std::cerr << "Warning:  Using undocumented port '" << name << "'." << std::endl;
#endif
    }
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
Component::configureLink(std::string name, std::string time_base, Event::HandlerBase* handler)
{
    return configureLink(name,getSimulation()->getTimeLord()->getTimeConverter(time_base),handler);
}

Link*
Component::configureLink(std::string name, Event::HandlerBase* handler)
{
    LinkMap* myLinks = my_info->getLinkMap();
    const std::string& type = my_info->getType();
    if ( !isPortValidForComponent(type,name) && !myLinks->isSelfPort(name) ) {
#ifdef USE_PARAM_WARNINGS
            std::cerr << "Warning:  Using undocumented port '" << name << "'." << std::endl;
#endif
    }
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
Component::addSelfLink(std::string name)
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
Component::configureSelfLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,time_base,handler);
}

Link*
Component::configureSelfLink( std::string name, std::string time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,time_base,handler);
}

Link*
Component::configureSelfLink( std::string name, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name,handler);
}

Link* Component::selfLink( std::string name, Event::HandlerBase* handler )
{
//     Link* link = new Link(handler);
//     link->Connect(link,0);
//     return link;
    Link* link = new SelfLink();
    link->setLatency(0);
    link->setFunctor(handler);
    if ( handler == NULL ) {
	link->setPolling();
    }
    return link;
}
    
SimTime_t Component::getCurrentSimTime(TimeConverter *tc) const {
    return tc->convertFromCoreTime(getSimulation()->getCurrentSimCycle());
}

SimTime_t Component::getCurrentSimTime(std::string base) {
    return getCurrentSimTime(getSimulation()->getTimeLord()->getTimeConverter(base));

}
    
SimTime_t Component::getCurrentSimTimeNano() const {
    return getCurrentSimTime(getSimulation()->getTimeLord()->getNano());
}

SimTime_t Component::getCurrentSimTimeMicro() const {
    return getCurrentSimTime(getSimulation()->getTimeLord()->getMicro());
}

SimTime_t Component::getCurrentSimTimeMilli() const {
    return getCurrentSimTime(getSimulation()->getTimeLord()->getMilli());
}

bool Component::registerExit()
{
    int thread = getSimulation()->getRank().thread;
    return getSimulation()->getExit()->refInc( getId(), thread ); 
}

bool Component::unregisterExit()
{
    int thread = getSimulation()->getRank().thread;
    return getSimulation()->getExit()->refDec( getId(), thread ); 
}

void
Component::registerAsPrimaryComponent()
{
    // Nop for now.  Will put in complete semantics later
}

void
Component::primaryComponentDoNotEndSim()
{
    int thread = getSimulation()->getRank().thread;
    getSimulation()->getExit()->refInc( getId(), thread );
}
    
void
Component::primaryComponentOKToEndSim()
{
    int thread = getSimulation()->getRank().thread;
    getSimulation()->getExit()->refDec( getId(), thread ); 
}

Module*
Component::loadModule(std::string type, Params& params)
{
    return Factory::getFactory()->CreateModule(type,params);
}

Module*
Component::loadModuleWithComponent(std::string type, Component* comp, Params& params)
{
    return Factory::getFactory()->CreateModuleWithComponent(type,comp,params);
}

SubComponent*
Component::loadSubComponent(std::string type, Component* comp, Params& params)
{
    std::string oldLoadingSubCopmonent = currentlyLoadingSubComponent;
    currentlyLoadingSubComponent = type;
    SubComponent* ret = Factory::getFactory()->CreateSubComponent(type,comp,params);
    currentlyLoadingSubComponent = oldLoadingSubCopmonent;
    return ret;
}
    
bool Component::doesComponentInfoStatisticExist(std::string statisticName)
{
    const std::string& type = my_info->getType();
    return Factory::getFactory()->DoesComponentInfoStatisticNameExist(type, statisticName);
}

uint8_t Component::getComponentInfoStatisticEnableLevel(std::string statisticName)
{
    const std::string& type = my_info->getType();
    return Factory::getFactory()->GetComponentInfoStatisticEnableLevel(type, statisticName);
}

std::string Component::getComponentInfoStatisticUnits(std::string statisticName)
{
    const std::string& type = my_info->getType();
    return Factory::getFactory()->GetComponentInfoStatisticUnits(type, statisticName);
}



SharedRegion* Component::getLocalSharedRegion(const std::string &key, size_t size)
{
    SharedRegionManager *mgr = Simulation::getSharedRegionManager();
    return mgr->getLocalSharedRegion(key, size);
}


SharedRegion* Component::getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger)
{
    SharedRegionManager *mgr = Simulation::getSharedRegionManager();
    return mgr->getGlobalSharedRegion(key, size, merger);
}



template<class Archive>
void
Component::serialize(Archive& ar, const unsigned int version) {
    // ar & BOOST_SERIALIZATION_NVP(type);
    // ar & BOOST_SERIALIZATION_NVP(id);
    // ar & BOOST_SERIALIZATION_NVP(name);
    ar & BOOST_SERIALIZATION_NVP(defaultTimeBase);
    // ar & BOOST_SERIALIZATION_NVP(myLinks);
}
    
} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Component::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Component)
