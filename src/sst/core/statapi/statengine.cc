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

#include "sst/core/statapi/statengine.h"

#include "sst/core/baseComponent.h"
#include "sst/core/configGraph.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/factory.h"
#include "sst/core/impl/oneshotManager.h"
#include "sst/core/output.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <string>
#include <utility>

namespace SST::Statistics {

std::vector<StatisticOutput*> StatisticProcessingEngine::stat_outputs_;

StatisticProcessingEngine::StatisticProcessingEngine() :
    output_(Output::getDefaultObject())
{
    // Stat group for default event-based and end-of-time output
    StatisticGroup group;
    stat_default_groups_.insert(std::make_pair(0, group));
}


void
StatisticProcessingEngine::static_setup(StatsConfig* stats_config)
{
    // Outputs are per MPI rank, so have to be static data
    for ( auto& cfg : stats_config->outputs ) {
        stat_outputs_.push_back(createStatisticOutput(cfg));
    }
}

void
StatisticProcessingEngine::stat_outputs_simulation_start()
{
    for ( auto& so : stat_outputs_ ) {
        so->startOfSimulation();
    }
}

void
StatisticProcessingEngine::stat_outputs_simulation_end()
{
    for ( auto& so : stat_outputs_ ) {
        so->endOfSimulation();
    }
}


void
StatisticProcessingEngine::setup(StatsConfig* stats_config)
{
    simulation_started_ = false;
    stat_load_level_    = stats_config->load_level;

    if ( stat_default_groups_.empty() ) stat_default_groups_.insert(std::make_pair(0, StatisticGroup()));
    for ( auto& group : stat_default_groups_ ) {
        group.second.output = stat_outputs_[0];
    }
    for ( auto& cfg : stats_config->groups ) {
        stat_groups_.emplace_back(cfg.second, this);
    }
}

void
StatisticProcessingEngine::restart()
{
    simulation_started_ = false;
    if ( stat_default_groups_.empty() ) stat_default_groups_.insert(std::make_pair(0, StatisticGroup()));
    for ( auto group : stat_default_groups_ ) {
        group.second.output = stat_outputs_[0];
    }
    for ( std::vector<StatisticGroup>::iterator it = stat_groups_.begin(); it != stat_groups_.end(); it++ ) {
        it->restartGroup(this);
    }
}

StatisticProcessingEngine::~StatisticProcessingEngine()
{
    stat_default_groups_.clear();
    stat_groups_.clear();
}

// Regular start registration
bool
StatisticProcessingEngine::registerStatisticWithEngine(StatisticBase* stat, Params& params)
{
    if ( stat->isNullStatistic() ) return true;

    auto* comp = stat->getComponent();
    if ( comp == nullptr ) {
        output_.verbose(CALL_INFO, 1, 0, " Error: Statistic %s hasn't any associated component .\n",
            stat->getFullStatName().c_str());
        return false;
    }

    // Determine group if any
    StatisticGroup& group = getGroupForStatistic(stat);

    // Make sure that the wireup has not been completed
    // If it has, stat output must support dynamic registration
    if ( true == Simulation_impl::getSimulation()->isWireUpFinished() ) {
        if ( !group.output->supportsDynamicRegistration() ) {
            output_.fatal(CALL_INFO, 1,
                "ERROR: Statistic %s - "
                "Cannot be registered for output %s after the Components have been wired up. "
                "Statistics on output %s must be registered on Component creation. exiting...\n",
                stat->getFullStatName().c_str(), group.output->getStatisticOutputName().c_str(),
                group.output->getStatisticOutputName().c_str());
        }
    }

    // Configure output rate - new stat only
    std::string rate     = params.find<std::string>("rate", "0ns");
    UnitAlgebra rate_ua  = UnitAlgebra(rate);
    bool        periodic = false;
    if ( rate_ua.hasUnits("s") || rate_ua.hasUnits("hz") ) {
        periodic = true;
    }
    else if ( !rate_ua.hasUnits("event") ) {
        // rate has a unit type we dont recognize
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1,
            "ERROR: Statistic %s - Collection Rate = %s not valid; exiting...\n", stat->getFullStatName().c_str(),
            rate.c_str());
    }
    stat->setRegisteredCollectionMode(periodic);

