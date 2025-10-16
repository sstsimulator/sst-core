// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/baseComponent.h"

#include "sst/core/component.h"
#include "sst/core/configGraph.h"
#include "sst/core/factory.h"
#include "sst/core/link.h"
#include "sst/core/linkMap.h"
#include "sst/core/portModule.h"
#include "sst/core/profile/clockHandlerProfileTool.h"
#include "sst/core/profile/eventHandlerProfileTool.h"
#include "sst/core/serialization/serialize.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/stringize.h"
#include "sst/core/subcomponent.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/warnmacros.h"
#include "sst/core/watchPoint.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace SST {

BaseComponent::BaseComponent(ComponentId_t id) :
    SST::Core::Serialization::serializable_base(),
    my_info_(Simulation_impl::getSimulation()->getComponentInfo(id)),
    sim_(Simulation_impl::getSimulation())
{
    if ( my_info_->component == nullptr ) {
        // If it's already set, then this is a ComponentExtension and
        // we shouldn't reset it.
        my_info_->component = this;
    }

    // Do this once instead of for every stat
    if ( my_info_->enabled_all_stats_ ) {
        if ( my_info_->statLoadLevel == STATISTICLOADLEVELUNINITIALIZED ) {
            my_info_->statLoadLevel = getStatEngine()->getStatLoadLevel();
        }
    }
    else {
        my_info_->statLoadLevel = 0; // All disabled, simplify checks later
    }
}

BaseComponent::~BaseComponent()
{
    // Need to cleanup my ComponentInfo and delete all my children.

    // If my_info_ is nullptr, then we are being deleted by our
    // ComponentInfo object.  This happens at the end of execution
    // when the simulation destructor fires.
    if ( !my_info_ ) return;
    if ( isExtension() ) return;

    // Start by deleting children
    std::map<ComponentId_t, ComponentInfo>& subcomps = my_info_->getSubComponents();
    for ( auto& ci : subcomps ) {
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
    my_info_->component = nullptr;
    if ( my_info_->parent_info ) {
        std::map<ComponentId_t, ComponentInfo>& parent_subcomps = my_info_->parent_info->getSubComponents();
        size_t                                  deleted         = parent_subcomps.erase(my_info_->id_);
        if ( deleted != 1 ) {
            // Should never happen, but issue warning just in case
            sim_->getSimulationOutput().output(
                "Warning:  BaseComponent destructor failed to remove ComponentInfo from parent.\n");
        }
    }

    // Delete all clock handlers.  We need to delete here because
    // handlers are not always registered with the clock object.
    for ( auto* handler : clock_handlers_ ) {
        delete handler;
    }

    // Delete any portModules
    for ( auto port : portModules ) {
        delete port;
    }
}

void
BaseComponent::setDefaultTimeBaseForLinks(TimeConverter tc)
{
    LinkMap* myLinks = my_info_->getLinkMap();
    if ( nullptr != myLinks ) {
        for ( std::pair<std::string, Link*> p : myLinks->getLinkMap() ) {
            if ( nullptr == p.second->getDefaultTimeBase() && p.second->isConfigured() ) {
                p.second->setDefaultTimeBase(tc);
            }
        }
    }
}

void
BaseComponent::pushValidParams(Params& params, const std::string& type)
{
    params.pushAllowedKeys(Factory::getFactory()->getParamNames(type));
}

void
BaseComponent::registerClock_impl(TimeConverter* tc, Clock::HandlerBase* handler, bool regAll)
{
    // Add this clock to our registered_clocks_ set
    registered_clocks_.insert(tc->getFactor());

    // Need to see if I already know about this clock handler
    bool found = false;
    for ( auto* x : clock_handlers_ ) {
        if ( handler == x ) {
            found = true;
            break;
        }
    }
    if ( !found ) clock_handlers_.push_back(handler);

    // Check to see if there is a profile tool installed
    auto tools = sim_->getProfileTool<Profile::ClockHandlerProfileTool>("clock");

    for ( auto* tool : tools ) {
        ClockHandlerMetaData mdata(my_info_->getID(), getName(), getType());
        // Add the receive profiler to the handler
        handler->attachTool(tool, mdata);
    }

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        setDefaultTimeBaseForLinks(tc);
        my_info_->defaultTimeBase = tc;
    }
}


TimeConverter*
BaseComponent::registerClock(const std::string& freq, Clock::HandlerBase* handler, bool regAll)
{
    TimeConverter* tc = sim_->registerClock(freq, handler, CLOCKPRIORITY);
    registerClock_impl(tc, handler, regAll);
    return tc;
}

