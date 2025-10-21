// -*- c++ -*-

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

#ifndef SST_CORE_SIMULATION_IMPL_H
#define SST_CORE_SIMULATION_IMPL_H

#include "sst/core/clock.h"
#include "sst/core/componentInfo.h"
#include "sst/core/exit.h"
#include "sst/core/impl/oneshotManager.h"
#include "sst/core/output.h"
#include "sst/core/profile/profiletool.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statengine.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/util/basicPerf.h"
#include "sst/core/util/filesystem.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <signal.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

/* Forward declare for Friendship */
extern int main(int argc, char** argv);

namespace SST {

// Function to exit, guarding against race conditions if multiple threads call it
[[noreturn]]
void SST_Exit(int exit_code);

#define _SIM_DBG(fmt, args...) __DBG(DBG_SIM, Sim, fmt, ##args)
#define STATALLFLAG            "--ALLSTATS--"

class Activity;
class CheckpointAction;
class Component;
class Config;
class ConfigGraph;
class Exit;
class Factory;
class InteractiveConsole;
class Link;
class LinkMap;
class Params;
class RealTimeManager;
class SimulatorHeartbeat;
class SyncBase;
class SyncManager;
class ThreadSync;
class TimeConverter;
class TimeLord;
class TimeVortex;
class UnitAlgebra;

namespace Statistics {
class StatisticOutput;
class StatisticProcessingEngine;
} // namespace Statistics

namespace Serialization {
class ObjectMap;
} // namespace Serialization


namespace pvt {

/**
   Class to sort the contents of the TimeVortex in preparation for
   checkpointing
 */
struct TimeVortexSort
{
    struct less
    {
        bool operator()(const Activity* lhs, const Activity* rhs) const;
    };

    std::vector<Activity*> data;

    using iterator = std::vector<Activity*>::iterator;

    // Iterator to start of actions after sort
    iterator action_start;

    TimeVortexSort() = default;

    void sortData();

    std::pair<iterator, iterator> getEventsForHandler(uintptr_t handler);

    std::pair<iterator, iterator> getActions();
};

} // namespace pvt

/**
 * Main control class for a SST Simulation.
 * Provides base features for managing the simulation
 */
class Simulation_impl
{

public:
    SST::Core::Serialization::ObjectMap* getComponentObjectMap();

    /********  Public API inherited from Simulation ********/
    /** Get the run mode of the simulation (e.g. init, run, both etc) */
    SimulationRunMode getSimulationMode() const { return runMode; };

    /** Return the current simulation time as a cycle count*/
    SimTime_t getCurrentSimCycle() const;

    /** Return the end simulation time as a cycle count*/
    SimTime_t getEndSimCycle() const;

    /** Return the current priority */
    int getCurrentPriority() const;

    /** Return the elapsed simulation time as a time */
    UnitAlgebra getElapsedSimTime() const;

    /** Return the end simulation time as a time */
    UnitAlgebra getEndSimTime() const;

    /** Get this instance's parallel rank */
    RankInfo getRank() const { return my_rank; }

    /** Get the number of parallel ranks in the simulation */
    RankInfo getNumRanks() const { return num_ranks; }

    /**
    Returns the output directory of the simulation
    @return Directory in which simulation outputs are placed
    */
    std::string& getOutputDirectory() { return output_directory; }

    /** Signifies that a library is required for this simulation.
     *  @param name Name of the library
     */
    void requireLibrary(const std::string& name);

    /** Causes the current status of the simulation to be printed to stderr.
     * @param fullStatus - if true, call printStatus() on all components as well
     *        as print the base Simulation's status
     */
    void printStatus(bool fullStatus);

    double getRunPhaseElapsedRealTime() const;
    double getInitPhaseElapsedRealTime() const;
    double getCompletePhaseElapsedRealTime() const;

    /******** End Public API from Simulation ********/

    using clockMap_t = std::map<std::pair<SimTime_t, int>, Clock*>; /*!< Map of times to clocks */
    // using oneShotMap_t = std::map<int, OneShot*>; /*!< Map of priorities to OneShots */

    ~Simulation_impl();

    /*********  Static Core-only Functions *********/

    /** Return a pointer to the singleton instance of the Simulation */
    static Simulation_impl* getSimulation() { return instanceMap.at(std::this_thread::get_id()); }

    /** Return the TimeLord associated with this Simulation */
    static TimeLord* getTimeLord() { return &timeLord; }

