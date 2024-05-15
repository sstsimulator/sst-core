// -*- c++ -*-

// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SIMULATION_IMPL_H
#define SST_CORE_SIMULATION_IMPL_H

#include "sst/core/clock.h"
#include "sst/core/componentInfo.h"
#include "sst/core/oneshot.h"
#include "sst/core/output.h"
#include "sst/core/profile/profiletool.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statengine.h"
#include "sst/core/unitAlgebra.h"

#include <atomic>
#include <cstdio>
#include <iostream>
#include <signal.h>
#include <thread>
#include <unordered_map>

/* Forward declare for Friendship */
extern int main(int argc, char** argv);

namespace SST {

#define _SIM_DBG(fmt, args...) __DBG(DBG_SIM, Sim, fmt, ##args)
#define STATALLFLAG            "--ALLSTATS--"

class Activity;
class CheckpointAction;
class Component;
class Config;
class ConfigGraph;
class Exit;
class Factory;
class Link;
class LinkMap;
class Params;
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

/**
 * Main control class for a SST Simulation.
 * Provides base features for managing the simulation
 */
class Simulation_impl
{

public:
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

    typedef std::map<std::pair<SimTime_t, int>, Clock*>   clockMap_t;   /*!< Map of times to clocks */
    typedef std::map<std::pair<SimTime_t, int>, OneShot*> oneShotMap_t; /*!< Map of times to OneShots */

    ~Simulation_impl();

    /*********  Static Core-only Functions *********/

    /** Return a pointer to the singleton instance of the Simulation */
    static Simulation_impl* getSimulation() { return instanceMap.at(std::this_thread::get_id()); }

    /** Return the TimeLord associated with this Simulation */
    static TimeLord* getTimeLord(void) { return &timeLord; }

    /** Return the base simulation Output class instance */
    static Output& getSimulationOutput() { return sim_output; }

    /** Create new simulation
     * @param config - Configuration of the simulation
     * @param my_rank - Parallel Rank of this simulation object
     * @param num_ranks - How many Ranks are in the simulation
     */
    static Simulation_impl* createSimulation(Config* config, RankInfo my_rank, RankInfo num_ranks);

    /**
     * Used to signify the end of simulation.  Cleans up any existing Simulation Objects
     */
    static void shutdown();

    /** Sets an internal flag for signaling the simulation.  Used internally */
    static void setSignal(int signal);

    /** Insert an activity to fire at a specified time */
    void insertActivity(SimTime_t time, Activity* ev);

    /** Return the exit event */
    Exit* getExit() const { return m_exit; }

    /******** Core only API *************/

    /** Processes the ConfigGraph to pull out any need information
     * about relationships among the threads
     */
    void processGraphInfo(ConfigGraph& graph, const RankInfo& myRank, SimTime_t min_part);

    int  initializeStatisticEngine(ConfigGraph& graph);
    int  prepareLinks(ConfigGraph& graph, const RankInfo& myRank, SimTime_t min_part);
    int  performWireUp(ConfigGraph& graph, const RankInfo& myRank, SimTime_t min_part);
    void exchangeLinkInfo();

    /** Set cycle count, which, if reached, will cause the simulation to halt. */
    void setStopAtCycle(Config* cfg);

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
    TimeConverter* registerOneShot(const std::string& timeDelay, OneShot::HandlerBase* handler, int priority);

    TimeConverter* registerOneShot(const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler, int priority);

    const std::vector<SimTime_t>& getInterThreadLatencies() const { return interThreadLatencies; }

    SimTime_t getInterThreadMinLatency() const { return interThreadMinLatency; }

    static TimeConverter* getMinPartTC() { return minPartTC; }

    LinkMap* getComponentLinkMap(ComponentId_t id) const
    {
        ComponentInfo* info = compInfoMap.getByID(id);
        if ( nullptr == info ) { return nullptr; }
        else {
            return info->getLinkMap();
        }
    }

    /** Returns reference to the Component Info Map */
    const ComponentInfoMap& getComponentInfoMap(void) { return compInfoMap; }

    /** returns the component with the given ID */
    BaseComponent* getComponent(const ComponentId_t& id) const
    {
        ComponentInfo* i = compInfoMap.getByID(id);
        // CompInfoMap_t::const_iterator i = compInfoMap.find(id);
        if ( nullptr != i ) { return i->getComponent(); }
        else {
            printf("Simulation::getComponent() couldn't find component with id = %" PRIu64 "\n", id);
            exit(1);
        }
    }

