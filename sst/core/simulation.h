// -*- c++ -*-

// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
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

#include "sst/core/clock.h"
//#include "sst/core/sdl.h"
//#include "sst/core/component.h"
//#include "sst/core/params.h"

namespace SST {

#define _SIM_DBG( fmt, args...) __DBG( DBG_SIM, Sim, fmt, ## args )

class Activity;
class Component;
class Config;
class ConfigGraph;
class Exit;
class Factory;
//class Graph;
class Introspector;
class LinkMap;
class Params;
class Sync;
class TimeConverter;
class TimeLord;
class TimeVortex;

typedef std::map<std::string, Introspector* > IntroMap_t;
typedef std::map<std::string, Component* > CompMap_t;
typedef std::map<ComponentId_t, std::string > CompIdMap_t;

// The Factory and TimeLord objects both should only be associated
// with a simulation object and never created on their own.  To
// accomplish this, create a base class of Simluation which is
// friended by both Factory and TimeLord.  The friendship is not
// inherited by the Simulation class, limiting the exposure of
// internal information to the 20 line object below and it is
// impossible to create either a Factory or a timeLord without a
// simulation object.
class SimulationBase {
public:
    Factory* getFactory(void) const { return factory; }
    TimeLord* getTimeLord(void) const { return timeLord; }

protected:
    SimulationBase(Config* config);
    SimulationBase() { } // Don't call - here for serialization
    virtual ~SimulationBase();

    TimeConverter* minPartToTC(SimTime_t cycles) const;

    Factory *factory;
    TimeLord *timeLord;
    
private:
    SimulationBase(SimulationBase const&); // Don't Implement
    void operator=(SimulationBase const&); // Don't implement

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class Simulation : public SimulationBase {
public:
    typedef std::map<SimTime_t, Clock*> clockMap_t; 
    typedef std::map< unsigned int, Sync* > SyncMap_t;

    ~Simulation();

    static Simulation *createSimulation(Config *config, int my_rank, int num_ranks);
    static Simulation *getSimulation() { return instance; }
    static void setSignal(int signal);
    void printStatus(bool fullStatus);

    int performWireUp( ConfigGraph& graph, int myRank );

    void initialize();
    void run(); 
    SimTime_t getCurrentSimCycle() const;
    int getRank() const {return my_rank;}
    int getNumRanks() const {return num_ranks;}
    TimeConverter* registerClock(std::string freq, Clock::HandlerBase* handler);
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);
    Cycle_t reregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);
    Cycle_t getNextClockCycle(TimeConverter* tc);
    void insertActivity(SimTime_t time, Activity* ev);
    Exit* getExit() const { return m_exit; }

    LinkMap* getComponentLinkMap(ComponentId_t id) const {
        std::map<ComponentId_t,LinkMap*>::const_iterator i = component_links.find(id);
        if (i == component_links.end()) {
            return NULL;
        } else {
            return i->second;
        }
    }

    void removeComponentLinkMap(ComponentId_t id) {
        std::map<ComponentId_t,LinkMap*>::iterator i = component_links.find(id);
        if (i == component_links.end()) {
            return;
        } else {
            component_links.erase(i);
        }
    }

    const CompMap_t& getComponentMap(void) const { return compMap; }
    const CompIdMap_t& getComponentIdMap(void) const { return compIdMap; }

    Component*
    getComponent(const std::string &name) const
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

    Component*
    getComponent(const ComponentId_t &id) const
    {
		CompIdMap_t::const_iterator i = compIdMap.find(id);
		if (i != compIdMap.end()) {
			return getComponent(i->second);
		} else {
            printf("Simulation::getComponent() couldn't find component with id = %lu\n", id);
            exit(1);
		}
    }

    Introspector*
    getIntrospector(const std::string &name) const
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

    void requireEvent(std::string name);
    
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

    TimeVortex*      timeVortex;
    TimeConverter*   minPartTC;
    Activity*        current_activity;
    Sync*            sync;
    CompMap_t        compMap;
    CompIdMap_t      compIdMap;
    IntroMap_t       introMap;
    clockMap_t       clockMap;
    SimTime_t        currentSimCycle;
    Exit*            m_exit;
    bool             endSim;
    int              my_rank;
    int              num_ranks;
    int              init_msg_count;
    unsigned int     init_phase;
    volatile sig_atomic_t lastRecvdSignal;
    std::map<ComponentId_t,LinkMap*> component_links;
    
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