    /** Return the base simulation Output class instance */
    static Output& getSimulationOutput() { return sim_output; }

    /** Create new simulation
     * @param my_rank - Parallel Rank of this simulation object
     * @param num_ranks - How many Ranks are in the simulation
     * @param restart - Whether this simulation is being restarted from a checkpoint (true) or not
     */
    static Simulation_impl* createSimulation(
        RankInfo my_rank, RankInfo num_ranks, bool restart, SimTime_t currentSimCycle, int currentPriority);

    /**
     * Used to signify the end of simulation.  Cleans up any existing Simulation Objects
     */
    static void shutdown();

    /** Sets an internal flag for signaling the simulation. Used by signal handler & thread 0. */
    static void notifySignal();

    static void serializeSharedObjectManager(SST::Core::Serialization::serializer& ser);

    /******** Core only API *************/

    /** Insert an activity to fire at a specified time */
    void insertActivity(SimTime_t time, Activity* ev);

    /** Return the exit event */
    Exit* getExit() const { return m_exit; }

    /** Processes the ConfigGraph to pull out any need information
     * about relationships among the threads
     */
    void processGraphInfo(ConfigGraph& graph, const RankInfo& myRank, SimTime_t min_part);

    int  initializeStatisticEngine(StatsConfig* stats_config);
    int  prepareLinks(ConfigGraph& graph, const RankInfo& myRank, SimTime_t min_part);
    int  performWireUp(ConfigGraph& graph, const RankInfo& myRank, SimTime_t min_part);
    void exchangeLinkInfo();

    /** Setup external control actions (forced stops, signal handling */
    void setupSimActions();

    /** Helper for signal string parsing */
    bool parseSignalString(std::string& arg, std::string& name, Params& params);

    /** Perform the init() phase of simulation */
    void initialize();

    /** Perform the complete() phase of simulation */
    void complete();

    /** Perform the setup() and run phases of the simulation. */
    void setup();

    void prepare_for_run();

    void run();

    void finish();

    /** Adjust clocks and time to reflect precise simulation end time which
        may differ in parallel simulations from the time simulation end is detected.
     */
    void adjustTimeAtSimEnd();

    bool isIndependentThread() { return independent; }

    void printProfilingInfo(FILE* fp);

    void printPerformanceInfo();

    /** Register a OneShot event to be called after a time delay
        Note: OneShot cannot be canceled, and will always callback after
              the timedelay.
    */
    // TimeConverter* registerOneShot(const std::string& timeDelay, int priority, OneShot::HandlerBase* handler, bool
    // absolute);

    // TimeConverter* registerOneShot(const UnitAlgebra& timeDelay, int priority, OneShot::HandlerBase* handler, bool
    // absolute);

    const std::vector<SimTime_t>& getInterThreadLatencies() const { return interThreadLatencies; }

    SimTime_t getInterThreadMinLatency() const { return interThreadMinLatency; }

    static TimeConverter getMinPartTC() { return minPartTC; }

    LinkMap* getComponentLinkMap(ComponentId_t id) const
    {
        ComponentInfo* info = compInfoMap.getByID(id);
        if ( nullptr == info ) {
            return nullptr;
        }
        else {
            return info->getLinkMap();
        }
    }

    /** Returns reference to the Component Info Map */
    const ComponentInfoMap& getComponentInfoMap() const { return compInfoMap; }

    /** returns the component with the given ID */
    BaseComponent* getComponent(const ComponentId_t& id) const
    {
        ComponentInfo* i = compInfoMap.getByID(id);
        if ( nullptr != i ) {
            return i->getComponent();
        }
        else {
            printf("Simulation::getComponent() couldn't find component with id = %" PRIu64 "\n", id);
            SST_Exit(1);
        }
    }

    /** returns the ComponentInfo object for the given ID */
    ComponentInfo* getComponentInfo(const ComponentId_t& id) const
    {
        ComponentInfo* i = compInfoMap.getByID(id);
        if ( nullptr != i ) {
            return i;
        }
        else {
            printf("Simulation::getComponentInfo() couldn't find component with id = %" PRIu64 "\n", id);
            SST_Exit(1);
        }
    }

    /**
    Set the output directory for this simulation
    @param outDir Path of directory to place simulation outputs in
    */
    void setOutputDirectory(const std::string& outDir) { output_directory = outDir; }