    /** returns the ComponentInfo object for the given ID */
    ComponentInfo* getComponentInfo(const ComponentId_t& id) const
    {
        ComponentInfo* i = compInfoMap.getByID(id);
        // CompInfoMap_t::const_iterator i = compInfoMap.find(id);
        if ( nullptr != i ) { return i; }
        else {
            printf("Simulation::getComponentInfo() couldn't find component with id = %" PRIu64 "\n", id);
            exit(1);
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
    bool isWireUpFinished() { return wireUpFinished; }

    uint64_t getTimeVortexMaxDepth() const;

    uint64_t getTimeVortexCurrentDepth() const;

    uint64_t getSyncQueueDataSize() const;

    /******** API provided through BaseComponent only ***********/

    /** Register a handler to be called on a set frequency */
    TimeConverter* registerClock(const std::string& freq, Clock::HandlerBase* handler, int priority);

    TimeConverter* registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority);

    TimeConverter* registerClock(TimeConverter* tcFreq, Clock::HandlerBase* handler, int priority);

    // registerClock function used during checkpoint/restart
    void registerClock(SimTime_t factor, Clock::HandlerBase* handler, int priority);

    /** Remove a clock handler from the list of active clock handlers */
    void unregisterClock(TimeConverter* tc, Clock::HandlerBase* handler, int priority);

    /** Reactivate an existing clock and handler.
     * @return time when handler will next fire
     */
    Cycle_t reregisterClock(TimeConverter* tc, Clock::HandlerBase* handler, int priority);

    /** Returns the next Cycle that the TImeConverter would fire. */
    Cycle_t getNextClockCycle(TimeConverter* tc, int priority = CLOCKPRIORITY);

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
    Statistics::StatisticProcessingEngine* getStatisticsProcessingEngine(void);


    friend class Link;
    friend class Action;
    friend class Output;
    // To enable main to set up globals
    friend int ::main(int argc, char** argv);

    Simulation_impl() {}
    Simulation_impl(Config* config, RankInfo my_rank, RankInfo num_ranks);
    Simulation_impl(Simulation_impl const&); // Don't Implement
    void operator=(Simulation_impl const&);  // Don't implement

    /** Get a handle to a TimeConverter
     * @param cycles Frequency which is the base of the TimeConverter
     */
    TimeConverter* minPartToTC(SimTime_t cycles) const;

    void checkpoint();
    void restart(Config* config);

    /** Factory used to generate the simulation components */
    static Factory* factory;

    static void                      resizeBarriers(uint32_t nthr);
    static Core::ThreadSafe::Barrier initBarrier;
    static Core::ThreadSafe::Barrier completeBarrier;
    static Core::ThreadSafe::Barrier setupBarrier;
    static Core::ThreadSafe::Barrier runBarrier;
    static Core::ThreadSafe::Barrier exitBarrier;
    static Core::ThreadSafe::Barrier finishBarrier;
    static std::mutex                simulationMutex;

    // Support for crossthread links
    static Core::ThreadSafe::Spinlock cross_thread_lock;
    static std::map<LinkId_t, Link*>  cross_thread_links;
    bool                              direct_interthread;

    Component* createComponent(ComponentId_t id, const std::string& name, Params& params);

    TimeVortex* getTimeVortex() const { return timeVortex; }

    /** Emergency Shutdown
     * Called when a SIGINT or SIGTERM has been seen
     */
    static void emergencyShutdown();
    /** Normal Shutdown
     */
    void        endSimulation(void);
    void        endSimulation(SimTime_t end);

    typedef enum {
        SHUTDOWN_CLEAN,     /* Normal shutdown */
        SHUTDOWN_SIGNAL,    /* SIGINT or SIGTERM received */
        SHUTDOWN_EMERGENCY, /* emergencyShutdown() called */
    } ShutdownMode_t;

    friend class SyncManager;

    TimeVortex*             timeVortex;
    std::string             timeVortexType;
    TimeConverter*          threadMinPartTC; // Unused...?
    Activity*               current_activity;
    static SimTime_t        minPart;
    static TimeConverter*   minPartTC;
    std::vector<SimTime_t>  interThreadLatencies;
    SimTime_t               interThreadMinLatency;
    SyncManager*            syncManager;
    // ThreadSync*      threadSync;
    ComponentInfoMap        compInfoMap;
    clockMap_t              clockMap;
    oneShotMap_t            oneShotMap;
    static Exit*            m_exit;
    SimulatorHeartbeat*     m_heartbeat = nullptr;
    bool                    endSim;
    bool                    independent; // true if no links leave thread (i.e. no syncs required)
    static std::atomic<int> untimed_msg_count;
    unsigned int            untimed_phase;
    volatile sig_atomic_t   lastRecvdSignal;
    ShutdownMode_t          shutdown_mode;
    bool                    wireUpFinished;

    /** TimeLord of the simulation */
    static TimeLord timeLord;
    /** Output */
    static Output   sim_output;

    /** Statistics Engine */
    SST::Statistics::StatisticProcessingEngine stat_engine;

    /** Performance Tracking Information **/

    void intializeProfileTools(const std::string& config);

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
                        Output::getDefaultObject().fatal(
                            CALL_INFO_LONG, 1,
                            "ERROR: wrong type of profiling tool found (name = %s).  Check to make sure the profiling "
                            "points enabled for this tool accept the type specified\n",
                            x.c_str());
                    }
                    ret.push_back(tool);
                }
                catch ( std::out_of_range& e ) {
                    // This shouldn't happen.  If it does, then something
                    // didn't get initialized correctly.
                    Output::getDefaultObject().fatal(
                        CALL_INFO_LONG, 1,
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
    SimTime_t currentSimCycle;
    int       currentPriority;
    SimTime_t endSimCycle;

    // Rank information
    RankInfo my_rank;
    RankInfo num_ranks;

    std::string output_directory;

    double run_phase_start_time;
    double run_phase_total_time;
    double init_phase_start_time;
    double init_phase_total_time;
    double complete_phase_start_time;
    double complete_phase_total_time;

    static std::unordered_map<std::thread::id, Simulation_impl*> instanceMap;
    static std::vector<Simulation_impl*>                         instanceVec;

    /******** Checkpoint/restart tracking data structures ***********/
    std::map<uintptr_t, Link*>     link_restart_tracking;
    std::map<uintptr_t, uintptr_t> event_handler_restart_tracking;
    CheckpointAction*              m_checkpoint         = nullptr;
    uint32_t                       checkpoint_id        = 0;
    std::string                    checkpointPrefix     = "";
    std::string                    globalOutputFileName = "";

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
