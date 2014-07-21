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

#ifndef SST_CORE_COMPONENT_H
#define SST_CORE_COMPONENT_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <map>

#include <sst/core/clock.h>
#include <sst/core/event.h>
//#include <sst/core/params.h>
//#include <sst/core/link.h>
//#include <sst/core/timeConverter.h>

namespace SST {

class Link;
class LinkMap;
class Module;
class Params;
class TimeConverter;
class UnitAlgebra;
 
#define _COMP_DBG( fmt, args...) __DBG( DBG_COMP, Component, fmt, ## args )

/**
 * Main component object for the simulation. 
 *  All models inherit from this. 
 */
class Component {
public:
    /* Deprecated typedef */
//    typedef Params Params_t;

    /** Constructor. Generally only called by the factory class. 
        @param id Unique component ID
    */
    Component( ComponentId_t id );
    virtual ~Component();

    /** Called when SIGINT or SIGTERM has been seen.
     * Allows components opportunity to clean up external state.
     */
    virtual void emergencyShutdown(void) {}

    /** Returns unique component ID */
    inline ComponentId_t getId() const { return id; }

	/** Returns component Name */
	inline const std::string& getName() const { return name; }

    /** Component's type, set by the factory when the object is created.
        It is identical to the configuration string used to create the
        component. I.e. the XML "<component id="aFoo"><foo>..." would
        set component::type to "foo" */
    std::string type;

    /** Used during the init phase.  The method will be called each phase of initialization.
     Initialization ends when no components have sent any data. */
    virtual void init(unsigned int phase) {}
    /** Called after all components have been constructed and inialization has
	completed, but before simulation time has begun. */
    virtual void setup( ) { }
    /** Called after simulation completes, but before objects are
        destroyed. A good place to print out statistics. */
    virtual void finish( ) { }

    /** Currently unused function */
    virtual bool Status( ) { return 0; }

    /**
     * Called by the Simulation to request that the component
     * print it's current status.  Useful for debugging.
     * @param out The Output class which should be used to print component status.
     */
    virtual void printStatus(Output &out) { return; }

    /** Determine if a port name is connected to any links */
    bool isPortConnected(const std::string &name) const;

    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler = NULL);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, std::string time_base, Event::HandlerBase* handler = NULL);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, Event::HandlerBase* handler = NULL);

    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureSelfLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler = NULL);
    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureSelfLink( std::string name, std::string time_base, Event::HandlerBase* handler = NULL);
    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
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
    TimeConverter* registerClock( const UnitAlgebra& freq, Clock::HandlerBase* handler, 
                                  bool regAll = true);
    /** Removes a clock handler from the component */
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);

    /** Reactivates an existing Clock and Handler
     * @return time of next time clock handler will fire
     */
    Cycle_t reregisterClock(TimeConverter *freq, Clock::HandlerBase* handler);
    /** Returns the next Cycle that the TimeConverter would fire */
    Cycle_t getNextClockCycle(TimeConverter *freq);

    /** Registers a default time base for the component and optionally
        sets the the component's links to that timebase. Useful for
        components which do not have a clock, but would like a default
        timebase.
        @param base Frequency for the clock in SI units
        @param regAll Should this clock perioud be used as the default
        time base for all of the links connected to this component
    */
    TimeConverter* registerTimeBase( std::string base, bool regAll = true);

    TimeConverter* getTimeConverter( const std::string& base );
    TimeConverter* getTimeConverter( const UnitAlgebra& base );
    
    /** return the time since the simulation began in units specified by
        the parameter.
        @param tc TimeConverter specificing the units */
    SimTime_t getCurrentSimTime(TimeConverter *tc);
    /** return the time since the simulation began in the default timebase */
    inline SimTime_t getCurrentSimTime() { 
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

    /** Register that the simulation should not end until this
        component says it is OK to. Calling this function (generally
        done in Component::setup() or in component constructor)
        increments a global counter. Calls to
        Component::unregisterExit() decrements the counter. The
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

    /** Register as a primary component, which allows the component to
        specifiy when it is and is not OK to end simulation.  The
        simulator will not end simulation natuarally (through use of
        the Exit object) while any primary component has specified
        primaryComponentDoNotEndSim().  However, it is still possible
        for Actions other than Exit to end simulation.  Once all
        primary components have specified
        primaryComponentOKToEndSim(), the Exit object will trigger and
        end simulation.

	This must be called during simulation wireup (i.e during the
	constructor for the component).  By default, the state of the
	primary component is set to OKToEndSim.

	If no component registers as a primary component, then the
	Exit object will not be used for that simulation and
	simulation termination must be accomplished through some other
	mechanism (e.g. --stopAt flag, or some other Action object).

        @sa Component::primaryComponentDoNotEndSim()
        @sa Component::primaryComponentOKToEndSim()
    */
    void registerAsPrimaryComponent();

    /** Tells the simulation that it should not exit.  The component
	will remain in this state until a call to
	primaryComponentOKToEndSim().

        @sa Component::registerAsPrimaryComponent()
        @sa Component::primaryComponentOKToEndSim()
    */
    void primaryComponentDoNotEndSim();
    
    /** Tells the simulation that it is now OK to end simulation.
	Simulation will not end until all primary components have
	called this function.
	
        @sa Component::registerAsPrimaryComponent()
        @sa Component::primaryComponentDoNotEndSim()
    */
    void primaryComponentOKToEndSim();

    /** Loads a module from an element Library
     * @param type Fully Qualified library.moduleName
     * @param params Parameters the module should use for configuration
     * @return handle to new instance of module, or NULL on failure.
     */
    Module* loadModule(std::string type, Params& params);
    /** Loads a module from an element Library
     * @param type Fully Qualified library.moduleName
     * @param comp Pointer to component to pass to Module's constructor
     * @param params Parameters the module should use for configuration
     * @return handle to new instance of module, or NULL on failure.
     */
    Module* loadModuleWithComponent(std::string type, Component* comp, Params& params);
    
    
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

    /** Creates a new selfLink */
    Link* selfLink( std::string name, Event::HandlerBase* handler = NULL );

private:

    void addSelfLink(std::string name);

    /** Unique ID */
    ComponentId_t   id;
	std::string name;
    LinkMap* myLinks;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
};

} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Component)

#endif // SST_CORE_COMPONENT_H
