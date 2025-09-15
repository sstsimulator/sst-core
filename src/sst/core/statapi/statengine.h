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

#ifndef SST_CORE_STATAPI_STATENGINE_H
#define SST_CORE_STATAPI_STATENGINE_H

#include "sst/core/clock.h"
#include "sst/core/factory.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statfieldinfo.h"
#include "sst/core/statapi/statgroup.h"
#include "sst/core/statapi/statnull.h"
#include "sst/core/threadsafe.h"
#include "sst/core/unitAlgebra.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

/* Forward declare for Friendship */
extern int  main(int argc, char** argv);
extern void finalize_statEngineConfig();

namespace SST {
class BaseComponent;
class Simulation_impl;
class ConfigGraph;
class ConfigStatGroup;
class ConfigStatOutput;
class Params;
struct StatsConfig;

namespace Statistics {

class StatisticOutput;

/**
    \class StatisticProcessingEngine

    An SST core component that handles timing and event processing informing
    all registered Statistics to generate their outputs at desired rates.
*/
class StatisticProcessingEngine : public SST::Core::Serialization::serializable
{

public:

    /** Called to create an enabled (non-null) statistic of the requested type
     *  This function also registers the statistic with the engine
     *  @param comp The statistic owner
     *  @param stat_name The name of the statistic
     *  @param stat_sub_id An optional id to differentiate multiple instances of the same statistic, "" if none
     *  @param params The parameters that should be used to configure the statistic
     */
    template <class T>
    Statistic<T>* createStatistic(
        BaseComponent* comp, const std::string& stat_name, const std::string& stat_sub_id, Params& params)
    {
        // Create a new statistic
        std::string   type = params.find<std::string>("type", "sst.AccumulatorStatistic");
        Statistic<T>* stat =
            Factory::getFactory()->CreateWithParams<Statistic<T>>(type, params, comp, stat_name, stat_sub_id, params);

        // Register with engine
        registerStatisticWithEngine(stat, params);
        return stat;
    }

    /** Called to create a disabled (null) statistic of the requested type
     * @param comp The statistic owner
     */
    template <class T>
    Statistic<T>* createDisabledStatistic()
    {
        SST::Params          empty {};
        static Statistic<T>* stat =
            Factory::getFactory()->CreateWithParams<Statistic<T>>("sst.NullStatistic", empty, nullptr, "", "", empty);
        return stat;
        // No need to register anything
    }

    /** Re-registers a statistic with the engine during a restart run.
     * Newly created statistics (not read from a checkpoint) use 'registerStatisticWithEngine' instead.
     * @param stat The statistic to re-register
     * @param start_at_time If the statistic should be enabled after some time, the time (in core cycles) else 0
     * @param stop_at_time If the statistic should be disabled after some time, the time (in core cycles) else 0
     * @param output_factor If the statistic should be output at a regular time interval, the interval (in core cycles)
     * else 0
     */
    bool reregisterStatisticWithEngine(
        StatisticBase* stat, SimTime_t start_at_time, SimTime_t stop_at_time, SimTime_t output_factor);

    /** Called by the Components and Subcomponent to perform a statistic Output.
     * @param stat - Pointer to the statistic.
     * @param end_of_sim_flag - Indicates that the output is occurring at the end of simulation.
     */
    void performStatisticOutput(StatisticBase* stat, bool end_of_sim_flag = false);

    /** Called by the Components and Subcomponent to perform a global statistic Output.
     * This routine will force ALL Components and Subcomponents to output their statistic information.
     * This may lead to unexpected results if the statistic counts or data is reset on output.
     * @param end_of_sim_flag - Indicates that the output is occurring at the end of simulation.
     */
    void performGlobalStatisticOutput(bool end_of_sim_flag = false);

    /** Return global statistic load level */
    uint8_t getStatLoadLevel() const { return stat_load_level_; }

    /** Return the statistic output objects - static as they are per-MPI rank */
    static const std::vector<StatisticOutput*>& getStatOutputs() { return stat_outputs_; }

    /** Called to setup the StatOutputs, which are shared across all
       the StatEngines on the same MPI rank.
     */
    static void static_setup(StatsConfig* stats_config);

