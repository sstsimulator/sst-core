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


#ifndef SST_INTROSPECTED_COMPONENT_H
#define SST_INTROSPECTED_COMPONENT_H

#include <cmath>
#include <iostream>
#include <list>
#include <deque>
#include <map>

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

#include <sst/core/component.h>
#include <sst/core/sst_types.h>
//#include <sst/core/linkMap.h>
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

class Introspector;

typedef boost::numeric::interval<double> I;

typedef struct {
    I il1, il2, dl1, dl2, itlb, dtlb;
    I clock, bpred, rf, io, logic;
    I alu, fpu, mult, ib, issueQ, decoder, bypass, exeu;
    I pipeline, lsq, rat, rob, btb, L2, mc;
    I router, loadQ, renameU, schedulerU, L3, L1dir, L2dir;
} itemized_t;

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
    itemized_t itemizedRuntimeDynamicPower;
    itemized_t itemizedLeakagePower;
    itemized_t itemizedCurrentPower;
    itemized_t itemizedTDP;
    itemized_t itemizedPeak;
    itemized_t itemizedTotalPower; //total energy
    Time_t currentSimTime;
}Pdissipation_t;


typedef std::map<ComponentId_t, Pdissipation_t> PowerDatabase;

/**
 * Main component object for the simulation. 
 *  All models inherit from this. 
 */
class IntrospectedComponent : public Component {
public:
    typedef  std::map<std::string,std::string> Params_t;
    
    typedef std::map<std::string, int> Monitors;


    /** List of common statistics that components want to be monitored.
        When editting the list, make sure to edit the vector, stats_name, 
	in introspectedComponent::getDataID() as well. */
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
        cache_load_lookups, cache_load_misses, cache_store_lookups, cache_store_misses, writeback_lookups, 
        writeback_misses, prefetch_lookups, prefetch_misses, 
        prefetch_insertions, prefetch_useful_insertions, MSHR_occupancy /* Miss Status Handling Registers total occupancy */,
        MSHR_full_cycles /* number of cycles when full */, WBB_insertions /* total writebacks */, WBB_victim_insertions /* total non-dirty insertions */,
        WBB_combines /* number eliminated due to write combining */, WBB_occupancy /* total occupancy */, WBB_full_cycles /* number of cycles when full */,
        WBB_hits, WBB_victim_hits, core_lookups, core_misses, MSHR_combos,
	/*IntSim*/
	ib_access, issueQ_access, decoder_access, pipeline_access, lsq_access,
	rat_access, rob_access, btb_access, l2_access, mc_access,
	loadQ_access, rename_access, scheduler_access, l3_access, l1dir_access, l2dir_access,
	/*DRAMSim*/
	dram_backgroundEnergy, dram_burstEnergy, dram_actpreEnergy, dram_refreshEnergy,
	/*router/NoC*/
	router_delay,
	/*power data*/
	current_power, leakage_power, runtime_power, total_power, peak_power     }; 

    /** Constructor. Generally only called by the factory class. 
        @param id Unique component ID
        @param sim Pointer to the global simulation object */
    IntrospectedComponent( ComponentId_t id );
    virtual ~IntrospectedComponent() {} 

    /** List of id of introspectors that monitor this component. */
    std::list<Introspector*> MyIntroList;

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
    std::pair<bool, Pdissipation_t> readPowerStats(Component* c);


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
    void addToIntroList(Introspector *introspector);
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
        This function is usually called in introspectedComponent::registerMonitorInt() or Component::registerMonitorDouble().
        @param dataName Description of the integer data*/
    int getDataID(std::string dataName);

protected:
    /** Database of integer monitors (arbitrary integer data that a
	compopent wishes to be monitored) available through
	introspectedComponent::getMonitorIntData() */	
    Monitors monitorINT;
    /** Database of double monitors (arbitrary double data that a
	compopent wishes to be monitored) available through
	introspectedComponent::getMonitorDoubleData() */	
    Monitors monitorDOUBLE;
	
private:
    IntrospectedComponent(); // For serialization only
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
}; 

}

BOOST_CLASS_EXPORT_KEY(SST::IntrospectedComponent)

#endif
