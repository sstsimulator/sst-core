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

#ifndef SST_CORE_COMPONENT_H
#define SST_CORE_COMPONENT_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <map>
#include <string>

#include <sst/core/clock.h>
#include <sst/core/oneshot.h>
#include <sst/core/event.h>
//#include <sst/core/params.h>
//#include <sst/core/link.h>
//#include <sst/core/timeConverter.h>
#include <sst/core/statapi/statoutput.h>
#include <sst/core/statapi/statengine.h>
#include <sst/core/statapi/statnull.h>
#include <sst/core/statapi/stataccumulator.h>
#include <sst/core/statapi/stathistogram.h>
#include <sst/core/statapi/statuniquecount.h>
#include "sst/core/simulation.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/statapi/statbase.h"

using namespace SST::Statistics;

namespace SST {

class Link;
class LinkMap;
class Module;
class Params;
class SubComponent;
class TimeConverter;
class UnitAlgebra;
class SharedRegion;
class SharedRegionMerger;

#define _COMP_DBG( fmt, args...) __DBG( DBG_COMP, Component, fmt, ## args )

/**
 * Main component object for the simulation. 
 *  All models inherit from this. 
 */
class Component {
public:

    static bool isPortValidForComponent(const std::string& comp_name, const std::string& port_name);
    
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
	/* inline const std::string& getName() const { return name; } */
	inline const std::string& getName() const { return my_info->getName(); }
    
    
    /** Component's type, set by the factory when the object is created.
        It is identical to the configuration string used to create the
        component. I.e. the XML "<component id="aFoo"><foo>..." would
        set component::type to "foo" */
    /* std::string type; */

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

    /** Registers a OneShot event for this component.
        Note: OneShot cannot be canceled, and will always callback after
          the timedelay.  
        @param timeDelay Time delay for the OneShot in SI units
        @param handler Pointer to OneShot::HandlerBase which is to be invoked
        at the specified interval
    */
    TimeConverter* registerOneShot( std::string timeDelay, OneShot::HandlerBase* handler);
    TimeConverter* registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler);

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
    SimTime_t getCurrentSimTime(TimeConverter *tc) const;
    /** return the time since the simulation began in the default timebase */
    inline SimTime_t getCurrentSimTime()  const{
        return getCurrentSimTime(defaultTimeBase);
    }
    /** return the time since the simulation began in timebase specified
        @param base Timebase frequency in SI Units */
    SimTime_t getCurrentSimTime(std::string base);

    /** Utility function to return the time since the simulation began in nanoseconds */ 
    SimTime_t getCurrentSimTimeNano() const;
    /** Utility function to return the time since the simulation began in microseconds */ 
    SimTime_t getCurrentSimTimeMicro() const;
    /** Utility function to return the time since the simulation began in milliseconds */ 
    SimTime_t getCurrentSimTimeMilli() const;

    /** Registers a statistic.  
        If Statistic is allowed to run (controlled by Python runtime parameters), 
        then a statistic will be created and returned. If not allowed to run,
        then a NullStatistic will be returned.  In either case, the returned 
        value should be used for all future Statistic calls.  The type of 
        Statistic and the Collection Rate is set by Python runtime parameters.
        If no type is defined, then an Accumulator Statistic will be provided 
        by default.  If rate set to 0 or not provided, then the statistic will 
        output results only at end of sim (if output is enabled).  
        @param statName Primary name of the statistic.  This name must match the 
               defined ElementInfoStatistic in the component, and must also 
               be enabled in the Python input file.
        @param statSubId An additional sub name for the statistic 
        @return Either a created statistic of desired type or a NullStatistic 
                depending upon runtime settings.
    */
    template <typename T>
    Statistic<T>* registerStatisticCore(std::string statName, std::string statSubId = "")
    {
        // NOTE: Templated Code for implementation of Statistic Registration
        // is in the componentregisterstat_impl.h file.  This was done
        // to avoid code bloat in the .h file.  
        #include "sst/core/statapi/componentregisterstat_impl.h"
    }
    
    template <typename T>
    Statistic<T>* registerStatistic(std::string statName, std::string statSubId = "")
    {
        // Verify here that name of the stat is one of the registered
        // names of the component's ElementInfoStatistic.  
        if (false == doesComponentInfoStatisticExist(statName)) {
            printf("Error: Statistic %s name %s is not found in ElementInfoStatistic, exiting...\n",
                   StatisticBase::buildStatisticFullName(getName().c_str(), statName, statSubId).c_str(),
                   statName.c_str());
            exit(1);
        }
        return registerStatisticCore<T>(statName, statSubId);
    }

    template <typename T>
    Statistic<T>* registerStatistic(const char* statName, const char* statSubId = "")
    {   
        return registerStatistic<T>(std::string(statName), std::string(statSubId));
    }
    
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
    
    /** Loads a SubComponent from an element Library
     * @param type Fully Qualified library.moduleName
     * @param comp Pointer to component to pass to SuComponent's constructor
     * @param params Parameters the module should use for configuration
     * @return handle to new instance of SubComponent, or NULL on failure.
     */
    SubComponent* loadSubComponent(std::string type, Component* comp, Params& params);
    
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


    /** Find a lookup table */
    SharedRegion* getLocalSharedRegion(const std::string &key, size_t size);
    SharedRegion* getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger = NULL);

    /* Get the Simulation */
    Simulation* getSimulation() const { return sim; }

private:

    friend class SubComponent;

    void addSelfLink(std::string name);

    // Does the statisticName exist in the ElementInfoStatistic 
    bool doesComponentInfoStatisticExist(std::string statisticName);
    // Return the EnableLevel for the statisticName from the ElementInfoStatistic 
    uint8_t getComponentInfoStatisticEnableLevel(std::string statisticName);
    // Return the Units for the statisticName from the ElementInfoStatistic 
    std::string getComponentInfoStatisticUnits(std::string statisticName);

    /** Unique ID */
    ComponentId_t   id;
	/* std::string name; */
    ComponentInfo* my_info;
    Simulation *sim;

    // LinkMap* myLinks;
    std::string currentlyLoadingSubComponent;



    template <typename T>
    Statistic<T>* CreateStatistic(Component* comp, std::string& type, std::string& statName, std::string& statSubId, Params& params)
    {
        // Load one of the SST Core provided Statistics
        // NOTE: This happens here (in simulation) instead of the factory because 
        //       it is a templated method.  The Component::registerStatistic<T>() 
        //       must be defined in the component.h however the the Factory is 
        //       not available from the Simulation::::getSimulation() because
        //       Factory is only defined via a forwarded definition.  Basically
        //       we have to go through some twists and jumps to make this work.        

        // Names of sst.xxx Statistics
        if (0 == strcasecmp("sst.nullstatistic", type.c_str())) {
            return new NullStatistic<T>(comp, statName, statSubId, params);
        }

        if (0 == strcasecmp("sst.accumulatorstatistic", type.c_str())) {
            return new AccumulatorStatistic<T>(comp, statName, statSubId, params);
        }

        if (0 == strcasecmp("sst.histogramstatistic", type.c_str())) {
            return new HistogramStatistic<T>(comp, statName, statSubId, params);
        }

	if(0 == strcasecmp("sst.uniquecountstatistic", type.c_str())) {
	    return new UniqueCountStatistic<T>(comp, statName, statSubId, params);
	}

        // We did not find this statistic
        printf("ERROR: Statistic %s is not supported by the SST Core...\n", type.c_str());

        return NULL;
    }



    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
};

} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Component)

#endif // SST_CORE_COMPONENT_H
