// -*- c++ -*-

// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SIMULATION_H
#define SST_CORE_SIMULATION_H


#include "sst/core/sst_types.h"

#include <signal.h>
#include <atomic>
#include <iostream>
#include <thread>

#include <unordered_map>

#include "sst/core/output.h"
#include "sst/core/clock.h"
#include "sst/core/oneshot.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/rankInfo.h"

#include "sst/core/componentInfo.h"

/* Forward declare for Friendship */
extern int main(int argc, char **argv);


namespace SST {

#define _SIM_DBG( fmt, args...) __DBG( DBG_SIM, Sim, fmt, ## args )
#define STATALLFLAG "--ALLSTATS--"

class Activity;
class Component;
class Config;
class ConfigGraph;
class Exit;
class Factory;
class SimulatorHeartbeat;
//class Graph;
class LinkMap;
class Params;
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
}



/**
 * Main control class for a SST Simulation.
 * Provides base features for managing the simulation
 */
class Simulation {
public:
    /** Type of Run Modes */
    typedef enum {
        UNKNOWN,    /*!< Unknown mode - Invalid for running */
        INIT,       /*!< Initialize-only.  Useful for debugging initialization and graph generation */
        RUN,        /*!< Run-only.  Useful when restoring from a checkpoint */
        BOTH        /*!< Default.  Both initialize and Run the simulation */
    } Mode_t;

    typedef std::map<std::pair<SimTime_t, int>, Clock*>   clockMap_t;              /*!< Map of times to clocks */
    typedef std::map<std::pair<SimTime_t, int>, OneShot*> oneShotMap_t;            /*!< Map of times to OneShots */
    // typedef std::map< unsigned int, Sync* > SyncMap_t; /*!< Map of times to Sync Objects */

    ~Simulation();

    /*********  Static Core-only Functions *********/

    /** Create new simulation
     * @param config - Configuration of the simulation
     * @param my_rank - Parallel Rank of this simulation object
     * @param num_ranks - How many Ranks are in the simulation
     */
#if !SST_BUILDING_CORE
    static Simulation *createSimulation(Config *config, RankInfo my_rank, RankInfo num_ranks, SimTime_t min_part) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    static Simulation *createSimulation(Config *config, RankInfo my_rank, RankInfo num_ranks, SimTime_t min_part);
#endif

    /**
     * Used to signify the end of simulation.  Cleans up any existing Simulation Objects
     */
#if !SST_BUILDING_CORE
    static void shutdown() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    static void shutdown();
#endif
    
    /** Sets an internal flag for signaling the simulation.  Used internally */
#if !SST_BUILDING_CORE
    static void setSignal(int signal) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    static void setSignal(int signal);
#endif

    
    /********* Public Static API ************/

    /** Return a pointer to the singleton instance of the Simulation */
    static Simulation *getSimulation() { return instanceMap.at(std::this_thread::get_id()); }

    /**
     * Returns the Simulation's SharedRegionManager
     */
    static SharedRegionManager* getSharedRegionManager() { return sharedRegionManager; }

    /** Return the TimeLord associated with this Simulation */
    static TimeLord* getTimeLord(void) { return &timeLord; }

    /** Return the base simulation Output class instance */
    static Output& getSimulationOutput() { return sim_output; };



    /********* Public API ************/
    /** Get the run mode of the simulation (e.g. init, run, both etc) */
    Mode_t getSimulationMode() const { return runMode; };

    /** Return the current simulation time as a cycle count*/
    const SimTime_t& getCurrentSimCycle() const;

    /** Return the end simulation time as a cycle count*/
    SimTime_t getEndSimCycle() const;

    /** Return the current priority */
    int getCurrentPriority() const;

    /** Return the elapsed simulation time as a time */
    UnitAlgebra getElapsedSimTime() const;

    /** Return the end simulation time as a time */
    UnitAlgebra getFinalSimTime() const;

    /** Get this instance's parallel rank */
    RankInfo getRank() const {return my_rank;}

    /** Get the number of parallel ranks in the simulation */
    RankInfo getNumRanks() const {return num_ranks;}

    /** Insert an activity to fire at a specified time */
    void insertActivity(SimTime_t time, Activity* ev);

    /** Return the exit event */
    Exit* getExit() const { return m_exit; }