    /**
     *  Gets the minimum next activity time across all TimeVortices in
     *  the Rank
     */
    static SimTime_t getLocalMinimumNextActivityTime();

    /**
     * Returns true when the Wireup is finished.
     */
    bool isWireUpFinished() { return wireUpFinished_; }

    uint64_t getTimeVortexMaxDepth() const;

    uint64_t getTimeVortexCurrentDepth() const;

    uint64_t getSyncQueueDataSize() const;

    /** Return the checkpoint event */
    CheckpointAction* getCheckpointAction() const { return checkpoint_action_; }


    std::pair<pvt::TimeVortexSort::iterator, pvt::TimeVortexSort::iterator> getEventsForHandler(uintptr_t handler);

    /******** API provided through BaseComponent only ***********/

    /** Register a handler to be called on a set frequency */
    TimeConverter* registerClock(const std::string& freq, Clock::HandlerBase* handler, int priority);

    TimeConverter* registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority);

    TimeConverter* registerClock(TimeConverter* tcFreq, Clock::HandlerBase* handler, int priority);
    TimeConverter* registerClock(TimeConverter& tcFreq, Clock::HandlerBase* handler, int priority);

    // registerClock function used during checkpoint/restart
    void registerClock(SimTime_t factor, Clock::HandlerBase* handler, int priority);

    // Reports that a clock should be present, but doesn't register anything with it
    void reportClock(SimTime_t factor, int priority);

    /** Remove a clock handler from the list of active clock handlers */
    void unregisterClock(TimeConverter* tc, Clock::HandlerBase* handler, int priority);
    void unregisterClock(TimeConverter& tc, Clock::HandlerBase* handler, int priority);

    /** Reactivate an existing clock and handler.
     * @return time when handler will next fire
     */
    Cycle_t reregisterClock(TimeConverter* tc, Clock::HandlerBase* handler, int priority);
    Cycle_t reregisterClock(TimeConverter& tc, Clock::HandlerBase* handler, int priority);

    /** Returns the next Cycle that the TimeConverter would fire. */
    Cycle_t getNextClockCycle(TimeConverter* tc, int priority = CLOCKPRIORITY);
    Cycle_t getNextClockCycle(TimeConverter& tc, int priority = CLOCKPRIORITY);

    /** Gets the clock the handler is registered with, represented by it's factor
     *
     * @param handler Clock handler to search for
     *
     * @return Factor of TimeConverter for the clock the specified
     * handler is registered with.  If the handler is not currently
     * registered with a clock, returns 0.
     */
    SimTime_t getClockForHandler(Clock::HandlerBase* handler);

    /** Return the Statistic Processing Engine associated with this Simulation */
    Statistics::StatisticProcessingEngine* getStatisticsProcessingEngine();


    friend class Link;
    friend class Action;
    friend class Output;
    // To enable main to set up globals
    friend int ::main(int argc, char** argv);

    Simulation_impl(RankInfo my_rank, RankInfo num_ranks, bool restart, SimTime_t currentSimCycle, int currentPriority);
    Simulation_impl(const Simulation_impl&)            = delete; // Don't Implement
    Simulation_impl& operator=(const Simulation_impl&) = delete; // Don't implement

    /** Get a handle to a TimeConverter
     * @param cycles Frequency which is the base of the TimeConverter
     */
    TimeConverter minPartToTC(SimTime_t cycles) const;

    std::string initializeCheckpointInfrastructure(const std::string& prefix);
    void        scheduleCheckpoint();

    /**
       Write the partition specific checkpoint data
     */
    void checkpoint(const std::string& checkpoint_filename);

    /**
       Append partitions registry information
     */
    void checkpoint_append_registry(const std::string& registry_name, const std::string& blob_name);

    /**
       Write the global data to a binary file and create the registry
       and write the header info
     */
    void checkpoint_write_globals(int checkpoint_id, const std::string& checkpoint_filename,
        const std::string& registry_filename, const std::string& globals_filename);
    void restart();

    /**
       Function used to get the rank for a link on restart.  A rank of
       -1 on the return means that the paritioning stayed the same
       between checkpoint and restart and the original rank info
       stored in the checkpoint should be used.
     */
    RankInfo getRankForLinkOnRestart(RankInfo rank, uintptr_t UNUSED(tag))
    {
        if ( serial_restart_ ) return RankInfo(0, 0);
        return RankInfo(rank.rank, rank.thread);
    }

    void initialize_interactive_console(const std::string& type);