    // Check that the stat's mode is compatible with the stat's type
    if ( !stat->isStatModeSupported(periodic) ) {
        output_.fatal(CALL_INFO, 1,
            " Warning: Statistic %s Does not support %s Based Collections; Collection Rate = %s\n",
            stat->getFullStatName().c_str(), (periodic ? "Periodic" : "Event"), rate.c_str());
    }

    // Do actual group assignment
    if ( group.is_default ) {
        // If the mode is Periodic Based, add the statistic to the
        // StatisticProcessingEngine otherwise add it as an Event Based Stat.
        bool success = true;
        if ( periodic ) {
            SimTime_t factor = Simulation_impl::getSimulation()->getTimeLord()->getTimeConverter(rate_ua)->getFactor();
            success          = addPeriodicBasedStatistic(factor, stat); // will place stat in correct default group
        }
        else {
            addEventBasedStatistic(rate_ua, stat);
        }
        if ( !success ) return false;

        getOutputForStatistic(stat)->registerStatistic(stat);
    }
    else {
        if ( stat->isOutputEventBased() ) {
            output_.output("ERROR: Statistics in groups must be periodic or dump at end, event-based output triggers "
                           "are not allowed\n");
            return false;
        }
        group.addStatistic(stat);
    }

    // Convert params to startat/stopat
    // Note that checkpoint restart skips this b/c params are empty
    // Restart directly calls setStatisticStart/StopTime
    std::string start_at = params.find<std::string>("startat", "0ns");
    UnitAlgebra ua;
    SimTime_t   factor;
    if ( start_at != "0ns" ) {
        ua = UnitAlgebra(start_at);
        if ( ua.getValue() != 0 ) {
            factor = Simulation_impl::getSimulation()->getTimeLord()->getTimeConverter(ua)->getFactor();
            setStatisticStartTime(stat, factor);
        }
    }

    std::string stop_at = params.find<std::string>("stopat", "0ns");
    if ( stop_at != "0ns" ) {
        ua = UnitAlgebra(stop_at);
        if ( ua.getValue() != 0 ) {
            factor = Simulation_impl::getSimulation()->getTimeLord()->getTimeConverter(ua)->getFactor();
            setStatisticStopTime(stat, factor);
        }
    }

    return true;
}

// Restart registration
bool
StatisticProcessingEngine::reregisterStatisticWithEngine(
    StatisticBase* stat, SimTime_t start_at_time, SimTime_t stop_at_time, SimTime_t output_factor)
{
    if ( stat->isNullStatistic() ) return true;

    if ( stat->getComponent() == nullptr ) {
        output_.verbose(CALL_INFO, 1, 0, " Error: Statistic %s hasn't any associated component .\n",
            stat->getFullStatName().c_str());
        return false;
    }

    // Determine group if any
    StatisticGroup& group = getGroupForStatistic(stat);

    bool success = true;
    if ( group.is_default ) {
        // If the mode is Periodic Based, add the statistic to the
        // StatisticProcessingEngine otherwise add it as an Event Based Stat.
        if ( stat->isOutputPeriodic() ) {
            success = addPeriodicBasedStatistic(output_factor, stat);
        }
        else {
            stat_default_groups_[0].addStatistic(stat);
        }
        if ( !success ) return false;

        getOutputForStatistic(stat)->registerStatistic(stat);
    }
    else {
        group.addStatistic(stat);
    }

    if ( start_at_time != 0 ) setStatisticStartTime(stat, start_at_time);
    if ( stop_at_time != 0 ) setStatisticStopTime(stat, stop_at_time);

    return success;
}

void
StatisticProcessingEngine::finalizeInitialization()
{
    for ( auto& group : stat_groups_ ) {
        group.output->registerGroup(&group);

        /* Register group clock, if rate is set */
        if ( group.output_freq != 0 ) {
            Simulation_impl::getSimulation()->registerClock(group.output_freq,
                new Clock::Handler2<StatisticProcessingEngine, &StatisticProcessingEngine::handleGroupClockEvent,
                    StatisticGroup*>(this, &group),
                STATISTICCLOCKPRIORITY);
        }
    }
}

