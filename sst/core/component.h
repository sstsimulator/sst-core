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

#include <cmath>
#include <iostream>
#include <list>

#if defined(__x86_64__) && defined(__APPLE__) && !defined(__USE_ISOC99)
// Boost interval sometimes doesn't detect the correct method for
// manipulating FP rounting on MacOS
#define __USE_ISOC99 1
#include <boost/numeric/interval.hpp>
#undef __USE_ISOC99
#else
#include <boost/numeric/interval.hpp>
#endif
#include <boost/io/ios_state.hpp>

#include <sst/core/sst.h>
#include <sst/core/linkMap.h>
#include <sst/core/clockHandler.h>
#include <sst/core/timeConverter.h>

namespace io_interval {  // from boost interval io example (io_wide)
template<class T, class Policies, class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits> 
&operator<<(std::basic_ostream<CharType, CharTraits> &stream,
            const boost::numeric::interval<T, Policies> &value)
{
    if (empty(value)) {
        return stream << "nothing";
    } else if (singleton(value)) {
        boost::io::ios_precision_saver state(stream, std::numeric_limits<T>::digits10);
        return stream << lower(value);
    } else if (zero_in(value)) {
        return stream << "0~";
    } else {
        std::streamsize p = stream.precision();
        p = (p > 15) ? 15 : p - 1;
        double eps = 1.0; for(; p > 0; --p) { eps /= 10; }
        T eps2 = static_cast<T>(eps / 2) * norm(value);
        boost::numeric::interval<T, Policies> r = widen(value, eps2);
        //return stream << '[' << lower(r) << ',' << upper(r) << ']';
        return stream << median(r) << " ± " << width(r)/2;
    }
}
}

namespace SST {

#define _COMP_DBG( fmt, args...) __DBG( DBG_COMP, Component, fmt, ## args )

typedef boost::numeric::interval<double> I;

typedef struct 
{
	//I internalPower;
	//I switchingPower;
	I TDP; //thermal dynamic power
	I runtimeDynamicPower;
	I leakagePower; //=threshold leakage + gate leakage
	I peak;
	I currentPower; //=leakage + rumtimeDynamic
	I averagePower;
	I totalEnergy;
	int currentCycle;
}Pdissipation_t;

typedef std::map<ComponentId_t, Pdissipation_t> PowerDatabase;


/**
 * Main component object for the simulation. 
 *  All models inherit from this. 
 */
class Component : public LinkMap {
public:
    typedef  std::map<std::string,std::string> Params_t;
    
    typedef std::map<std::string, int> Monitors;


    /** List of common statistics that components want to be monitored.
        When editting the list, make sure to edit the vector, stats_name, 
	in Component::getDataID() as well. */
    enum stats {/*McPAT counters*/
	    core_temperature,branch_read, branch_write, RAS_read, RAS_write,
	    il1_read, il1_readmiss, IB_read, IB_write, BTB_read, BTB_write,
	    int_win_read, int_win_write, fp_win_read, fp_win_write, ROB_read, ROB_write,
	    iFRAT_read, iFRAT_write, iFRAT_search, fFRAT_read, fFRAT_write, fFRAT_search, iRRAT_write, fRRAT_write,
	    ifreeL_read, ifreeL_write, ffreeL_read, ffreeL_write, idcl_read, fdcl_read,
	    dl1_read, dl1_readmiss, dl1_write, dl1_writemiss, LSQ_read, LSQ_write,
	    itlb_read, itlb_readmiss, dtlb_read, dtlb_readmiss,
	    int_regfile_reads, int_regfile_writes, float_regfile_reads, float_regfile_writes, RFWIN_read, RFWIN_write,
	    bypass_access, router_access,
	    L2_read, L2_readmiss, L2_write, L2_writemiss, L3_read, L3_readmiss, L3_write, L3_writemiss,
	    L1Dir_read, L1Dir_readmiss, L1Dir_write, L1Dir_writemiss, L2Dir_read, L2Dir_readmiss, L2Dir_write, L2Dir_writemiss,
	    memctrl_read, memctrl_write,
	    /*sim-panalyzer counters*/
	    alu_access, fpu_access, mult_access, io_access,
	    /*zesto counters */
	    cache_load_lookups, cache_load_misses, cache_store_lookups, cache_store_misses, writeback_lookups, writeback_misses, prefetch_lookups, prefetch_misses, 
	    prefetch_insertions, prefetch_useful_insertions, MSHR_occupancy /* Miss Status Handling Registers total occupancy */,
	    MSHR_full_cycles /* number of cycles when full */, WBB_insertions /* total writebacks */, WBB_victim_insertions /* total non-dirty insertions */,
    	    WBB_combines /* number eliminated due to write combining */, WBB_occupancy /* total occupancy */, WBB_full_cycles /* number of cycles when full */,
    	    WBB_hits, WBB_victim_hits, core_lookups, core_misses, MSHR_combos
    }; 


