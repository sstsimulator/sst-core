// -*- c++ -*-

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

#ifndef SST_CORE_SIMULATION_H
#define SST_CORE_SIMULATION_H

#include "sst/core/sst_types.h"

#include <signal.h>
#include <atomic>
#include <iostream>
#include <thread>

#include <unordered_map>

#include <sst/core/output.h>
#include <sst/core/clock.h>
#include <sst/core/oneshot.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/rankInfo.h>

#include <sst/core/componentInfo.h>

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
class Introspector;
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


typedef std::map<std::string, Introspector* > IntroMap_t;


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

    typedef std::map<SimTime_t, Clock*>   clockMap_t;              /*!< Map of times to clocks */
    typedef std::map<SimTime_t, OneShot*> oneShotMap_t;            /*!< Map of times to OneShots */
    typedef std::vector<std::string>      statEnableList_t;        /*!< List of Enabled Statistics */
    typedef std::vector<Params>           statParamsList_t;        /*!< List of Enabled Statistics Parameters */
    typedef std::map<ComponentId_t, statEnableList_t*> statEnableMap_t;  /*!< Map of Statistics that are requested to be enabled for a component defined in configGraph */
    typedef std::map<ComponentId_t, statParamsList_t*> statParamsMap_t;  /*!< Map of Statistic Params for a component defined in configGraph */
    // typedef std::map< unsigned int, Sync* > SyncMap_t; /*!< Map of times to Sync Objects */

    ~Simulation();

    /** Create new simulation
     * @param config - Configuration of the simulation
     * @param my_rank - Parallel Rank of this simulation object
     * @param num_ranks - How many Ranks are in the simulation
     */
    static Simulation *createSimulation(Config *config, RankInfo my_rank, RankInfo num_ranks, SimTime_t min_part);
    /**
     * Used to signify the end of simulation.  Cleans up any existing Simulation Objects
     */
    static void shutdown();
    /** Return a pointer to the singleton instance of the Simulation */
    static Simulation *getSimulation() { return instanceMap.at(std::this_thread::get_id()); }
    /** Sets an internal flag for signaling the simulation.  Used internally */
    static void setSignal(int signal);
    /** Causes the current status of the simulation to be printed to stderr.
     * @param fullStatus - if true, call printStatus() on all components as well
     *        as print the base Simulation's status
     */
    void printStatus(bool fullStatus);

    /** Processes the ConfigGraph to pull out any need information
     * about relationships among the threads
     */
    void processGraphInfo( ConfigGraph& graph, const RankInfo &myRank, SimTime_t min_part );

    /** Converts a ConfigGraph graph into actual set of links and components */
    int performWireUp( ConfigGraph& graph, const RankInfo &myRank, SimTime_t min_part );

    /** Set cycle count, which, if reached, will cause the simulation to halt. */
    void setStopAtCycle( Config* cfg );
    /** Perform the init() phase of simulation */
    void initialize();
    /** Perform the setup() and run phases of the simulation. */
    void setup();
    void run();
    void finish();

    bool isIndependentThread() { return independent;}
    
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
    /** Register a handler to be called on a set frequency */
    TimeConverter* registerClock(std::string freq, Clock::HandlerBase* handler);
    TimeConverter* registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler);
    /** Remove a clock handler from the list of active clock handlers */
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);
    /** Reactivate an existing clock and handler.
     * @return time when handler will next fire
     */
    Cycle_t reregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);
    /** Returns the next Cycle that the TImeConverter would fire. */
    Cycle_t getNextClockCycle(TimeConverter* tc);

    /** Register a OneShot event to be called after a time delay
        Note: OneShot cannot be canceled, and will always callback after
              the timedelay.
    */
    TimeConverter* registerOneShot(std::string timeDelay, OneShot::HandlerBase* handler);
    TimeConverter* registerOneShot(const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler);
    
    /** Insert an activity to fire at a specified time */
    void insertActivity(SimTime_t time, Activity* ev);

    /** Return the exit event */
    Exit* getExit() const { return m_exit; }

    const std::vector<SimTime_t>& getInterThreadLatencies() { return interThreadLatencies; }
    const SimTime_t getInterThreadMinLatency() { return interThreadMinLatency; }
    static TimeConverter* getMinPartTC() { return minPartTC; }

    /** Return the TimeLord associated with this Simulation */
    static TimeLord* getTimeLord(void) { return &timeLord; }
    /** Return the base simulation Output class instance */
    static Output& getSimulationOutput() { return sim_output; };

    static Statistics::StatisticOutput* getStatisticsOutput() { return statisticsOutput; }
    static void signalStatisticsBegin();
    static void signalStatisticsEnd();

    uint64_t getTimeVortexMaxDepth() const;
    uint64_t getTimeVortexCurrentDepth() const;
    uint64_t getSyncQueueDataSize() const;


    /** Return the Statistic Processing Engine associated with this Simulation */
    Statistics::StatisticProcessingEngine* getStatisticsProcessingEngine(void) const { return statisticsEngine; }

    /** Return pointer to map of links for a given component id */
    LinkMap* getComponentLinkMap(ComponentId_t id) const {
        ComponentInfo* info = compInfoMap.getByID(id);
        if ( NULL == info ) {
            return NULL;
        } else {
            return info->getLinkMap();
        }
    }


    // /** Returns reference to the Component Map */
    // const CompMap_t& getComponentMap(void) const { return compMap; }
    // /** Returns reference to the Component ID Map */
    // // const CompIdMap_t& getComponentIdMap(void) const { return compIdMap; }
    const ComponentInfoMap& getComponentInfoMap(void) { return compInfoMap; }

    
    /** Returns the component with a given name */
    Component* getComponent(const std::string &name) const
    {
        ComponentInfo* i = compInfoMap.getByName(name);
        if (NULL != i) {
            return i->getComponent();
        } else {
            printf("Simulation::getComponent() couldn't find component with name = %s\n",
                   name.c_str()); 
            exit(1); 
        }
    }
    
    /** returns the component with the given ID */
    Component* getComponent(const ComponentId_t &id) const
    {        
		ComponentInfo* i = compInfoMap.getByID(id);
		// CompInfoMap_t::const_iterator i = compInfoMap.find(id);
		if ( NULL != i ) {
			return i->getComponent();
		} else {
            printf("Simulation::getComponent() couldn't find component with id = %lu\n", id);
            exit(1);
		}
    }

    ComponentInfo* getComponentInfo(const std::string& name) const
    {
        ComponentInfo* i = compInfoMap.getByName(name);
        if (NULL != i) {
            return i;
        } else {
            printf("Simulation::getComponentInfo() couldn't find component with name = %s\n",
                   name.c_str()); 
            exit(1); 
        }
    }
    
    ComponentInfo* getComponentInfo(const ComponentId_t &id) const
    {        
		ComponentInfo* i = compInfoMap.getByID(id);
		// CompInfoMap_t::const_iterator i = compInfoMap.find(id);
		if ( NULL != i ) {
			return i;
		} else {
            printf("Simulation::getComponentInfo() couldn't find component with id = %lu\n", id);
            exit(1);
		}
    }

    statEnableList_t* getComponentStatisticEnableList(const ComponentId_t &id) const
    {
		statEnableMap_t::const_iterator i = statisticEnableMap.find(id);
		if (i != statisticEnableMap.end()) {
			return i->second;
		} else {
            printf("Simulation::getComponentStatisticEnableList() couldn't find component with id = %lu\n", id);
            exit(1);
		}
        return NULL; 
    }
    
    statParamsList_t* getComponentStatisticParamsList(const ComponentId_t &id) const
    {
		statParamsMap_t::const_iterator i = statisticParamsMap.find(id);
		if (i != statisticParamsMap.end()) {
			return i->second;
		} else {
            printf("Simulation::getComponentStatisticParamsList() couldn't find component with id = %lu\n", id);
            exit(1);
		}
        return NULL; 
    }


    /** Returns the Introspector with the given name */
    Introspector* getIntrospector(const std::string &name) const
    {
        IntroMap_t::const_iterator i = introMap.find(name);
        if (i != introMap.end()) {
            return i->second;
        } else {
            printf("Simulation::getIntrospector() couldn't find introspector with name = %s\n",
                   name.c_str()); 
            exit(1); 
        }
    }

    /**
	Set the output directory for this simulation
	@param outDir Path of directory to place simulation outputs in
    */
    void setOutputDirectory(std::string& outDir) {
	output_directory = outDir;
    }

    /**
	Returns the output directory of the simulation
	@return Directory in which simulation outputs are placed
    */
    std::string& getOutputDirectory() {
	return output_directory;
    }

    /** Signifies that an event type is required for this simulation
     *  Causes to Factory to verify that the required event type can be found.
     *  @param name fully qualified libraryName.EventName
     */
    void requireEvent(std::string name);

    /**
     * Returns the time of the next item to be executed
     * that is in the TImeVortex of the Simulation
     */
    SimTime_t getNextActivityTime() const;

    /**
     *  Gets the minimum next activity time across all TimeVortices in
     *  the Rank
     */
    static SimTime_t getLocalMinimumNextActivityTime();
    
    /**
     * Returns the Simulation's SharedRegionManager
     */
    static SharedRegionManager* getSharedRegionManager() { return sharedRegionManager; }
    
    /** 
    * Returns true when the Wireup is finished.
    */
    bool isWireUpFinished() {return wireUpFinished; }

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
    static Core::ThreadSafe::Barrier& getThreadBarrier() { return barrier; }

    /** Factory used to generate the simulation components */
    static Factory *factory;
    /** TimeLord of the simulation */
    static TimeLord timeLord;
    /** Statistics Output of the simulation */
    static Statistics::StatisticOutput* statisticsOutput;
    /** Output */
    static Output sim_output;
    static Core::ThreadSafe::Barrier barrier;
    static Core::ThreadSafe::Barrier exit_barrier;
    static std::mutex simulationMutex;



    Component* createComponent(ComponentId_t id, std::string &name, 
                               Params &params);
    Introspector* createIntrospector(std::string &name, 
                                     Params &params );

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
    static SyncBase* sync;
    static SimTime_t minPart;
    static TimeConverter*   minPartTC;
    std::vector<SimTime_t> interThreadLatencies;
    SimTime_t        interThreadMinLatency;
    SyncManager*     syncManager;
    ThreadSync*      threadSync;
    ComponentInfoMap compInfoMap;
    IntroMap_t       introMap;
    clockMap_t       clockMap;
    statEnableMap_t  statisticEnableMap;
    statParamsMap_t  statisticParamsMap;
    oneShotMap_t     oneShotMap;
    SimTime_t        currentSimCycle;
    SimTime_t        endSimCycle;
    int              currentPriority;
    static Exit*     m_exit;
    SimulatorHeartbeat*	m_heartbeat;
    bool             endSim;
    RankInfo         my_rank;
    RankInfo         num_ranks;
    bool             independent; // true if no links leave thread (i.e. no syncs required)
    static std::atomic<int>       init_msg_count;
    unsigned int     init_phase;
    volatile sig_atomic_t lastRecvdSignal;
    ShutdownMode_t   shutdown_mode;
    // std::map<ComponentId_t,LinkMap*> component_links;
    std::string      output_directory;
    static SharedRegionManager* sharedRegionManager;
    bool             wireUpFinished;
    /** Statistics Timing Engine of the simulation */
    Statistics::StatisticProcessingEngine* statisticsEngine;

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

// Uses Simulation's barrier
void wait_my_turn_start();

void wait_my_turn_end(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads);

// Uses Simulation's barrier
void wait_my_turn_end();


} // namespace SST

#endif //SST_CORE_SIMULATION_H