    /** Signifies that an event type is required for this simulation
     *  Causes the Factory to verify that the required event type can be found.
     *  @param name fully qualified libraryName.EventName
     */
    void requireEvent(const std::string& name);


    /** Causes the current status of the simulation to be printed to stderr.
     * @param fullStatus - if true, call printStatus() on all components as well
     *        as print the base Simulation's status
     */
    void printStatus(bool fullStatus);

    
    /******** Core only API *************/
    
    /** Processes the ConfigGraph to pull out any need information
     * about relationships among the threads
     */
#if !SST_BUILDING_CORE
    void processGraphInfo( ConfigGraph& graph, const RankInfo &myRank, SimTime_t min_part ) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    void processGraphInfo( ConfigGraph& graph, const RankInfo &myRank, SimTime_t min_part );
#endif    

    /** Converts a ConfigGraph graph into actual set of links and components */
#if !SST_BUILDING_CORE
    int performWireUp( ConfigGraph& graph, const RankInfo &myRank, SimTime_t min_part ) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    int performWireUp( ConfigGraph& graph, const RankInfo &myRank, SimTime_t min_part );
#endif
    
    /** Set cycle count, which, if reached, will cause the simulation to halt. */
#if !SST_BUILDING_CORE
    void setStopAtCycle( Config* cfg ) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    void setStopAtCycle( Config* cfg );
#endif
    
    /** Perform the init() phase of simulation */
#if !SST_BUILDING_CORE
    void initialize() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    void initialize();
#endif
    
    /** Perform the complete() phase of simulation */
#if !SST_BUILDING_CORE
    void complete() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    void complete();
#endif
    
    /** Perform the setup() and run phases of the simulation. */
#if !SST_BUILDING_CORE
    void setup() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    void setup();
#endif
    
#if !SST_BUILDING_CORE
    void run() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    void run();
#endif
    
#if !SST_BUILDING_CORE
    void finish() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    void finish();
#endif

#if !SST_BUILDING_CORE
    bool isIndependentThread() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) { return independent;}
#else
    bool isIndependentThread() { return independent;}
#endif

    /** Register a OneShot event to be called after a time delay
        Note: OneShot cannot be canceled, and will always callback after
              the timedelay.
    */
#if !SST_BUILDING_CORE
    TimeConverter* registerOneShot(const std::string& timeDelay, OneShot::HandlerBase* handler, int priority) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    TimeConverter* registerOneShot(const std::string& timeDelay, OneShot::HandlerBase* handler, int priority);
#endif
    
#if !SST_BUILDING_CORE
    TimeConverter* registerOneShot(const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler, int priority) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    TimeConverter* registerOneShot(const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler, int priority);
#endif

#if !SST_BUILDING_CORE
    const std::vector<SimTime_t>& getInterThreadLatencies() const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) { return interThreadLatencies; }
#else
    const std::vector<SimTime_t>& getInterThreadLatencies() const { return interThreadLatencies; }
#endif

#if !SST_BUILDING_CORE
    SimTime_t getInterThreadMinLatency() const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")))  { return interThreadMinLatency; }
#else
    SimTime_t getInterThreadMinLatency() const { return interThreadMinLatency; }
#endif

#if !SST_BUILDING_CORE
    static TimeConverter* getMinPartTC() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) { return minPartTC; }
#else
    static TimeConverter* getMinPartTC() { return minPartTC; }
#endif
    
    /** Return pointer to map of links for a given component id */
#if !SST_BUILDING_CORE
    LinkMap* getComponentLinkMap(ComponentId_t id) const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) {
#else
    LinkMap* getComponentLinkMap(ComponentId_t id) const {
#endif
        ComponentInfo* info = compInfoMap.getByID(id);
        if ( nullptr == info ) {
            return nullptr;
        } else {
            return info->getLinkMap();
        }
    }

    /** Returns reference to the Component Info Map */
#if !SST_BUILDING_CORE
    const ComponentInfoMap& getComponentInfoMap(void) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) { return compInfoMap; }
#else
    const ComponentInfoMap& getComponentInfoMap(void) { return compInfoMap; }
#endif   

    /** returns the component with the given ID */