TimeConverter*
BaseComponent::registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, bool regAll)
{
    TimeConverter* tc = sim_->registerClock(freq, handler, CLOCKPRIORITY);
    registerClock_impl(tc, handler, regAll);
    return tc;
}

TimeConverter*
BaseComponent::registerClock(TimeConverter tc, Clock::HandlerBase* handler, bool regAll)
{
    TimeConverter* tcRet = sim_->registerClock(tc, handler, CLOCKPRIORITY);
    registerClock_impl(tcRet, handler, regAll);
    return tcRet;
}

TimeConverter*
BaseComponent::registerClock(TimeConverter* tc, Clock::HandlerBase* handler, bool regAll)
{
    TimeConverter* tcRet = sim_->registerClock(tc, handler, CLOCKPRIORITY);
    registerClock_impl(tcRet, handler, regAll);
    return tcRet;
}

Cycle_t
BaseComponent::reregisterClock(TimeConverter freq, Clock::HandlerBase* handler)
{
    return sim_->reregisterClock(freq, handler, CLOCKPRIORITY);
}

Cycle_t
BaseComponent::reregisterClock(TimeConverter* freq, Clock::HandlerBase* handler)
{
    return sim_->reregisterClock(freq, handler, CLOCKPRIORITY);
}

Cycle_t
BaseComponent::getNextClockCycle(TimeConverter* freq)
{
    return sim_->getNextClockCycle(freq, CLOCKPRIORITY);
}

Cycle_t
BaseComponent::getNextClockCycle(TimeConverter freq)
{
    return sim_->getNextClockCycle(freq, CLOCKPRIORITY);
}

void
BaseComponent::unregisterClock(TimeConverter* tc, Clock::HandlerBase* handler)
{
    sim_->unregisterClock(tc, handler, CLOCKPRIORITY);
}

void
BaseComponent::unregisterClock(TimeConverter tc, Clock::HandlerBase* handler)
{
    sim_->unregisterClock(tc, handler, CLOCKPRIORITY);
}

TimeConverter*
BaseComponent::registerTimeBase(const std::string& base, bool regAll)
{
    TimeConverter* tc = Simulation_impl::getTimeLord()->getTimeConverter(base);

    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
        setDefaultTimeBaseForLinks(tc);
        my_info_->defaultTimeBase = tc;
    }
    return tc;
}

TimeConverter*
BaseComponent::getTimeConverter(const std::string& base) const
{
    return Simulation_impl::getTimeLord()->getTimeConverter(base);
}

TimeConverter*
BaseComponent::getTimeConverter(const UnitAlgebra& base) const
{
    return Simulation_impl::getTimeLord()->getTimeConverter(base);
}

bool
BaseComponent::isPortConnected(const std::string& name) const
{
    return (my_info_->getLinkMap()->getLink(name) != nullptr);
}

// Looks at parents' shared ports and returns the link connected to
// the port of the correct name in one of my parents. If I find the
// correct link, and it hasn't been configured yet, I return it to the
// child and remove it from my linkmap.  The child will insert it into
// their link map.
Link*
BaseComponent::getLinkFromParentSharedPort(const std::string& port, std::vector<ConfigPortModule>& port_modules)
{
    LinkMap* myLinks = my_info_->getLinkMap();

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
                // Need to see if there are any associated PortModules
                if ( my_info_->portModules != nullptr ) {
                    auto it = my_info_->portModules->find(port);
                    if ( it != my_info_->portModules->end() ) {
                        // Found PortModules, swap them into
                        // port_modules and remove from my map
                        port_modules.swap(it->second);
                        my_info_->portModules->erase(it);
                    }
                }
                return tmp;
            }
        }
    }

    // If we get here, we didn't find the link.  Check to see if my
    // parent shared with me and if so, call
    // getLinkFromParentSharedPort on them

    if ( my_info_->sharesPorts() ) {
        return my_info_->parent_info->component->getLinkFromParentSharedPort(port, port_modules);
    }
    else {
        return nullptr;
    }
}