    /** Constructor. Generally only called by the factory class. 
        @param id Unique component ID
        @param sim Pointer to the global simulation object */
    Component( ComponentId_t id );
    virtual ~Component() = 0;
    /** Returns unique component ID */
    inline ComponentId_t Id() { return _id; }
    /** Component's type, set by the factory when the object is created.
        It is identical to the configuration string used to create the
        component. I.e. the XML "<component id="aFoo"><foo>..." would
        generate a component from libfoo.so and set component::type to
        "foo" */
    std::string type;
    /** List of id of introspectors that monitor this component. */
        std::list<ComponentId_t> MyIntroList;


    /** Called after all components have been constructed, but before
        simulation time has begun. */
    virtual int Setup( ) { return 0; }
    /** Called after simulation completes, but before objects are
        destroyed. A good place to print out statistics. */
    virtual int Finish( ) { return 0; }

    virtual bool Status( ) { return 0; }

    // Power
    /** Central power/energy database that stores power dissipation data 
        (including current power, total energy, peak power, etc) of the components 
        on the same rank.*/
    static PowerDatabase PDB;
    /** Register/update power dissipation data in the central power database 
        @param pusage Structure that contains power dissipation data of a component */
    void regPowerStats(Pdissipation_t pusage);	    
    /** Read power dissipation data of this component from database 
        @param c Pointer to the component that one whats to query power
                 from the central power database */
    Pdissipation_t readPowerStats(Component* c);

