// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_SIMULATION_H
#define SST_CORE_SIMULATION_H

#include <iostream>

#include "sst/core/sst.h"
#include "sst/core/sdl.h"
#include "sst/core/clockHandler.h"
#include "sst/core/component.h"

namespace SST {

#define _SIM_DBG( fmt, args...) __DBG( DBG_SIM, Sim, fmt, ## args )

class Activity;
class Clock;
class ClockEvent;
class Config;
class Exit;
class Factory;
class Graph;
class Introspector;
class LinkMap;
class SDL_CompMap;
class Sync;
class TimeConverter;
class TimeLord;
class TimeVortex;

typedef std::map< ComponentId_t, Introspector* > IntroMap_t;
typedef std::map< ComponentId_t, Component* > CompMap_t;

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
    Factory* getFactory(void) { return factory; }
    TimeLord* getTimeLord(void) { return timeLord; }

protected:
    SimulationBase(Config* config);
    SimulationBase() { } // Don't call - here for serialization
    virtual ~SimulationBase();

    TimeConverter* minPartToTC(SimTime_t cycles);

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
    typedef std::map<SimTime_t, ClockEvent*> clockMap_t; 
    typedef std::map< unsigned int, Sync* > SyncMap_t;

    static Simulation *createSimulation(Config *config);
    static Simulation *getSimulation() { return instance; }
    static void printStatus(void);

    int WireUp(Graph& graph, SDL_CompMap_t& sdlMap,
               int minPart, int myRank);
    int performWireUp( Graph& graph, SDL_CompMap_t& sdlMap,
		       int minPart, int myRank );
    void Run();
    SimTime_t getCurrentSimCycle();
    TimeConverter* registerClock(std::string freq, ClockHandler_t* handler);
    void unregisterClock(TimeConverter *tc, ClockHandler_t* handler);
    void insertActivity(SimTime_t time, Activity* ev);
//     void insertEvent(SimTime_t time, Activity* ev, EventHandlerBase<bool,Activity*>* functor);
//     void insertEvent(SimTime_t time, Activity* ev, EventHandlerBase<bool,Event*>* functor);
    Exit* getExit() { return m_exit; }

    /* for introspection*/
    CompMap_t* getCompMap(){
	    return compMap;
	}

    LinkMap* getComponentLinkMap(ComponentId_t id) {
	return component_links[id];
    }
    
    Component* getComponent(const char* type){
	    ComponentId_t id = atoi(type);
	    std::map<ComponentId_t, Component*>::iterator it;
	    it = compMap->find(id);
            if (it != compMap->end() )   // found key
               return it->second;
	    else { 
	       printf("Simulation::getComponent couldn't find component with id = %lu\n",id); exit(1); 
	    }
    }

    Introspector* getIntrospector(const char* type){
	    ComponentId_t id = atoi(type);
	    std::map<ComponentId_t, Introspector*>::iterator it;
	    it = introMap->find(id);
            if (it != introMap->end() )   // found key
               return it->second;
	    else { 
	       printf("Simulation::getIntrospector couldn't find introspector with id = %lu\n",id); exit(1); 
	    }
    }

protected:
	
private:
    friend class Link;
    friend class Action;

    Simulation(); // Don't call.  Only rational way to serialize
    Simulation(Config* config);
    Simulation(Simulation const&);     // Don't Implement
    void operator=(Simulation const&); // Don't implement
	
    std::string EventName(Event *);
    Component* createComponent(ComponentId_t id, std::string name, 
                               Component::Params_t params);
//     EventQueue_t* getEventQueue() { return eQueue; }

//     EventQueue_t*    eQueue;

    TimeVortex* getTimeVortex() { return timeVortex; }

    void endSimulation(void) {
	endSim = true;
    }

    TimeVortex*      timeVortex;
    Sync*            sync;
    CompMap_t*       compMap;
    IntroMap_t*      introMap;
    clockMap_t       clockMap;
    SimTime_t        currentSimCycle;
    Exit*            m_exit;
    bool endSim;
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

#endif
