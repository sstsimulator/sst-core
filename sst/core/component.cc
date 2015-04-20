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
#include "sst/core/serialization.h"
#include "sst/core/component.h"
#include "sst/core/unitAlgebra.h"

#include <boost/foreach.hpp>
#include <string>

#include "sst/core/debug.h"
//#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/factory.h"
#include "sst/core/link.h"
#include "sst/core/linkMap.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/unitAlgebra.h"

using namespace SST::Statistics;

namespace SST {

Component::Component(ComponentId_t id) :
    defaultTimeBase(NULL), id( id)
{
    myLinks = Simulation::getSimulation()->getComponentLinkMap(id);
	const CompInfoMap_t& map = Simulation::getSimulation()->getComponentInfoMap();
	CompInfoMap_t::const_iterator i = map.find(id);
	if ( i != map.end() ) {
        name = i->second.name;
        type = i->second.type;
    }
    _COMP_DBG( "new\n" );
    currentlyLoadingSubModule = "";
}

Component::Component() 
{
}

Component::~Component() 
{
    // Simulation::getSimulation()->removeComponentLinkMap(id);
    delete myLinks;
}

TimeConverter* Component::registerClock( std::string freq, Clock::HandlerBase* handler, bool regAll) {
    TimeConverter* tc = Simulation::getSimulation()->registerClock(freq,handler);
    
    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
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
    TimeConverter* tc = Simulation::getSimulation()->registerClock(freq,handler);
    
    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
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
    return Simulation::getSimulation()->reregisterClock(freq,handler);
}

Cycle_t Component::getNextClockCycle( TimeConverter* freq ) {
    return Simulation::getSimulation()->getNextClockCycle(freq);
}

void Component::unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler) {
    Simulation::getSimulation()->unregisterClock(tc,handler);
}

TimeConverter* Component::registerOneShot( std::string timeDelay, OneShot::HandlerBase* handler) {
    return Simulation::getSimulation()->registerOneShot(timeDelay, handler);
}

TimeConverter* Component::registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler) {
    return Simulation::getSimulation()->registerOneShot(timeDelay, handler);
}

TimeConverter* Component::registerTimeBase( std::string base, bool regAll) {
    TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(base);
    
    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
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
    return Simulation::getSimulation()->getTimeLord()->getTimeConverter(base);
}
    
TimeConverter*
Component::getTimeConverter( const UnitAlgebra& base )
{
    return Simulation::getSimulation()->getTimeLord()->getTimeConverter(base);
}

    
bool
Component::isPortConnected(const std::string &name) const
{
    return (myLinks->getLink(name) != NULL);
}

Link*
Component::configureLink(std::string name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    Link* tmp = myLinks->getLink(name);
    if ( tmp == NULL ) return NULL;
    
    // If no functor, this is a polling link
    if ( handler == NULL ) {
        tmp->setPolling();
    }
    tmp->setFunctor(handler);
    tmp->setDefaultTimeBase(time_base);
    return tmp;    
}

Link*
Component::configureLink(std::string name, std::string time_base, Event::HandlerBase* handler)
{
    return configureLink(name,Simulation::getSimulation()->getTimeLord()->getTimeConverter(time_base),handler);
}

Link*
Component::configureLink(std::string name, Event::HandlerBase* handler)
{
    Link* tmp = myLinks->getLink(name);
    if ( tmp == NULL ) return NULL;
    
    // If no functor, this is a polling link
    if ( handler == NULL ) {
        tmp->setPolling();
    }
    tmp->setFunctor(handler);
    return tmp;    
}

void
Component::addSelfLink(std::string name)
{
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
    return tc->convertFromCoreTime(Simulation::getSimulation()->getCurrentSimCycle());
}

SimTime_t Component::getCurrentSimTime(std::string base) {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getTimeConverter(base));

}
    
SimTime_t Component::getCurrentSimTimeNano() const {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getNano());
}

SimTime_t Component::getCurrentSimTimeMicro() const {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getMicro());
}

SimTime_t Component::getCurrentSimTimeMilli() const {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getMilli());
}

bool Component::registerExit()
{
    return Simulation::getSimulation()->getExit()->refInc( getId() ); 
}

bool Component::unregisterExit()
{
    return Simulation::getSimulation()->getExit()->refDec( getId() ); 
}

void
Component::registerAsPrimaryComponent()
{
    // Nop for now.  Will put in complete semantics later
}

void
Component::primaryComponentDoNotEndSim()
{
    Simulation::getSimulation()->getExit()->refInc( getId() ); 
}
    
void
Component::primaryComponentOKToEndSim()
{
    Simulation::getSimulation()->getExit()->refDec( getId() ); 
}

Module*
Component::loadModule(std::string type, Params& params)
{
    return Simulation::getSimulation()->getFactory()->CreateModule(type,params);
}

Module*
Component::loadModuleWithComponent(std::string type, Component* comp, Params& params)
{
    return Simulation::getSimulation()->getFactory()->CreateModuleWithComponent(type,comp,params);
}

SubComponent*
Component::loadSubComponent(std::string type, Component* comp, Params& params)
{
    currentlyLoadingSubModule = type;
    SubComponent* ret = Simulation::getSimulation()->getFactory()->CreateSubComponent(type,comp,params);
    currentlyLoadingSubModule = "";
    return ret;
}
    
bool Component::doesComponentInfoStatisticExist(std::string statisticName)
{
    return Simulation::getSimulation()->getFactory()->DoesComponentInfoStatisticNameExist(type, statisticName);
}

uint8_t Component::getComponentInfoStatisticEnableLevel(std::string statisticName)
{
    return Simulation::getSimulation()->getFactory()->GetComponentInfoStatisticEnableLevel(type, statisticName);
}

std::string Component::getComponentInfoStatisticUnits(std::string statisticName)
{
    return Simulation::getSimulation()->getFactory()->GetComponentInfoStatisticUnits(type, statisticName);
}

template<class Archive>
void
Component::serialize(Archive& ar, const unsigned int version) {
    ar & BOOST_SERIALIZATION_NVP(type);
    ar & BOOST_SERIALIZATION_NVP(id);
    ar & BOOST_SERIALIZATION_NVP(name);
    ar & BOOST_SERIALIZATION_NVP(defaultTimeBase);
    ar & BOOST_SERIALIZATION_NVP(myLinks);
}
    
} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Component::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Component)