Link*
BaseComponent::configureLink_impl(const std::string& name, SimTime_t time_base, Event::HandlerBase* handler)
{
    LinkMap* myLinks = my_info_->getLinkMap();

    Link* tmp = nullptr;

    // If I have a linkmap, check to see if a link was connected to
    // port "name"
    if ( nullptr != myLinks ) {
        tmp = myLinks->getLink(name);
    }
    // If tmp is nullptr, then I didn't have the port connected, check
    // with parents if sharing is turned on
    if ( nullptr == tmp ) {
        if ( my_info_->sharesPorts() ) {
            std::vector<ConfigPortModule> port_modules;
            tmp = my_info_->parent_info->component->getLinkFromParentSharedPort(name, port_modules);
            // If I got a link from my parent, I need to put it in my
            // link map

            if ( nullptr != tmp ) {
                if ( nullptr == myLinks ) {
                    myLinks            = new LinkMap();
                    my_info_->link_map = myLinks;
                }
                myLinks->insertLink(name, tmp);
                // Need to set the link's defaultTimeBase to uninitialized
                tmp->resetDefaultTimeBase();

                // Need to see if I got any port_modules, if so, need
                // to add them to my_info_->portModules

                if ( port_modules.size() > 0 ) {
                    if ( nullptr == my_info_->portModules ) {
                        // This memory is currently leaked as portModules is otherwise a pointer to ConfigComponent
                        // ConfigComponent does not exist for anonymous subcomponents
                        my_info_->portModules = new std::map<std::string, std::vector<ConfigPortModule>>();
                    }
                    (*my_info_->portModules)[name].swap(port_modules);
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
        else {
            tmp->setFunctor(handler);

            // Check to see if there is a profile tool installed
            auto tools = sim_->getProfileTool<Profile::EventHandlerProfileTool>("event");
            for ( auto& tool : tools ) {
                EventHandlerMetaData mdata(my_info_->getID(), getName(), getType(), name);

                // Add the receive profiler to the handler
                if ( tool->profileReceives() ) handler->attachTool(tool, mdata);

                // Add the send profiler to the link
                if ( tool->profileSends() ) tmp->attachTool(tool, mdata);
            }
        }

        // Check for PortModules
        // portModules pointer may be invalid after wire up
        // Only SelfLinks can be initialized after wire up and SelfLinks do not support PortModules
        if ( !sim_->isWireUpFinished() && my_info_->portModules != nullptr ) {
            auto it = my_info_->portModules->find(name);
            if ( it != my_info_->portModules->end() ) {
                EventHandlerMetaData mdata(my_info_->getID(), getName(), getType(), name);
                for ( auto& portModule : it->second ) {
                    auto* pm = Factory::getFactory()->CreateWithParams<PortModule>(
                        portModule.type, portModule.params, portModule.params);
                    pm->setComponent(this);
                    if ( pm->installOnSend() ) tmp->attachTool(pm, mdata);
                    if ( pm->installOnReceive() ) {
                        if ( handler )
                            handler->attachInterceptTool(pm, mdata);
                        else
                            fatal(
                                CALL_INFO_LONG, 1, "ERROR: Trying to install a receive PortModule on a Polling Link\n");
                    }
                    portModules.push_back(pm);
                }
            }
        }
        tmp->setDefaultTimeBase(time_base);
#ifdef __SST_DEBUG_EVENT_TRACKING__
        tmp->setSendingComponentInfo(my_info_->getName(), my_info_->getType(), name);
#endif
    }
    return tmp;
}

Link*
BaseComponent::configureLink(const std::string& name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    // Lookup core-owned time_base in case it differs from the one passed in (unlikely but possible)
    SimTime_t factor = 0;
    if ( nullptr != time_base )
        factor = time_base->getFactor();
    else if ( my_info_->defaultTimeBase.isInitialized() )
        factor = my_info_->defaultTimeBase.getFactor();

    return configureLink_impl(name, factor, handler);
}

Link*
BaseComponent::configureLink(const std::string& name, TimeConverter time_base, Event::HandlerBase* handler)
{
    return configureLink_impl(name, time_base.getFactor(), handler);
}

Link*
BaseComponent::configureLink(const std::string& name, const std::string& time_base, Event::HandlerBase* handler)
{
    SimTime_t factor = Simulation_impl::getTimeLord()->getTimeConverter(time_base)->getFactor();
    return configureLink_impl(name, factor, handler);
}

Link*
BaseComponent::configureLink(const std::string& name, const UnitAlgebra& time_base, Event::HandlerBase* handler)
{
    SimTime_t factor = Simulation_impl::getTimeLord()->getTimeConverter(time_base)->getFactor();
    return configureLink_impl(name, factor, handler);
}

Link*
BaseComponent::configureLink(const std::string& name, Event::HandlerBase* handler)
{
    SimTime_t factor = my_info_->defaultTimeBase ? my_info_->defaultTimeBase.getFactor() : 0;
    return configureLink_impl(name, factor, handler);
}

void
BaseComponent::addSelfLink(const std::string& name)
{
    LinkMap* myLinks = my_info_->getLinkMap();
    myLinks->addSelfPort(name);
    if ( myLinks->getLink(name) != nullptr ) {
        printf("Attempting to add self link with duplicate name: %s\n", name.c_str());
        abort();
    }

    Link* link = new SelfLink();
    // Set default time base to the component time base
    link->setDefaultTimeBase(my_info_->defaultTimeBase);
    myLinks->insertLink(name, link);
}

Link*
BaseComponent::configureSelfLink(const std::string& name, TimeConverter time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name, time_base, handler);
}

Link*
BaseComponent::configureSelfLink(const std::string& name, TimeConverter* time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name, *time_base, handler);
}

Link*
BaseComponent::configureSelfLink(const std::string& name, const std::string& time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name, time_base, handler);
}

