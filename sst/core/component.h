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


#ifndef SST_COMPONENT_H
#define SST_COMPONENT_H

#include <map>

#include <sst/core/sst_types.h>
#include <sst/core/linkMap.h>
#include <sst/core/clock.h>
#include <sst/core/timeConverter.h>

namespace SST {

#define _COMP_DBG( fmt, args...) __DBG( DBG_COMP, Component, fmt, ## args )

/**
 * Main component object for the simulation. 
 *  All models inherit from this. 
 */
class Component {
public:
    typedef  std::map<std::string,std::string> Params_t;

    /** Constructor. Generally only called by the factory class. 
        @param id Unique component ID
    */
    Component( ComponentId_t id );
    virtual ~Component() {}

    /** Returns unique component ID */
    inline ComponentId_t getId() const { return id; }

    /** Component's type, set by the factory when the object is created.
        It is identical to the configuration string used to create the
        component. I.e. the XML "<component id="aFoo"><foo>..." would
        set component::type to "foo" */
    std::string type;

    /** Called after all components have been constructed, but before
        simulation time has begun. */
    virtual int Setup( ) { return 0; }
    /** Called after simulation completes, but before objects are
        destroyed. A good place to print out statistics. */
    virtual int Finish( ) { return 0; }

    virtual bool Status( ) { return 0; }
    
    Link* configureLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler = NULL);
    Link* configureLink( std::string name, std::string time_base, Event::HandlerBase* handler = NULL);
    Link* configureLink( std::string name, Event::HandlerBase* handler = NULL);

    Link* configureSelfLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler = NULL);
    Link* configureSelfLink( std::string name, std::string time_base, Event::HandlerBase* handler = NULL);
    Link* configureSelfLink( std::string name, Event::HandlerBase* handler = NULL);

    /** Registers a clock for this component.
        @param freq Frequency for the clock in SI units
        @param handler Pointer to Clock::HandlerBase which is to be invoked
        at the specified interval
        @param regAll Should this clock perioud be used as the default
        time base for all of the links connected to this component
    */
    TimeConverter* registerClock( std::string freq, Clock::HandlerBase* handler, 
                                  bool regAll = true);
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);

    /** Registers a default time base for the component and optionally
        sets the the component's links to that timebase. Useful for
        components which do not have a clock, but would like a default
        timebase.
        @params base Frequency for the clock in SI units
        @params regAll Should this clock perioud be used as the default
        time base for all of the links connected to this component
    */
    TimeConverter* registerTimeBase( std::string base, bool regAll = true);

    /** return the time since the simulation began in units specified by
        the parameter.
        @param tc TimeConverter specificing the units */
    SimTime_t getCurrentSimTime(TimeConverter *tc);
    /** return the time since the simulation began in the default timebase */
    SimTime_t getCurrentSimTime() { 
        return getCurrentSimTime(defaultTimeBase); 
    }
    /** return the time since the simulation began in timebase specified
        @param base Timebase frequency in SI Units */
    SimTime_t getCurrentSimTime(std::string base);

    /** Utility function to return the time since the simulation began in nanoseconds */ 
    SimTime_t getCurrentSimTimeNano();
    /** Utility function to return the time since the simulation began in microseconds */ 
    SimTime_t getCurrentSimTimeMicro();
    /** Utility function to return the time since the simulation began in milliseconds */ 
    SimTime_t getCurrentSimTimeMilli();

    /** Register that the simulation should not end until this component
        says it is OK to. Calling this function (generally done in
        Component::Setup()) increments a global counter. Calls to
        Component::unregisterExit() decrement the counter. The
        simulation cannot end unless this counter reaches zero, or the
        simulation time limit is reached. This counter is synchonized
        periodically with the other nodes.

        @sa Component::unregisterExit()
    */
    bool registerExit();

    /** Indicate permission for the simulation to end. This function is
        the mirror of Component::registerExit(). It decrements the
        global counter, which, upon reaching zero, indicates that the
        simulation can terminate. @sa Component::registerExit() */
    bool unregisterExit();

protected:
    Component(); // For serialization only

    /** Manually set the default detaulTimeBase */ 
    void setDefaultTimeBase(TimeConverter *tc) {
        defaultTimeBase = tc;
    }

    /** Timebase used if no other timebase is specified for calls like
        Component::getCurrentSimTime(). Often set by Component::registerClock()
        function */
    TimeConverter* defaultTimeBase;

    Link* selfLink( std::string name, Event::HandlerBase* handler = NULL );

private:

    void addSelfLink(std::string name);
    
    /** Unique ID */
    ComponentId_t   id;
    LinkMap* myLinks;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
}; 

}

#endif