void
StatisticProcessingEngine::startOfSimulation()
{
    simulation_started_ = true;
}

void
StatisticProcessingEngine::endOfSimulation()
{
    // Output default group stats
    for ( auto& group : stat_default_groups_ ) {
        for ( StatisticBase* stat : group.second.stats ) {
            performStatisticOutputImpl(stat, true);
        }
    }

    // Output non-default group stats
    for ( auto& group : stat_groups_ ) {
        performStatisticGroupOutputImpl(group, true);
    }
}

StatisticOutput*
StatisticProcessingEngine::createStatisticOutput(const ConfigStatOutput& cfg)
{
    auto& unsafe_params = const_cast<SST::Params&>(cfg.params);
    auto  lc_type       = cfg.type;
    std::transform(lc_type.begin(), lc_type.end(), lc_type.begin(), ::tolower);
    StatisticOutput* so =
        Factory::getFactory()->CreateWithParams<StatisticOutput>(lc_type, unsafe_params, unsafe_params);
    if ( nullptr == so ) {
        // This fix addresses a problem where if the lib or element type actually has an upper case char, the
        // tolower above will not allow you to use it. However, it does mean if there's an upper case in the lib,
        // the element name will also be case-sensitive. Oh well.
        so = Factory::getFactory()->CreateWithParams<StatisticOutput>(cfg.type, unsafe_params, unsafe_params);
        if ( nullptr == so ) {
            Output::getDefaultObject().fatal(
                CALL_INFO, 1, " - Unable to instantiate Statistic Output %s\n", cfg.type.c_str());
        }
    }

    if ( false == so->checkOutputParameters() ) {
        // If checkOutputParameters() fail, Tell the user how to use them and abort simulation
        Output::getDefaultObject().output("Statistic Output (%s) :\n", so->getStatisticOutputName().c_str());
        so->printUsage();
        Output::getDefaultObject().output("\n");

        Output::getDefaultObject().output("Statistic Output Parameters Provided:\n");
        cfg.params.print_all_params(Output::getDefaultObject(), "  ");
        Output::getDefaultObject().fatal(CALL_INFO, 1, " - Required Statistic Output Parameters not set\n");
    }
    return so;
}

void
StatisticProcessingEngine::castError(
    const std::string& type, const std::string& stat_name, const std::string& field_name)
{
    Simulation_impl::getSimulationOutput().fatal(CALL_INFO, 1,
        "Unable to cast statistic %s of type %s to correct field type %s", stat_name.c_str(), type.c_str(),
        field_name.c_str());
}

StatisticOutput*
StatisticProcessingEngine::getOutputForStatistic(const StatisticBase* stat) const
{
    return stat->getGroup()->output;
}

/* Return the group that would claim this stat */
StatisticGroup&
StatisticProcessingEngine::getGroupForStatistic(const StatisticBase* stat) const
{
    for ( auto& group : stat_groups_ ) {
        if ( group.claimsStatistic(stat) ) {
            return const_cast<StatisticGroup&>(group);
        }
    }
    return const_cast<StatisticGroup&>(stat_default_groups_.at(0));
}

bool
StatisticProcessingEngine::addPeriodicBasedStatistic(SimTime_t factor, StatisticBase* stat)
{
    Simulation_impl*    sim = Simulation_impl::getSimulation();
    Clock::HandlerBase* clock_handler;

    // See if the map contains an entry for this factor
    if ( stat_default_groups_.find(factor) == stat_default_groups_.end() ) {
        // Create a new default group with this factor, factor=0 is always already in the group map
        StatisticGroup group;
        group.output_freq = factor;
        group.output      = stat_default_groups_[0].output;
        stat_default_groups_.insert(std::make_pair(factor, group));

        // This factor is not found in the map, so create a new clock handler.
        clock_handler = new Clock::Handler2<StatisticProcessingEngine,
            &StatisticProcessingEngine::handleStatisticEngineClockEvent, SimTime_t>(this, factor);

        // Set the clock priority so that normal clocks events will occur before this clock event.
        sim->registerClock(factor, clock_handler, STATISTICCLOCKPRIORITY);
    }

    // Add stat to correct group
    stat_default_groups_[factor].addStatistic(stat);

    if ( 0 != factor ) { // Set output rate flag and update stat's default group to correct one
        stat->setOutputRateFlag();
    }

    return true;
}