Link*
BaseComponent::configureSelfLink(const std::string& name, const UnitAlgebra& time_base, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name, time_base, handler);
}

Link*
BaseComponent::configureSelfLink(const std::string& name, Event::HandlerBase* handler)
{
    addSelfLink(name);
    return configureLink(name, handler);
}

UnitAlgebra
BaseComponent::getCoreTimeBase() const
{
    return sim_->getTimeLord()->getTimeBase();
}

SimTime_t
BaseComponent::getCurrentSimCycle() const
{
    return sim_->getCurrentSimCycle();
}

int
BaseComponent::getCurrentPriority() const
{
    return sim_->getCurrentPriority();
}

UnitAlgebra
BaseComponent::getElapsedSimTime() const
{
    return sim_->getElapsedSimTime();
}

SimTime_t
BaseComponent::getEndSimCycle() const
{
    return sim_->getEndSimCycle();
}

UnitAlgebra
BaseComponent::getEndSimTime() const
{
    return sim_->getEndSimTime();
}

RankInfo
BaseComponent::getRank() const
{
    return sim_->getRank();
}

RankInfo
BaseComponent::getNumRanks() const
{
    return sim_->getNumRanks();
}

Output&
BaseComponent::getSimulationOutput() const
{
    return Simulation_impl::getSimulationOutput();
}

SimTime_t
BaseComponent::getCurrentSimTime(TimeConverter tc) const
{
    return tc.convertFromCoreTime(sim_->getCurrentSimCycle());
}

SimTime_t
BaseComponent::getCurrentSimTime(TimeConverter* tc) const
{
    return getCurrentSimTime(*tc);
}

SimTime_t
BaseComponent::processCurrentTimeWithUnderflowedBase(const std::string& base) const
{
    // Use UnitAlgebra to compute because core timebase was too big to
    // represent the requested units
    UnitAlgebra uabase(base);
    UnitAlgebra curr_time = sim_->getElapsedSimTime();

    UnitAlgebra result = curr_time / uabase;

    auto value = result.getValue();
    if ( value > static_cast<uint64_t>(MAX_SIMTIME_T) ) {
        throw std::overflow_error("Error:  Current time (" + curr_time.toStringBestSI() +
                                  ") is too large to fit into a 64-bit integer when using requested base (" + base +
                                  ")");
    }

    return value.toUnsignedLong();
}

SimTime_t
BaseComponent::getCurrentSimTime(const std::string& base) const
{
    SimTime_t ret;
    try {
        TimeConverter* tc = Simulation_impl::getTimeLord()->getTimeConverter(base);
        ret               = getCurrentSimTime(*tc);
    }
    catch ( std::underflow_error& e ) {
        // base is too small for the core timebase, fall back to using UnitAlgebra
        ret = processCurrentTimeWithUnderflowedBase(base);
    }

    return ret;
}


SimTime_t
BaseComponent::getCurrentSimTimeNano() const
{
    TimeConverter* tc = Simulation_impl::getTimeLord()->getNano();
    if ( tc ) return tc->convertFromCoreTime(sim_->getCurrentSimCycle());
    return getCurrentSimTime("1 ns");
}

SimTime_t
BaseComponent::getCurrentSimTimeMicro() const
{
    TimeConverter* tc = Simulation_impl::getTimeLord()->getMicro();
    if ( tc ) return tc->convertFromCoreTime(sim_->getCurrentSimCycle());
    return getCurrentSimTime("1 us");
}