    /** Factory used to generate the simulation components */
    static Factory* factory;

    /**
       Filesystem object that will be used to ensure all core-created
       files end up in the directory specified by --output-directory.
     */
    static SST::Util::Filesystem filesystem;

    static void                      resizeBarriers(uint32_t nthr);
    static Core::ThreadSafe::Barrier initBarrier;
    static Core::ThreadSafe::Barrier completeBarrier;
    static Core::ThreadSafe::Barrier setupBarrier;
    static Core::ThreadSafe::Barrier runBarrier;
    static Core::ThreadSafe::Barrier exitBarrier;
    static Core::ThreadSafe::Barrier finishBarrier;
    static std::mutex                simulationMutex;

    static Util::BasicPerfTracker basicPerf;

    // Support for crossthread links
    static Core::ThreadSafe::Spinlock cross_thread_lock;
    static std::map<LinkId_t, Link*>  cross_thread_links;
    bool                              direct_interthread;

    Component* createComponent(ComponentId_t id, const std::string& name, Params& params);

    TimeVortex* getTimeVortex() const { return timeVortex; }

    /** Emergency Shutdown
     * Called when a fatal event has occurred
     */
    static void emergencyShutdown();

    /** Signal Shutdown
     * Called when a signal needs to terminate SST
     * E.g., SIGINT or SIGTERM has been seen
     * abnormal indicates whether this was unexpected or not
     */
    void signalShutdown(bool abnormal);

    /** Normal Shutdown
     */
    void endSimulation();
    void endSimulation(SimTime_t end);

    enum ShutdownMode_t {
        SHUTDOWN_CLEAN,     /* Normal shutdown */
        SHUTDOWN_SIGNAL,    /* SIGINT or SIGTERM received */
        SHUTDOWN_EMERGENCY, /* emergencyShutdown() called */
    };

    friend class SyncManager;

    TimeVortex*             timeVortex = nullptr;
    std::string             timeVortexType;  // Required for checkpoint
    TimeConverter           threadMinPartTC; // Unused...?
    Activity*               current_activity;
    static SimTime_t        minPart;
    static TimeConverter    minPartTC;
    std::vector<SimTime_t>  interThreadLatencies;
    SimTime_t               interThreadMinLatency = MAX_SIMTIME_T;
    SyncManager*            syncManager;
    ComponentInfoMap        compInfoMap;
    clockMap_t              clockMap;
    static Exit*            m_exit;
    SimulatorHeartbeat*     m_heartbeat = nullptr;
    CheckpointAction*       checkpoint_action_;
    static std::string      checkpoint_directory_;
    bool                    endSim = false;
    bool                    independent; // true if no links leave thread (i.e. no syncs required)
    static std::atomic<int> untimed_msg_count;
    unsigned int            untimed_phase;
    volatile sig_atomic_t   signal_arrived_; // true if a signal has arrived
    ShutdownMode_t          shutdown_mode_;
    bool                    wireUpFinished_;
    RealTimeManager*        real_time_;
    std::string             interactive_type_  = "";
    std::string             interactive_start_ = "";
    std::string             replay_file_       = "";
    InteractiveConsole*     interactive_       = nullptr;
    bool                    enter_interactive_ = false;
    std::string             interactive_msg_;
    SimTime_t               stop_at_ = 0;

    // OneShotManager
    Core::OneShotManager one_shot_manager_;

    /**
       vector to hold offsets of component blobs in checkpoint files
     */
    std::vector<std::pair<ComponentId_t, uint64_t>> component_blob_offsets_;

    pvt::TimeVortexSort tv_sort_;

    /** TimeLord of the simulation */
    static TimeLord timeLord;
    /** Output */
    static Output   sim_output;


    /** Statistics Engine */
    SST::Statistics::StatisticProcessingEngine stat_engine;

    /** Performance Tracking Information **/

    void initializeProfileTools(const std::string& config);

    std::map<std::string, SST::Profile::ProfileTool*> profile_tools;
    // Maps the component profile points to profiler names
    std::map<std::string, std::vector<std::string>>   profiler_map;

