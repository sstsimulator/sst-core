// -*- c++ -*-

// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
#include "sst/core/rankInfo.h"
#include "sst/core/simulation.h"
#include "sst/core/sst_types.h"
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
class Component;
class Config;
class ConfigGraph;
class Exit;
class Factory;
class SimulatorHeartbeat;
class Link;
class LinkMap;
class Params;
class SharedRegionManager;
class SimulatorHeartbeat;
class SyncBase;
class SyncManager;
class ThreadSync;
class TimeConverter;
class TimeLord;
class TimeVortex;
class UnitAlgebra;
class SharedRegionManager;
namespace Statistics {
class StatisticOutput;
class StatisticProcessingEngine;
} // namespace Statistics

namespace Statistics {
class StatisticOutput;
class StatisticProcessingEngine;
} // namespace Statistics

/**
 * Main control class for a SST Simulation.
 * Provides base features for managing the simulation
 */
class Simulation_impl : public Simulation
{

public:
    /********  Public API inherited from Simulation ********/
    /** Get the run mode of the simulation (e.g. init, run, both etc) */
    Mode_t getSimulationMode() const override { return runMode; };

    /** Return the current simulation time as a cycle count*/
    SimTime_t getCurrentSimCycle() const override;

    /** Return the end simulation time as a cycle count*/
    SimTime_t getEndSimCycle() const override;

    /** Return the current priority */
    int getCurrentPriority() const override;

    /** Return the elapsed simulation time as a time */
    UnitAlgebra getElapsedSimTime() const override;

    /** Return the end simulation time as a time */
    UnitAlgebra getEndSimTime() const override;

    /** Return the end simulation time as a time */
    UnitAlgebra getFinalSimTime() const override;

    /** Get this instance's parallel rank */
    RankInfo getRank() const override { return my_rank; }

    /** Get the number of parallel ranks in the simulation */
    RankInfo getNumRanks() const override { return num_ranks; }

    /**
    Returns the output directory of the simulation
    @return Directory in which simulation outputs are placed
    */
    std::string& getOutputDirectory() override { return output_directory; }

    /** Signifies that an event type is required for this simulation
     *  Causes the Factory to verify that the required event type can be found.
     *  @param name fully qualified libraryName.EventName
     */
    virtual void requireEvent(const std::string& name) override;

    /** Signifies that a library is required for this simulation.
     *  @param name Name of the library
     */
    virtual void requireLibrary(const std::string& name) override;

    /** Causes the current status of the simulation to be printed to stderr.
     * @param fullStatus - if true, call printStatus() on all components as well
     *        as print the base Simulation's status
     */
    virtual void printStatus(bool fullStatus) override;

    virtual double getRunPhaseElapsedRealTime() const override;
    virtual double getInitPhaseElapsedRealTime() const override;
    virtual double getCompletePhaseElapsedRealTime() const override;

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

    void run();

    void finish();

    bool isIndependentThread() { return independent; }

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

    void registerClockHandler(SST::ComponentId_t id, HandlerId_t handler);

    /** Remove a clock handler from the list of active clock handlers */
    void unregisterClock(TimeConverter* tc, Clock::HandlerBase* handler, int priority);

    /** Reactivate an existing clock and handler.
     * @return time when handler will next fire
     */
    Cycle_t reregisterClock(TimeConverter* tc, Clock::HandlerBase* handler, int priority);

    /** Returns the next Cycle that the TImeConverter would fire. */
    Cycle_t getNextClockCycle(TimeConverter* tc, int priority = CLOCKPRIORITY);

    /** Return the Statistic Processing Engine associated with this Simulation */
    Statistics::StatisticProcessingEngine* getStatisticsProcessingEngine(void) const;

    // private:

    friend class Link;
    friend class Action;
    friend class Output;
    // To enable main to set up globals
    friend int ::main(int argc, char** argv);

    // Simulation_impl() {}
    Simulation_impl(Config* config, RankInfo my_rank, RankInfo num_ranks);
    Simulation_impl(Simulation_impl const&); // Don't Implement
    void operator=(Simulation_impl const&);  // Don't implement

    /** Get a handle to a TimeConverter
     * @param cycles Frequency which is the base of the TimeConverter
     */
    TimeConverter* minPartToTC(SimTime_t cycles) const;

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

    static std::map<LinkId_t, Link*> cross_thread_links;
    bool                             direct_interthread;

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
    TimeConverter*          threadMinPartTC;
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
    SimulatorHeartbeat*     m_heartbeat;
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

    /** Performance Tracking Information **/

#if SST_PERFORMANCE_INSTRUMENTING
    FILE*                                          fp;
    std::map<SST::HandlerId_t, SST::ComponentId_t> handler_mapping;
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

#if SST_CLOCK_PROFILING
    std::map<SST::HandlerId_t, uint64_t> clockHandlers;
    std::map<SST::HandlerId_t, uint64_t> clockCounters;
#endif

#if SST_EVENT_PROFILING
    uint64_t                        rankLatency         = 0;
    uint64_t                        rankExchangeCounter = 0;
    std::map<std::string, uint64_t> eventHandlers;
    std::map<std::string, uint64_t> eventRecvCounters;
    std::map<std::string, uint64_t> eventSendCounters;
    uint64_t                        messageXferSize = 0;
#endif

#if SST_SYNC_PROFILING
    uint64_t syncCounter     = 0;
    uint64_t rankSyncTime    = 0;
    uint64_t threadSyncTime  = 0;
    uint64_t rankSyncCounter = 0;
#endif

#if SST_HIGH_RESOLUTION_CLOCK
    uint64_t    clockDivisor    = 1e9;
    std::string clockResolution = "ns";
#else
    uint64_t    clockDivisor    = 1e6;
    std::string clockResolution = "us";
#endif

    Mode_t    runMode;
    SimTime_t currentSimCycle;
    SimTime_t endSimCycle;
    int       currentPriority;
    RankInfo  my_rank;
    RankInfo  num_ranks;

    std::string                 output_directory;
    static SharedRegionManager* sharedRegionManager;

    double run_phase_start_time;
    double run_phase_total_time;
    double init_phase_start_time;
    double init_phase_total_time;
    double complete_phase_start_time;
    double complete_phase_total_time;

    static std::unordered_map<std::thread::id, Simulation_impl*> instanceMap;
    static std::vector<Simulation_impl*>                         instanceVec;

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
