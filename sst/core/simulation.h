// -*- c++ -*-

// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SIMULATION_H
#define SST_CORE_SIMULATION_H

#include "sst/core/sst_types.h"
#include <sst/core/serialization.h>

#include <signal.h>
#include <iostream>

#include "sst/core/output.h"
#include "sst/core/clock.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/config.h"

//#include "sst/core/sdl.h"
//#include "sst/core/component.h"
//#include "sst/core/params.h"

namespace SST {

#define _SIM_DBG( fmt, args...) __DBG( DBG_SIM, Sim, fmt, ## args )

class Activity;
class Component;
//class Config;
class ConfigGraph;
class Exit;
class Factory;
class SimulatorHeartbeat;
//class Graph;
class Introspector;
class LinkMap;
class Params;
class Sync;
class TimeConverter;
class TimeLord;
class TimeVortex;
class UnitAlgebra;
    
typedef std::map<std::string, Introspector* > IntroMap_t;
typedef std::map<std::string, Component* > CompMap_t;
typedef std::map<ComponentId_t, std::string > CompIdMap_t;

/** The Factory and TimeLord objects both should only be associated
    with a simulation object and never created on their own.  To
    accomplish this, create a base class of Simluation which is
    friended by both Factory and TimeLord.  The friendship is not
    inherited by the Simulation class, limiting the exposure of
    internal information to the 20 line object below and it is
    impossible to create either a Factory or a timeLord without a
    simulation object.
 */
class SimulationBase {
public:
    /** Return the Factory associated with this Simulation */
    Factory* getFactory(void) const { return factory; }
    /** Return the TimeLord associated with this Simulation */
    TimeLord* getTimeLord(void) const { return timeLord; }

protected:
    /** Constructor
     * @param config - Configuration to use for this simulation
     */
    SimulationBase(Config* config);
    SimulationBase() { } // Don't call - here for serialization
    virtual ~SimulationBase();

    /** Get a handle to a TimeConverter
     * @param cycles Frequency which is the base of the TimeConverter
     */
    TimeConverter* minPartToTC(SimTime_t cycles) const;

    /** Factory used to generate the simulation components */
    Factory *factory;
    /** TimeLord of the simulation */
    TimeLord *timeLord;

private:
    SimulationBase(SimulationBase const&); // Don't Implement
    void operator=(SimulationBase const&); // Don't implement

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

/**
 * Main control class for a SST Simulation.
 * Provides base features for managing the simulation
 */
class Simulation : public SimulationBase {
public:
    typedef std::map<SimTime_t, Clock*> clockMap_t;    /*!< Map of times to clocks */
    typedef std::map< unsigned int, Sync* > SyncMap_t; /*!< Map of times to Sync Objects */

    ~Simulation();

    /** Create new simulation
     * @param config - Configuration of the simulation
     * @param my_rank - Parallel Rank of this simulation object
     * @param num_ranks - How many Ranks are in the simulation
     */
    static Simulation *createSimulation(Config *config, int my_rank, int num_ranks);
    /** Return a pointer to the singleton instance of the Simulation */
    static Simulation *getSimulation() { return instance; }
    /** Sets an internal flag for signaling the simulation.  Used internally */
    static void setSignal(int signal);
    /** Causes the current status of the simulation to be printed to stderr.
     * @param fullStatus - if true, call printStatus() on all components as well
     *        as print the base Simulation's status
     */
    void printStatus(bool fullStatus);

    /** Converts a ConfigGraph graph into actual set of links and components */
    int performWireUp( ConfigGraph& graph, int myRank );

    /** Set cycle count, which, if reached, will cause the simulation to halt. */
    void setStopAtCycle( Config* cfg );
    /** Perform the init() phase of simulation */
    void initialize();
    /** Perform the setup() and run phases of the simulation. */
    void run();
    /** Return the base simulation Output class instance */
    Output& getSimulationOutput() { return sim_output; };

    /** Get the run mode of the simulation (e.g. init, run, both etc) */
    Config::Mode_t getSimulationMode() const;
    /** Return the current simulation time as a cycle count*/
    SimTime_t getCurrentSimCycle() const;
    /** Return the current priority */
    int getCurrentPriority() const;
    /** Return the elapsed simulation time as a time */
    UnitAlgebra getElapsedSimTime() const;
    /** Get this instance's parallel rank */
    int getRank() const {return my_rank;}
    /** Get the number of parallel ranks in the simulation */
    int getNumRanks() const {return num_ranks;}
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