SimTime_t
BaseComponent::getCurrentSimTimeMilli() const
{
    TimeConverter* tc = Simulation_impl::getTimeLord()->getMilli();
    if ( tc ) return tc->convertFromCoreTime(sim_->getCurrentSimCycle());
    return getCurrentSimTime("1 ms");
}


double
BaseComponent::getRunPhaseElapsedRealTime() const
{
    return sim_->getRunPhaseElapsedRealTime();
}

double
BaseComponent::getInitPhaseElapsedRealTime() const
{
    return sim_->getInitPhaseElapsedRealTime();
}

double
BaseComponent::getCompletePhaseElapsedRealTime() const
{
    return sim_->getCompletePhaseElapsedRealTime();
}

bool
BaseComponent::isSimulationRunModeInit() const
{
    return sim_->getSimulationMode() == SimulationRunMode::INIT;
}

bool
BaseComponent::isSimulationRunModeRun() const
{
    return sim_->getSimulationMode() == SimulationRunMode::RUN;
}

bool
BaseComponent::isSimulationRunModeBoth() const
{
    return sim_->getSimulationMode() == SimulationRunMode::BOTH;
}

std::string&
BaseComponent::getOutputDirectory() const
{
    return sim_->getOutputDirectory();
}

void
BaseComponent::requireLibrary(const std::string& name)
{
    sim_->requireLibrary(name);
}

uint8_t
BaseComponent::getStatisticValidityAndLevel(const std::string& statisticName) const
{
    const std::string& type = my_info_->getType();
    return Factory::getFactory()->GetStatisticValidityAndEnableLevel(type, statisticName);
}

StatisticProcessingEngine*
BaseComponent::getStatEngine()
{
    return &sim_->stat_engine;
}

void
BaseComponent::vfatal(
    uint32_t line, const char* file, const char* func, int exit_code, const char* format, va_list arg) const
{
    Output abort("Rank: @R,@I, time: @t - called in file: @f, line: @l, function: @p", 5, -1, Output::STDOUT);

    // Get info about the simulation
    std::string    name      = my_info_->getName();
    std::string    type      = my_info_->getType();
    // Build up the full list of types all the way to parent component
    std::string    type_tree = my_info_->getType();
    ComponentInfo* parent    = my_info_->parent_info;
    while ( parent != nullptr ) {
        type_tree = parent->type + "." + type_tree;
        parent    = parent->parent_info;
    }

    std::string prologue = format_string(
        "Element name: %s,  type: %s (full type tree: %s)", name.c_str(), type.c_str(), type_tree.c_str());

    std::string msg = vformat_string(format, arg);
    abort.fatal(line, file, func, exit_code, "\n%s\n%s\n", prologue.c_str(), msg.c_str());
}

void
BaseComponent::fatal(uint32_t line, const char* file, const char* func, int exit_code, const char* format, ...) const
{
    va_list arg;
    va_start(arg, format);
    vfatal(line, file, func, exit_code, format, arg);
    va_end(arg);
}

void
BaseComponent::sst_assert(
    bool condition, uint32_t line, const char* file, const char* func, int exit_code, const char* format, ...) const
{
    if ( !condition ) {
        va_list arg;
        va_start(arg, format);
        vfatal(line, file, func, exit_code, format, arg);
        va_end(arg);
    }
}

SubComponentSlotInfo*
BaseComponent::getSubComponentSlotInfo(const std::string& name, bool fatalOnEmptyIndex)
{
    SubComponentSlotInfo* info = new SubComponentSlotInfo(this, name);
    if ( info->getMaxPopulatedSlotNumber() < 0 ) {
        // Nothing registered on this slot
        delete info;
        return nullptr;
    }
    if ( !info->isAllPopulated() && fatalOnEmptyIndex ) {
        Simulation_impl::getSimulationOutput().fatal(CALL_INFO, 1,
            "SubComponent slot %s requires a dense allocation of SubComponents and did not get one.\n", name.c_str());
    }
    return info;
}

TimeConverter*
BaseComponent::getDefaultTimeBase()
{
    return Simulation_impl::getTimeLord()->getTimeConverter(my_info_->defaultTimeBase.getFactor());
}


const TimeConverter*
BaseComponent::getDefaultTimeBase() const
{
    return Simulation_impl::getTimeLord()->getTimeConverter(my_info_->defaultTimeBase.getFactor());
}

bool
BaseComponent::doesSubComponentExist(const std::string& type)
{
    return Factory::getFactory()->doesSubComponentExist(type);
}