#if !SST_BUILDING_CORE
    BaseComponent* getComponent(const ComponentId_t &id) const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) {
#else
    BaseComponent* getComponent(const ComponentId_t &id) const {
#endif
        ComponentInfo* i = compInfoMap.getByID(id);
        // CompInfoMap_t::const_iterator i = compInfoMap.find(id);
        if ( nullptr != i ) {
            return i->getComponent();
        } else {
            printf("Simulation::getComponent() couldn't find component with id = %" PRIu64 "\n", id);
            exit(1);
        }
    }


    /** returns the ComponentInfo object for the given ID */
#if !SST_BUILDING_CORE
    ComponentInfo* getComponentInfo(const ComponentId_t &id) const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) {
#else
    ComponentInfo* getComponentInfo(const ComponentId_t &id) const {
#endif
        ComponentInfo* i = compInfoMap.getByID(id);
        // CompInfoMap_t::const_iterator i = compInfoMap.find(id);
        if ( nullptr != i ) {
            return i;
        } else {
            printf("Simulation::getComponentInfo() couldn't find component with id = %" PRIu64 "\n", id);
            exit(1);
        }
    }


    /**
    Set the output directory for this simulation
    @param outDir Path of directory to place simulation outputs in
    */
#if !SST_BUILDING_CORE
    void setOutputDirectory(const std::string& outDir) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) {
#else
    void setOutputDirectory(const std::string& outDir) {
#endif
        output_directory = outDir;
    }

    /**
    Returns the output directory of the simulation
    @return Directory in which simulation outputs are placed
    */
#if !SST_BUILDING_CORE
    std::string& getOutputDirectory() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) {
#else
    std::string& getOutputDirectory() {
#endif
        return output_directory;
    }

    /**
     * Returns the time of the next item to be executed
     * that is in the TImeVortex of the Simulation
     */
#if !SST_BUILDING_CORE
    SimTime_t getNextActivityTime() const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    SimTime_t getNextActivityTime() const;
#endif
    
    /**
     *  Gets the minimum next activity time across all TimeVortices in
     *  the Rank
     */
#if !SST_BUILDING_CORE
    static SimTime_t getLocalMinimumNextActivityTime() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    static SimTime_t getLocalMinimumNextActivityTime();
#endif
    
    /**
    * Returns true when the Wireup is finished.
    */
#if !SST_BUILDING_CORE
    bool isWireUpFinished() __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11."))) {return wireUpFinished; }
#else
    bool isWireUpFinished() {return wireUpFinished; }
#endif
    


#if !SST_BUILDING_CORE
    uint64_t getTimeVortexMaxDepth() const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    uint64_t getTimeVortexMaxDepth() const;
#endif

#if !SST_BUILDING_CORE
    uint64_t getTimeVortexCurrentDepth() const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    uint64_t getTimeVortexCurrentDepth() const;
#endif

#if !SST_BUILDING_CORE
    uint64_t getSyncQueueDataSize() const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    uint64_t getSyncQueueDataSize() const;
#endif
    
    
    /******** API provided through BaseComponent only ***********/

    
    /** Register a handler to be called on a set frequency */
#if !SST_BUILDING_CORE
    TimeConverter* registerClock(const std::string& freq, Clock::HandlerBase* handler, int priority) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    TimeConverter* registerClock(const std::string& freq, Clock::HandlerBase* handler, int priority);
#endif
    
#if !SST_BUILDING_CORE
    TimeConverter* registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    TimeConverter* registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority);
#endif
    
#if !SST_BUILDING_CORE
    TimeConverter* registerClock(TimeConverter *tcFreq, Clock::HandlerBase* handler, int priority) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    TimeConverter* registerClock(TimeConverter *tcFreq, Clock::HandlerBase* handler, int priority);
#endif
    
    /** Remove a clock handler from the list of active clock handlers */
#if !SST_BUILDING_CORE
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler, int priority) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler, int priority);
#endif
    
    /** Reactivate an existing clock and handler.
     * @return time when handler will next fire
     */
#if !SST_BUILDING_CORE
    Cycle_t reregisterClock(TimeConverter *tc, Clock::HandlerBase* handler, int priority) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    Cycle_t reregisterClock(TimeConverter *tc, Clock::HandlerBase* handler, int priority);
#endif
    
    /** Returns the next Cycle that the TImeConverter would fire. */
#if !SST_BUILDING_CORE
    Cycle_t getNextClockCycle(TimeConverter* tc, int priority = CLOCKPRIORITY) __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    Cycle_t getNextClockCycle(TimeConverter* tc, int priority = CLOCKPRIORITY);