void
StatisticProcessingEngine::addEventBasedStatistic(const UnitAlgebra& count, StatisticBase* stat)
{
    if ( 0 != count.getValue() ) {
        // Set the Count Limit
        stat->setCollectionCountLimit(count.getRoundedValue());
    }
    else {
        stat->setCollectionCountLimit(0);
    }
    stat->setFlagResetCountOnOutput(true);

    // Add the statistic to the default group
    stat_default_groups_[0].addStatistic(stat);
}

void
StatisticProcessingEngine::setStatisticStartTime(StatisticBase* stat, SimTime_t factor)
{
    Simulation_impl* sim = Simulation_impl::getSimulation();
    StatArray_t*     stat_array;

    // Check to see if the time is zero or has already passed, if it is we skip this work
    if ( factor > sim->getCurrentSimCycle() ) {
        // See if the map contains an entry for this factor
        if ( start_time_map_.find(factor) == start_time_map_.end() ) {
            sim->one_shot_manager_.registerAbsoluteHandler<StatisticProcessingEngine,
                &StatisticProcessingEngine::handleStatisticEngineStartTimeEvent, SimTime_t>(
                factor, STATISTICCLOCKPRIORITY, this, factor);

            // Also create a new Array of Statistics and relate it to the map
            stat_array              = new std::vector<StatisticBase*>();
            start_time_map_[factor] = stat_array;
        }

        // The Statistic Map has the time factor registered.
        stat_array = start_time_map_[factor];

        // Add the statistic to the lists of statistics to be called when the OneShot fires.
        stat_array->push_back(stat);

        // Disable the Statistic until the start time event occurs
        stat->disable();
        stat->setStartAtFlag();
    }
}

SimTime_t
StatisticProcessingEngine::getStatisticStartTimeFactor(StatisticBase* stat)
{
    for ( auto& time : start_time_map_ ) {
        for ( auto stat_ptr : *time.second ) {
            if ( stat == stat_ptr ) return time.first;
        }
    }
    return 0;
}

void
StatisticProcessingEngine::setStatisticStopTime(StatisticBase* stat, SimTime_t factor)
{
    Simulation_impl* sim = Simulation_impl::getSimulation();
    StatArray_t*     stat_array;

    // Check to see if the time is zero or has already passed, if it is we skip this work
    if ( factor > sim->getCurrentSimCycle() ) {
        // See if the map contains an entry for this factor
        if ( stop_time_map_.find(factor) == stop_time_map_.end() ) {
            // This tcFactor is not found in the map, so create a new OneShot handler.
            sim->one_shot_manager_.registerAbsoluteHandler<StatisticProcessingEngine,
                &StatisticProcessingEngine::handleStatisticEngineStopTimeEvent, SimTime_t>(
                factor, STATISTICCLOCKPRIORITY, this, factor);

            // Also create a new Array of Statistics and relate it to the map
            stat_array             = new std::vector<StatisticBase*>();
            stop_time_map_[factor] = stat_array;
        }

        // The Statistic Map has the time factor registered.
        stat_array = stop_time_map_[factor];

        // Add the statistic to the lists of statistics to be called when the OneShot fires.
        stat_array->push_back(stat);
        stat->setStopAtFlag();
    }
}

SimTime_t
StatisticProcessingEngine::getStatisticStopTimeFactor(StatisticBase* stat)
{
    for ( auto& time : stop_time_map_ ) {
        for ( auto stat_ptr : *time.second ) {
            if ( stat == stat_ptr ) return time.first;
        }
    }
    return 0;
}