Statistics::StatisticBase*
BaseComponent::createEnabledAllStatistic(
    Params& params, const std::string& name, const std::string& stat_sub_id, StatCreateFunction fxn)
{
    // Check if statistic was already registered, if so, return it
    auto iter = enabled_all_stats_.find(name);
    if ( iter != enabled_all_stats_.end() ) {
        auto& submap  = iter->second;
        auto  subiter = submap.find(stat_sub_id);
        if ( subiter != submap.end() ) {
            return subiter->second;
        }
    }

    // New registration
    params.insert(my_info_->all_stat_config_->params);
    Statistics::StatisticBase* stat       = fxn(this, getStatEngine(), name, stat_sub_id, params);
    enabled_all_stats_[name][stat_sub_id] = stat;
    return stat;
}

Statistics::StatisticBase*
BaseComponent::createExplicitlyEnabledStatistic(
    Params& params, StatisticId_t id, const std::string& name, const std::string& stat_sub_id, StatCreateFunction fxn)
{
    Output& out = Simulation_impl::getSimulationOutput();
    if ( my_info_->parent_info ) {
        out.fatal(CALL_INFO, 1, "Creating explicitly enabled statistic '%s' should only happen in parent component",
            name.c_str());
    }

    auto piter = my_info_->stat_configs_->find(id);
    if ( piter == my_info_->stat_configs_->end() ) {
        out.fatal(
            CALL_INFO, 1, "Explicitly enabled statistic '%s' does not have parameters mapped to its ID", name.c_str());
    }
    auto& cfg = piter->second;
    if ( cfg.shared ) {
        auto iter = explicitly_enabled_shared_stats_.find(id);
        if ( iter != explicitly_enabled_shared_stats_.end() ) {
            return iter->second;
        }
        else {
            params.insert(cfg.params);
            Statistics::StatisticBase* stat      = fxn(this, getStatEngine(), cfg.name, "", params);
            explicitly_enabled_shared_stats_[id] = stat;
            return stat;
        }
    }
    else {
        auto iter = explicitly_enabled_unique_stats_.find(id);
        if ( iter != explicitly_enabled_unique_stats_.end() ) {
            auto& map     = iter->second;
            auto  subiter = map.find(name);
            if ( subiter != map.end() ) {
                auto& submap     = subiter->second;
                auto  subsubiter = submap.find(stat_sub_id);
                if ( subsubiter != submap.end() ) {
                    return subsubiter->second;
                }
            }
        }
        // stat does not exist yet
        params.insert(cfg.params);
        auto* stat                                              = fxn(this, getStatEngine(), name, stat_sub_id, params);
        explicitly_enabled_unique_stats_[id][name][stat_sub_id] = stat;
        return stat;
    }

    // unreachable
    return nullptr;
}

void
BaseComponent::performStatisticOutput(StatisticBase* stat)
{
    sim_->getStatisticsProcessingEngine()->performStatisticOutput(stat);
}

void
BaseComponent::performGlobalStatisticOutput()
{
    sim_->getStatisticsProcessingEngine()->performGlobalStatisticOutput(false);
}

std::vector<Profile::ComponentProfileTool*>
BaseComponent::getComponentProfileTools(const std::string& point)
{
    return sim_->getProfileTool<Profile::ComponentProfileTool>(point);
}

void
BaseComponent::initiateInteractive(const std::string& msg)
{
    sim_->enter_interactive_ = true;
    sim_->interactive_msg_   = msg;
}


void
BaseComponent::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(my_info_);
    SST_SER(component_state_);

    switch ( ser.mode() ) {
    case SST::Core::Serialization::serializer::SIZER:
    case SST::Core::Serialization::serializer::PACK:
    {
        // Serialize our registered_clocks_
        SST_SER(registered_clocks_);

        // Need to serialize each handler
        std::pair<Clock::HandlerBase*, SimTime_t> p;
        size_t                                    num_handlers = clock_handlers_.size();
        SST_SER(num_handlers);
        for ( auto* handler : clock_handlers_ ) {
            p.first  = handler;
            // See if it's currently registered with a clock
            p.second = sim_->getClockForHandler(handler);
            SST_SER(p);
        }
        break;
    }
    case SST::Core::Serialization::serializer::UNPACK:
    {
        sim_ = Simulation_impl::getSimulation();

        if ( isStateDoNotEndSim() ) {
            // First set state to OKToEndSim to suppress warning in
            // primaryComponentDoNotEndSim().
            setStateOKToEndSim();
            primaryComponentDoNotEndSim();
        }

        SST_SER(registered_clocks_);
        for ( auto x : registered_clocks_ ) {
            sim_->reportClock(x, CLOCKPRIORITY);
        }

        std::pair<Clock::HandlerBase*, SimTime_t> p;
        size_t                                    num_handlers;
        SST_SER(num_handlers);
        for ( size_t i = 0; i < num_handlers; ++i ) {
            SST_SER(p);
            // Add handler to clock_handlers list
            clock_handlers_.push_back(p.first);
            // If it was previously registered, register it now
            if ( p.second ) {
                sim_->registerClock(p.second, p.first, CLOCKPRIORITY);
            }
        }
        break;
    }
    case SST::Core::Serialization::serializer::MAP:
        // All variables for BaseComponent are mapped in the
        // SerializeBaseComponentHelper class. Nothing to do here.
        break;
    }
}

