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
#include "sst/core/subcomponent.h"
#include "sst/core/unitAlgebra.h"

#include <boost/foreach.hpp>
#include <string>

//#include "sst/core/event.h"
#include "sst/core/factory.h"
#include "sst/core/link.h"
#include "sst/core/linkMap.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/unitAlgebra.h"

using namespace SST::Statistics;

namespace SST {

SubComponent::SubComponent(Component* parent) :
    parent(parent)
{
    type = parent->currentlyLoadingSubModule;
}

    SubComponent::SubComponent() : 
parent(NULL)
{
}

SubComponent::~SubComponent() 
{
}

Link*
SubComponent::configureLink(std::string name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    return parent->configureLink(name,time_base,handler);
}

Link*
SubComponent::configureLink(std::string name, std::string time_base, Event::HandlerBase* handler)
{
    return configureLink(name,Simulation::getSimulation()->getTimeLord()->getTimeConverter(time_base),handler);
}

Link*
SubComponent::configureLink(std::string name, Event::HandlerBase* handler)
{
    return parent->configureLink(name,handler);
}

Link*
SubComponent::configureSelfLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    return parent->configureSelfLink(name,time_base,handler);
}

Link*
SubComponent::configureSelfLink( std::string name, std::string time_base, Event::HandlerBase* handler)
{
    return parent->configureSelfLink(name,time_base,handler);
}

Link*
SubComponent::configureSelfLink( std::string name, Event::HandlerBase* handler)
{
    return parent->configureSelfLink(name,handler);
}

bool
SubComponent::doesSubComponentInfoStatisticExist(std::string statisticName)
{
    return Simulation::getSimulation()->getFactory()->DoesSubComponentInfoStatisticNameExist(type, statisticName);
}


TimeConverter* SubComponent::registerClock( std::string freq, Clock::HandlerBase* handler) {
    return parent->registerClock(freq,handler,false);
}

TimeConverter* SubComponent::registerClock( const UnitAlgebra& freq, Clock::HandlerBase* handler) {
    return parent->registerClock(freq,handler,false);
}

void SubComponent::unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler) {
    parent->unregisterClock(tc,handler);
}

Cycle_t SubComponent::reregisterClock( TimeConverter* freq, Clock::HandlerBase* handler) {
    return parent->reregisterClock(freq,handler);
}

Cycle_t SubComponent::getNextClockCycle( TimeConverter* freq ) {
    return Simulation::getSimulation()->getNextClockCycle(freq);
}

//For now, no OneShot support in SubSubComponent
#if 0
TimeConverter* SubComponent::registerOneShot( std::string timeDelay, OneShot::HandlerBase* handler) {
    return Simulation::getSimulation()->registerOneShot(timeDelay, handler);
}

TimeConverter* SubComponent::registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler) {
    return Simulation::getSimulation()->registerOneShot(timeDelay, handler);
}
#endif
    

TimeConverter*
SubComponent::getTimeConverter( const std::string& base )
{
    return Simulation::getSimulation()->getTimeLord()->getTimeConverter(base);
}
    
TimeConverter*
SubComponent::getTimeConverter( const UnitAlgebra& base )
{
    return Simulation::getSimulation()->getTimeLord()->getTimeConverter(base);
}

    
SimTime_t SubComponent::getCurrentSimTime(TimeConverter *tc) const {
    return tc->convertFromCoreTime(Simulation::getSimulation()->getCurrentSimCycle());
}

SimTime_t SubComponent::getCurrentSimTime(std::string base) {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getTimeConverter(base));

}
    
SimTime_t SubComponent::getCurrentSimTimeNano() const {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getNano());
}

SimTime_t SubComponent::getCurrentSimTimeMicro() const {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getMicro());
}

SimTime_t SubComponent::getCurrentSimTimeMilli() const {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getMilli());
}

} // namespace SST