    /** Insert an activity to fire at a specified time */
    void insertActivity(SimTime_t time, Activity* ev);

    /** Return the exit event */
    Exit* getExit() const { return m_exit; }

    /** Return pointer to map of links for a given component id */
    LinkMap* getComponentLinkMap(ComponentId_t id) const {
        std::map<ComponentId_t,LinkMap*>::const_iterator i = component_links.find(id);
        if (i == component_links.end()) {
            return NULL;
        } else {
            return i->second;
        }
    }

    /** Clears the linkMap for a given Component ID */
    void removeComponentLinkMap(ComponentId_t id) {
        std::map<ComponentId_t,LinkMap*>::iterator i = component_links.find(id);
        if (i == component_links.end()) {
            return;
        } else {
            component_links.erase(i);
        }
    }

    /** Returns reference to the Component Map */
    const CompMap_t& getComponentMap(void) const { return compMap; }
    /** Returns reference to the Component ID Map */
    const CompIdMap_t& getComponentIdMap(void) const { return compIdMap; }

    /** Returns the component with a given name */
    Component* getComponent(const std::string &name) const
    {
        CompMap_t::const_iterator i = compMap.find(name);
        if (i != compMap.end()) {
            return i->second;
        } else {
            printf("Simulation::getComponent() couldn't find component with name = %s\n",
                   name.c_str()); 
            exit(1); 
        }
    }

    /** returns the component with the given ID */
    Component* getComponent(const ComponentId_t &id) const
    {
		CompIdMap_t::const_iterator i = compIdMap.find(id);
		if (i != compIdMap.end()) {
			return getComponent(i->second);
		} else {
            printf("Simulation::getComponent() couldn't find component with id = %lu\n", id);
            exit(1);
		}
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

    /** Signifies that an event type is required for this simulation
     *  Causes to Factory to verify that the required event type can be found.
     *  @param name fully qualified libraryName.EventName
     */
    void requireEvent(std::string name);

    /**
     * Returns the time of the next item to be executed
     * that is in the TImeVortex of the Simulation
     */
    SimTime_t getNextActivityTime();
    
private:
    friend class Link;
    friend class Action;
    
    Simulation(); // Don't call.  Only rational way to serialize
    Simulation(Config* config, int my_rank, int num_ranks);
    Simulation(Simulation const&);     // Don't Implement
    void operator=(Simulation const&); // Don't implement
	
    Component* createComponent(ComponentId_t id, std::string name, 
                               Params params);
    Introspector* createIntrospector(std::string name, 
                                     Params params );

    TimeVortex* getTimeVortex() const { return timeVortex; }
    void endSimulation(void) { endSim = true; }

    Config::Mode_t   runMode;
    TimeVortex*      timeVortex;
    TimeConverter*   minPartTC;
    Activity*        current_activity;
    Sync*            sync;
    CompMap_t        compMap;
    CompIdMap_t      compIdMap;
    IntroMap_t       introMap;
    clockMap_t       clockMap;
    SimTime_t        currentSimCycle;
    int              currentPriority;
    Exit*            m_exit;
    SimulatorHeartbeat*	m_heartbeat;
    bool             endSim;
    int              my_rank;
    int              num_ranks;
    int              init_msg_count;
    unsigned int     init_phase;
    volatile sig_atomic_t lastRecvdSignal;
    std::map<ComponentId_t,LinkMap*> component_links;
    Output           sim_output;

    static Simulation *instance;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    template<class Archive>
    friend void save_construct_data(Archive & ar, 
                                    const Simulation * t, 
                                    const unsigned int file_version)
    {
    }

    template<class Archive>
    friend void load_construct_data(Archive & ar, 
                                    Simulation * t,
                                    const unsigned int file_version)
    {
        ::new(t)Simulation();
        Simulation::instance = t;
    }
};

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::SimulationBase)
BOOST_CLASS_EXPORT_KEY(SST::Simulation)

#endif //SST_CORE_SIMULATION_H