// Add a watch point to all handlers in the Component Tree
void
BaseComponent::addWatchPoint(WatchPoint* pt)
{
    // Find parent component

    ComponentInfo* curr = my_info_;
    while ( curr->parent_info != nullptr ) {
        curr = curr->parent_info;
    }
    curr->component->addWatchPointRecursive(pt);
}

void
BaseComponent::addWatchPointRecursive(WatchPoint* pt)
{
    // Find all my handlers, then call all my children

    // Clock handlers
    ClockHandlerMetaData mdata(my_info_->getID(), getName(), getType());
    for ( Clock::HandlerBase* x : clock_handlers_ ) {
        // Add the receive profiler to the handler
        x->attachTool(pt, mdata);
    }

    // Event Handlers
    LinkMap* myLinks = my_info_->getLinkMap();
    if ( myLinks != nullptr ) {
        std::map<std::string, Link*>& linkmap = myLinks->getLinkMap();
        for ( auto& x : linkmap ) {
            if ( x.second != nullptr ) {
                // Need to get the handler info from my pair link
                EventHandlerMetaData mdata(my_info_->getID(), getName(), getType(), x.first);
                Event::HandlerBase* handler = reinterpret_cast<Event::HandlerBase*>(x.second->pair_link->delivery_info);
                // Check to make sure there is a handler. Links
                // configured as polling links will not have a handler
                if ( handler ) handler->attachTool(pt, mdata);
            }
        }
    }


    // Call for my subcomponents
    for ( auto it = my_info_->subComponents.begin(); it != my_info_->subComponents.end(); ++it ) {
        it->second.component->addWatchPointRecursive(pt);
    }
}

// Remove a watch point from all handlers in the Component Tree
void
BaseComponent::removeWatchPoint(WatchPoint* pt)
{
    // Find parent component
    ComponentInfo* curr = my_info_;
    while ( curr->parent_info != nullptr ) {
        curr = curr->parent_info;
    }
    curr->component->removeWatchPointRecursive(pt);
}

void
BaseComponent::removeWatchPointRecursive(WatchPoint* pt)
{
    // Find all my handlers, then call all my children

    // Clock handlers
    for ( Clock::HandlerBase* x : clock_handlers_ ) {
        x->detachTool(pt);
    }

    // Event Handlers
    LinkMap* myLinks = my_info_->getLinkMap();
    if ( myLinks != nullptr ) {
        std::map<std::string, Link*>& linkmap = myLinks->getLinkMap();
        for ( auto& x : linkmap ) {
            if ( x.second != nullptr ) {
                // Need to get the handler info from my pair link
                Event::HandlerBase* handler = reinterpret_cast<Event::HandlerBase*>(x.second->pair_link->delivery_info);
                // Check to make sure there is a handler. Links
                // configured as polling links will not have a handler
                if ( handler ) handler->detachTool(pt);
            }
        }
    }

    // Call for my subcomponents
    for ( auto it = my_info_->subComponents.begin(); it != my_info_->subComponents.end(); ++it ) {
        it->second.component->removeWatchPointRecursive(pt);
    }
}


/**** Primary Component API ****/

void
BaseComponent::registerAsPrimaryComponent()
{
    // Set to be a primary component if not already set.  Not an error
    // to call more than once.
    if ( sim_->isWireUpFinished() ) {
        // Error, called after construct phase
        sim_->getSimulationOutput().fatal(
            CALL_INFO, 1, "ERROR: registerAsPrimaryComponent() must be called during ComponentConstruction\n");
    }
    else if ( !isStatePrimary() ) {
        setStateAsPrimary();
    }
    else {
        sim_->getSimulationOutput().verbose(CALL_INFO, 1, 1,
            "WARNING: Component (%s) called registerAsPrimaryComponent() more than once\n", getName().c_str());
    }
}