void
StatisticProcessingEngine::performStatisticOutput(StatisticBase* stat, bool end_of_sim_flag /*=false*/)
{
    if ( stat->getGroup()->is_default )
        performStatisticOutputImpl(stat, end_of_sim_flag);
    else
        performStatisticGroupOutputImpl(*const_cast<StatisticGroup*>(stat->getGroup()), end_of_sim_flag);
}

void
StatisticProcessingEngine::performStatisticOutputImpl(StatisticBase* stat, bool end_of_sim_flag /*=false*/)
{
    StatisticOutput* stat_output = getOutputForStatistic(stat);

    // Has the simulation started?
    if ( true == simulation_started_ ) {
        stat_output->output(stat, end_of_sim_flag);

        if ( false == end_of_sim_flag ) {
            // Check to see if the Statistic Count needs to be reset
            if ( true == stat->getFlagResetCountOnOutput() ) {
                stat->resetCollectionCount();
            }

            // Check to see if the Statistic Data needs to be cleared
            if ( true == stat->getFlagClearDataOnOutput() ) {
                stat->clearStatisticData();
            }
        }
    }
}

void
StatisticProcessingEngine::performStatisticGroupOutputImpl(StatisticGroup& group, bool end_of_sim_flag /*=false*/)
{
    StatisticOutput* stat_output = group.output;

    // Has the simulation started?
    if ( true == simulation_started_ ) {

        stat_output->outputGroup(&group, end_of_sim_flag);

        if ( false == end_of_sim_flag ) {
            for ( auto& stat : group.stats ) {
                // Check to see if the Statistic Count needs to be reset
                if ( true == stat->getFlagResetCountOnOutput() ) {
                    stat->resetCollectionCount();
                }

                // Check to see if the Statistic Data needs to be cleared
                if ( true == stat->getFlagClearDataOnOutput() ) {
                    stat->clearStatisticData();
                }
            }
        }
    }
}

void
StatisticProcessingEngine::performGlobalStatisticOutput(bool end_of_sim_flag /*=false*/)
{
    // Output default stats
    for ( auto& group : stat_default_groups_ ) {
        for ( auto& stat : group.second.stats ) {
            performStatisticOutputImpl(stat, end_of_sim_flag);
        }
    }

    // Output non-default stats
    for ( auto& group : stat_groups_ ) {
        performStatisticGroupOutputImpl(group, end_of_sim_flag);
    }
}

bool
StatisticProcessingEngine::handleStatisticEngineClockEvent(Cycle_t UNUSED(cycle_num), SimTime_t time_factor)
{
    // Walk the default group for this time_factor and call the output method of each statistic
    for ( auto& stat : stat_default_groups_[time_factor].stats ) {
        // Perform the output
        performStatisticOutputImpl(stat, false);
    }
    // Return false to keep the clock going
    return false;
}

bool
StatisticProcessingEngine::handleGroupClockEvent(Cycle_t UNUSED(cycle_num), StatisticGroup* group)
{
    performStatisticGroupOutputImpl(*group, false);
    return false;
}

void
StatisticProcessingEngine::handleStatisticEngineStartTimeEvent(SimTime_t time_factor)
{
    StatArray_t* stat_array = start_time_map_[time_factor];
    for ( auto& stat : *stat_array ) {
        stat->enable();
        stat->unsetStartAtFlag();
    }

    start_time_map_.erase(time_factor);
    delete stat_array;
}

void
StatisticProcessingEngine::handleStatisticEngineStopTimeEvent(SimTime_t time_factor)
{
    StatArray_t* stat_array = stop_time_map_[time_factor];
    for ( auto& stat : *stat_array ) {
        stat->disable();
        stat->unsetStopAtFlag();
    }

    stop_time_map_.erase(time_factor);
    delete stat_array;
}

void
StatisticProcessingEngine::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(simulation_started_); // Always true at restart (checkpoints can't occur before sim start)
    SST_SER(stat_load_level_);    // This is global
    SST_SER(stat_groups_); // Going to have to revisit if changing partitioning - will stat groups need to be global?
                           // Are they global already?
    // Default stat groups are reconstructed on restart
}

} // namespace SST::Statistics
