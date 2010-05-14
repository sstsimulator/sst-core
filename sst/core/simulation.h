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



#ifndef _SST_SIMULATION_H
#define _SST_SIMULATION_H

#include <sst/sst.h>
#include <sst/sdl.h>

#include <sst/event.h>
#include <sst/eventQueue.h>
#include <sst/component.h>
#include <sst/factory.h>
#include <sst/clockEvent.h>
#include <sst/introspector.h>

#include <iostream>

namespace SST {

#define _SIM_DBG( fmt, args...) __DBG( DBG_SIM, Sim, fmt, ## args )


class Config;
class Graph;
class Sync;
class Factory;
class Clock;
class TimeLord;
class ClockEvent;
class Exit;
class SDL_CompMap;

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
    virtual ~SimulationBase();

    Factory *factory;
    TimeLord *timeLord;

private:
    SimulationBase();                      // Don't Implement
    SimulationBase(SimulationBase const&); // Don't Implement
    void operator=(SimulationBase const&); // Don't implement
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
    void Run();
    SimTime_t getCurrentSimCycle();
    TimeConverter* registerClock(std::string freq, ClockHandler_t* handler);
    void unregisterClock(TimeConverter *tc, ClockHandler_t* handler);
    void insertEvent(SimTime_t time, Event* ev, EventHandlerBase<bool,Event*>* functor);
    Exit* getExit() { return m_exit; }

    /* for introspection*/
    CompMap_t* getCompMap(){
	    return compMap;
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
	
private:
    friend class Link;

    Simulation(Config* config);
    Simulation();                      // Don't Implement
    Simulation(Simulation const&);     // Don't Implement
    void operator=(Simulation const&); // Don't implement
	
    std::string EventName(Event *);
    Component* createComponent(ComponentId_t id, std::string name, 
                               Component::Params_t params);
    EventQueue_t* getEventQueue() { return eQueue; }

    EventQueue_t*    eQueue;
    SyncMap_t        syncMap;
    CompMap_t*       compMap;
    IntroMap_t*      introMap;
    clockMap_t       clockMap;
    Component*       test;
    SimTime_t        currentSimCycle;
    Exit*            m_exit;

    static const Time_t frequency = 1000000000000.0;
    static Simulation *instance;
};

} // namespace SST

#endif