    //Introspector
	/** Get the period set by defaultTimeBase, which is usually set by Component::registerClock().
	    This can be used by clever components to ensure they only compute statistics data when needed.
	    Returns the time between two handler function calls. (For introspectors, this means time
	    between introspections.) */
	SimTime_t getFreq() {return defaultTimeBase->getFactor();}
	/** Add the statistical integer data to the map of monitors 
	    for this component. The map, monitorINT, stores both the name and the ID of the integer data.
	    @param paramName Description of the integer data*/
	void registerMonitorInt(std::string dataName);
	/** Arbitrary statistical integer data monitoring. Returns a pair<> which indicates
            if the component has a monitor associated with the string
            indicated by "dataName", and (if so) the ID of the integer data.
	    @param dataName Description of the integer data*/
	std::pair<bool, int> ifMonitorIntData(std::string dataName);
	/** Add the statistical double data to the map of monitors 
	    for this component. The map, monitorDOUBLE, stores both the name and the ID of the double data.
	    @param dataName Description of the double data*/	
	void registerMonitorDouble(std::string dataName);
	/** Arbitrary statistical double data monitoring. Returns a pair<> which indicates
            if the component has a monitor associated with the string
            indicated by "dataName", and (if so) the ID of the double data.
	    @param dataName Description of the double data*/
	std::pair<bool, int> ifMonitorDoubleData(std::string dataName);
	/** Add the "id" of a introspector to an internal list, MyIntroList.
	    Indicates the introspector monitors the component.
	    @param id ID of the introspector that monitors the component*/
	void addToIntroList(ComponentId_t id);
	/** Check if current is the time for the component to push/report data (e.g. power)
	    by querying its introspector.
	    @param current Current cycle from component's view
	    @param type Component type of the introspector that component queries about push frequency*/ 
	bool isTimeToPush(Cycle_t current, const char *type);
	/** Return the value of the integer data indicated by "dataID" and "index" (if the data structure is a table).
	    Each component type needs to implement its own getIntData. The function is usually called by introspector pull
	    mechanism.
	    @param dataID ID of the integer data
	    @param index of the table (if the data structure is a table); default is set to 0 */ 
	virtual uint64_t getIntData(int dataID, int index=0){ return 0;}
	/** Return the value of the double data indicated by "dataID" and "index" (if the data structure is a table).
	    Each component type needs to implement its own getDoubleData. The function is usually called by introspector pull
	    mechanism.
	    @param dataID ID of the integer data
	    @param index of the table (if the data structure is a table); default is set to 0 */ 
	virtual double getDoubleData(int dataID, int index=0){ return 0;}
	/** Returns the ID of the data associated with the string indicated by "dataName".
	    This function is usually called in Component::registerMonitorInt() or Component::registerMonitorDouble().
	    @param dataName Description of the integer data*/
	int getDataID(std::string dataName);

protected:
    /** Unique ID */
    ComponentId_t   _id;
    /** Timebase used if no other timebase is specified for calls like
        Component::getCurrentSimTime(). Often set by Component::registerClock()
        function */
    TimeConverter* defaultTimeBase;

    Component();

    /** Manually set the default detaulTimeBase */ 
    void setDefaultTimeBase(TimeConverter *tc) {
        defaultTimeBase = tc;
    }

    /** Database of integer monitors (arbitrary integer data that a
	compopent wishes to be monitored) available through
	Component::getMonitorIntData() */	
    Monitors monitorINT;
    /** Database of double monitors (arbitrary double data that a
	compopent wishes to be monitored) available through
	Component::getMonitorDoubleData() */	
    Monitors monitorDOUBLE;

public:
    /** Registers a clock for this component.
        @param freq Frequency for the clock in SI units
        @param handler Pointer to ClockHandler_t which is to be invoked
        at the specified interval
        @param regAll Should this clock perioud be used as the default
        time base for all of the links connected to this component
    */
    TimeConverter* registerClock( std::string freq, ClockHandler_t* handler, 
                                  bool regAll = true);
    void unregisterClock(TimeConverter *tc, ClockHandler_t* handler);

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
	
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( LinkMap );
        ar & BOOST_SERIALIZATION_NVP( _id );
    }
}; 

typedef std::map< ComponentId_t, Component* > CompMap_t;
typedef std::deque< Component* > CompQueue_t;

inline int
Connect(Component* c1, std::string c1_name, Cycle_t lat1,
        Component* c2, std::string c2_name, Cycle_t lat2 )
{
    _COMP_DBG( "(forward) Connecting link \"%s\" with link \"%s\"\n", c1_name.c_str(), c2_name.c_str() );
    Link *l2= c2->LinkGet( c2_name );
    if ( l2 == NULL ) return 1;
    c1->LinkConnect( c1_name, l2, lat1 );

    _COMP_DBG( "(reverse) Connecting link \"%s\" with link \"%s\"\n", c2_name.c_str(), c1_name.c_str() );
    Link *l1= c1->LinkGet( c1_name );
    if ( l1 == NULL ) return 1;
    c2->LinkConnect( c2_name, l1, lat2 );

    return 0;
}

inline int
Connect(Component* c1, std::string c1_name,
        Component* c2, std::string c2_name )
{
  return Connect(c1,c1_name,1,c2,c2_name,1);
}

}

#endif