    /** Called to notify StatOutputs that simulation has started
     */
    static void stat_outputs_simulation_start();

    /** Called to notify StatOutputs that simulation has ended
     */
    static void stat_outputs_simulation_end();

    /** Get the start time factor belonging to 'stat'.
     * Used during checkpoint. Linear search - could be slow.
     * @param stat The statistic to lookup
     */
    SimTime_t getStatisticStartTimeFactor(StatisticBase* stat);

    /** Get the stop time factor belonging to 'stat'.
     * Used during checkpoint. Linear search - could be slow.
     * @param stat The statistic to lookup
     */
    SimTime_t getStatisticStopTimeFactor(StatisticBase* stat);

    /** Serialization support for the engine. */
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Statistics::StatisticProcessingEngine)

private:
    friend class SST::Simulation_impl;
    friend int ::main(int argc, char** argv);
    friend void ::finalize_statEngineConfig();

    /** Constructor */
    StatisticProcessingEngine();

    /** Destructor */
    ~StatisticProcessingEngine();

    void setup(StatsConfig* stats_config);
    void restart();

    static StatisticOutput* createStatisticOutput(const ConfigStatOutput& cfg);

    /** Registers a newly-created statistic with the engine
     * Statistics created during a restart run should use reregisterStatisticWithEngine instead.
     */
    bool registerStatisticWithEngine(StatisticBase* stat, Params& params);

    StatisticOutput* getOutputForStatistic(const StatisticBase* stat) const;
    StatisticGroup&  getGroupForStatistic(const StatisticBase* stat) const;
    bool             addPeriodicBasedStatistic(SimTime_t factor, StatisticBase* stat);
    void             addEventBasedStatistic(const UnitAlgebra& count, StatisticBase* stat);
    void             setStatisticStartTime(StatisticBase* stat, SimTime_t factor);
    void             setStatisticStopTime(StatisticBase* stat, SimTime_t factor);

    void finalizeInitialization(); /* Called when performWireUp() finished */
    void startOfSimulation();
    void endOfSimulation();

    void performStatisticOutputImpl(StatisticBase* stat, bool end_of_sim_flag);
    void performStatisticGroupOutputImpl(StatisticGroup& group, bool end_of_sim_flag);

    bool handleStatisticEngineClockEvent(Cycle_t cycle_num, SimTime_t time_factor);
    bool handleGroupClockEvent(Cycle_t cycle_num, StatisticGroup* group);
    void handleStatisticEngineStartTimeEvent(SimTime_t time_factor);
    void handleStatisticEngineStopTimeEvent(SimTime_t time_factor);

    [[noreturn]]
    void castError(const std::string& type, const std::string& stat_name, const std::string& field_name);

    using StatArray_t = std::vector<StatisticBase*>;       /*!< Array of Statistics */
    using StatMap_t   = std::map<SimTime_t, StatArray_t*>; /*!< Map of simtimes to Statistic Arrays */

    /* All non-null statistics are stored in *one* (only one) of the following data structures:
     * - stat_groups_
     * - stat_default_groups_
     *
     * Any stats with a start time are also stored in start_time_map_ until their start time occurs
     * Any stats with a stop time are also stored in stop_time_map_ until their stop time occurs
     *
     * The stat engine creates a single instance of each null stat type which is shared across all null stats on the
     * rank. These stats are not stored by the engine.
     */
    StatMap_t start_time_map_;     /*!< Map of Array's of Statistics that are started at a sim time */
    StatMap_t stop_time_map_;      /*!< Map of Array's of Statistics that are stopped at a sim time */
    bool      simulation_started_; /*!< Flag showing if Simulation has started */

    Output& output_;
    uint8_t stat_load_level_; /*!< The global statistic load level */
    std::map<SimTime_t, StatisticGroup>
        stat_default_groups_; /*!< Statistic groups for statistics that are not explicitly assigned to one */
    std::vector<StatisticGroup> stat_groups_; /*!< A list of all statistic group objects */

    static std::vector<StatisticOutput*>
        stat_outputs_; /*!< The statistic output objects that exist on this engine's rank */
};

} // namespace Statistics
} // namespace SST

#endif // SST_CORE_STATAPI_STATENGINE_H