#endif

    /** Return the Statistic Processing Engine associated with this Simulation */
#if !SST_BUILDING_CORE
    Statistics::StatisticProcessingEngine* getStatisticsProcessingEngine(void) const __attribute__ ((deprecated("this function was not intended to be used outside of SST core and will be removed in SST 11.")));
#else
    Statistics::StatisticProcessingEngine* getStatisticsProcessingEngine(void) const;
#endif
    



private:
    friend class Link;
    friend class Action;
    friend class Output;
    // To enable main to set up globals
    friend int ::main(int argc, char **argv);

    Simulation(); // Don't call.  Only rational way to serialize
    Simulation(Config* config, RankInfo my_rank, RankInfo num_ranks, SimTime_t min_part);
    Simulation(Simulation const&);     // Don't Implement
    void operator=(Simulation const&); // Don't implement


    /** Get a handle to a TimeConverter
     * @param cycles Frequency which is the base of the TimeConverter
     */
    TimeConverter* minPartToTC(SimTime_t cycles) const;

    /** Factory used to generate the simulation components */
    static Factory *factory;
    /** TimeLord of the simulation */
    static TimeLord timeLord;
    /** Output */
    static Output sim_output;
    static void resizeBarriers(uint32_t nthr);
    static Core::ThreadSafe::Barrier initBarrier;
    static Core::ThreadSafe::Barrier completeBarrier;
    static Core::ThreadSafe::Barrier setupBarrier;
    static Core::ThreadSafe::Barrier runBarrier;
    static Core::ThreadSafe::Barrier exitBarrier;
    static Core::ThreadSafe::Barrier finishBarrier;
    static std::mutex simulationMutex;



    Component* createComponent(ComponentId_t id, const std::string& name,
                               Params &params);

    TimeVortex* getTimeVortex() const { return timeVortex; }

    /** Emergency Shutdown
     * Called when a SIGINT or SIGTERM has been seen
     */
    static void emergencyShutdown();
    /** Normal Shutdown
     */
    void endSimulation(void) { endSimulation(currentSimCycle); }
    void endSimulation(SimTime_t end);

    typedef enum {
        SHUTDOWN_CLEAN,     /* Normal shutdown */
        SHUTDOWN_SIGNAL,    /* SIGINT or SIGTERM received */
        SHUTDOWN_EMERGENCY, /* emergencyShutdown() called */
    } ShutdownMode_t;

    friend class SyncManager;

    Mode_t   runMode;
    TimeVortex*      timeVortex;
    TimeConverter*   threadMinPartTC;
    Activity*        current_activity;
    static SimTime_t minPart;
    static TimeConverter*   minPartTC;
    std::vector<SimTime_t> interThreadLatencies;
    SimTime_t        interThreadMinLatency;
    SyncManager*     syncManager;
    ThreadSync*      threadSync;
    ComponentInfoMap compInfoMap;
    clockMap_t       clockMap;
    oneShotMap_t     oneShotMap;
    SimTime_t        currentSimCycle;
    SimTime_t        endSimCycle;
    int              currentPriority;
    static Exit*     m_exit;
    SimulatorHeartbeat*    m_heartbeat;
    bool             endSim;
    RankInfo         my_rank;
    RankInfo         num_ranks;
    bool             independent; // true if no links leave thread (i.e. no syncs required)
    static std::atomic<int>       untimed_msg_count;
    unsigned int     untimed_phase;
    volatile sig_atomic_t lastRecvdSignal;
    ShutdownMode_t   shutdown_mode;
    std::string      output_directory;
    static SharedRegionManager* sharedRegionManager;
    bool             wireUpFinished;

    static std::unordered_map<std::thread::id, Simulation*> instanceMap;
    static std::vector<Simulation*> instanceVec;

    friend void wait_my_turn_start();
    friend void wait_my_turn_end();

};

// Function to allow for easy serialization of threads while debugging
// code.  Can only be used when you can guarantee all threads will be
// taking the same code path.  If you can't guarantee that, then use a
// spinlock to make sure only one person is in a given region at a
// time.  ONLY FOR DEBUG USE.
void wait_my_turn_start(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads);

void wait_my_turn_end(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads);


} // namespace SST

#endif //SST_CORE_SIMULATION_H