void
BaseComponent::primaryComponentDoNotEndSim()
{
    if ( !isStatePrimary() ) {
        sim_->getSimulationOutput().verbose(CALL_INFO, 1, 1,
            "WARNING: Component (%s) called primaryComponentDoNotEndSim() without first "
            "calling registerAsPrimaryComponent(). Call had no effect.\n",
            getName().c_str());
    }
    else if ( isStateDoNotEndSim() ) {
        sim_->getSimulationOutput().verbose(CALL_INFO, 1, 1,
            "WARNING: Component (%s) had multiple calls to primaryComponentDoNotEndSim()\n", getName().c_str());
    }
    else {
        setStateDoNotEndSim();
        int thread = sim_->getRank().thread;
        sim_->getExit()->refInc(thread);
    }
}

void
BaseComponent::primaryComponentOKToEndSim()
{
    if ( !isStatePrimary() ) {
        sim_->getSimulationOutput().verbose(CALL_INFO, 1, 1,
            "WARNING: Component (%s) called primaryComponentOKToEndSim() without first "
            "calling registerAsPrimaryComponent(). Call had no effect.\n",
            getName().c_str());
    }
    else if ( isStateOKToEndSim() ) {
        sim_->getSimulationOutput().verbose(CALL_INFO, 1, 1,
            "WARNING: Component (%s) had multiple calls to primaryComponentOKToEndSim()\n", getName().c_str());
    }
    else {
        setStateOKToEndSim();
        int thread = sim_->getRank().thread;
        sim_->getExit()->refDec(thread);
    }
}


namespace Core::Serialization::pvt {

static const long null_ptr_id = -1;

void
SerializeBaseComponentHelper::size_basecomponent(serializable_base* s, serializer& ser)
{
    long dummy = 0;
    ser.size(dummy);
    if ( s ) {
        s->serialize_order(ser);
    }
}

void
SerializeBaseComponentHelper::pack_basecomponent(serializable_base* s, serializer& ser)
{
    if ( s ) {
        long cls_id = s->cls_id();
        ser.pack(cls_id);
        s->serialize_order(ser);
    }
    else {
        long id = null_ptr_id;
        ser.pack(id);
    }
}

void
SerializeBaseComponentHelper::unpack_basecomponent(serializable_base*& s, serializer& ser)
{
    long cls_id;
    ser.unpack(cls_id);
    if ( cls_id == null_ptr_id ) {
        s = nullptr;
    }
    else {
        s = SST::Core::Serialization::serializable_factory::get_serializable(cls_id);
        ser.unpacker().report_new_pointer(reinterpret_cast<uintptr_t>(s));
        s->serialize_order(ser);
    }
}

void
SerializeBaseComponentHelper::map_basecomponent(serializable_base*& s, serializer& ser, const std::string& name)
{
    if ( nullptr == s ) return;

    BaseComponent*  comp    = static_cast<BaseComponent*>(s);
    ObjectMapClass* obj_map = new ObjectMapClass(s, s->cls_name());
    ser.mapper().report_object_map(obj_map);
    ser.mapper().map_hierarchy_start(name, obj_map);

    // Put in any subcomponents first
    for ( auto it = comp->my_info_->subComponents.begin(); it != comp->my_info_->subComponents.end(); ++it ) {
        std::string name_str = it->second.getShortName();
        if ( name_str == "" ) {
            // This is an anonymous subcomponent, create a name based
            // on slotname and slotnum
            name_str += it->second.getSlotName() + "[" + std::to_string(it->second.getSlotNum()) + "]";
        }
        SST_SER_NAME(it->second.component, name_str.c_str());
        it->second.serialize_comp(ser);
    }

    // Put in ComponentInfo data
    ObjectMap* my_info_dir = new ObjectMapHierarchyOnly();
    ser.mapper().map_hierarchy_start("my_info_", my_info_dir);

    SST_SER_NAME(const_cast<ComponentId_t&>(comp->my_info_->id_), "id", SerOption::map_read_only);
    SST_SER_NAME(const_cast<std::string&>(comp->my_info_->type), "type", SerOption::map_read_only);
    SST_SER_NAME(comp->my_info_->defaultTimeBase, "defaultTimeBase");

    ser.mapper().map_hierarchy_end(); // for my_info_dir

    s->serialize_order(ser);
    ser.mapper().map_hierarchy_end(); // obj_map
}

} // namespace Core::Serialization::pvt
} // namespace SST