    template <typename T>
    std::vector<T*> getProfileTool(std::string point)
    {
        std::vector<T*> ret;
        try {
            std::vector<std::string>& profilers = profiler_map.at(point);

            for ( auto& x : profilers ) {
                try {
                    SST::Profile::ProfileTool* val = profile_tools.at(x);

                    T* tool = dynamic_cast<T*>(val);
                    if ( !tool ) {
                        //  Not the right type, fatal
                        Output::getDefaultObject().fatal(CALL_INFO_LONG, 1,
                            "ERROR: wrong type of profiling tool found (name = %s).  Check to make sure the profiling "
                            "points enabled for this tool accept the type specified\n",
                            x.c_str());
                    }
                    ret.push_back(tool);
                }
                catch ( std::out_of_range& e ) {
                    // This shouldn't happen.  If it does, then something
                    // didn't get initialized correctly.
                    Output::getDefaultObject().fatal(CALL_INFO_LONG, 1,
                        "INTERNAL ERROR: ProfileTool refered to in profiler_map not found in profile_tools map\n");
                    return ret;
                }
            }
        }
        catch ( std::out_of_range& e ) {
            // point not turned on, return nullptr
            return ret;
        }


        return ret;
    }


#if SST_PERFORMANCE_INSTRUMENTING
    FILE* fp;
#endif

#if SST_PERIODIC_PRINT
    uint64_t periodicCounter = 0;
#endif

#if SST_RUNTIME_PROFILING
    uint64_t       sumtime = 0;
    uint64_t       runtime = 0;
    struct timeval start, end, diff;
    struct timeval sumstart, sumend, sumdiff;
#endif


#if SST_EVENT_PROFILING
    uint64_t rankLatency     = 0; // Serialization time
    uint64_t messageXferSize = 0;

    uint64_t rankExchangeBytes   = 0; // Serialization size
    uint64_t rankExchangeEvents  = 0; // Serialized events
    uint64_t rankExchangeCounter = 0; // Num rank peer exchanges


    // Profiling functions
    void incrementSerialCounters(uint64_t count);
    void incrementExchangeCounters(uint64_t events, uint64_t bytes);

#endif

#if SST_SYNC_PROFILING
    uint64_t rankSyncCounter   = 0; // Num. of rank syncs
    uint64_t rankSyncTime      = 0; // Total time rank syncing, in ns
    uint64_t threadSyncCounter = 0; // Num. of thread syncs
    uint64_t threadSyncTime    = 0; // Total time thread syncing, in ns
                                    // Does not include thread syncs as part of rank syncs
    void     incrementSyncTime(bool rankSync, uint64_t count);
#endif

#if SST_HIGH_RESOLUTION_CLOCK
    uint64_t    clockDivisor    = 1e9;
    std::string clockResolution = "ns";
#else
    uint64_t    clockDivisor    = 1e6;
    std::string clockResolution = "us";
#endif

    // Run mode for the simulation
    SimulationRunMode runMode;

    // Track current simulated time
    SimTime_t currentSimCycle = 0;
    int       currentPriority = 0;
    SimTime_t endSimCycle     = 0;

    // Rank information
    RankInfo my_rank;
    RankInfo num_ranks;

    std::string output_directory;

    double run_phase_start_time_;
    double run_phase_total_time_;
    double init_phase_start_time_;
    double init_phase_total_time_;
    double complete_phase_start_time_;
    double complete_phase_total_time_;

    static std::unordered_map<std::thread::id, Simulation_impl*> instanceMap;
    static std::vector<Simulation_impl*>                         instanceVec_;

    /******** Checkpoint/restart tracking data structures ***********/
    std::map<std::pair<int, uintptr_t>, Link*> link_restart_tracking;
    std::map<uintptr_t, uintptr_t>             event_handler_restart_tracking;
    uint32_t                                   checkpoint_id_       = 0;
    std::string                                checkpoint_prefix_   = "";
    std::string                                globalOutputFileName = "";
    bool                                       serial_restart_      = false;

    // Config object used by the simulation
    static Config       config;
    static StatsConfig* stats_config_;

    void printSimulationState();

    friend void wait_my_turn_start();
    friend void wait_my_turn_end();

private:
    /**
     * Returns the time of the next item to be executed
     * that is in the TImeVortex of the Simulation
     */
    SimTime_t getNextActivityTime() const;
};

// Function to allow for easy serialization of threads while debugging
// code.  Can only be used when you can guarantee all threads will be
// taking the same code path.  If you can't guarantee that, then use a
// spinlock to make sure only one person is in a given region at a
// time.  ONLY FOR DEBUG USE.
void wait_my_turn_start(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads);

void wait_my_turn_end(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads);

} // namespace SST

#endif // SST_CORE_SIMULATION_IMPL_H
