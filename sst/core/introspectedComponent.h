// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_INTROSPECTED_COMPONENT_H
#define SST_CORE_INTROSPECTED_COMPONENT_H
#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <cmath>
//#include <deque>
#include <iostream>
#include <list>
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
#include <boost/any.hpp>

#include <sst/core/component.h>
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

using boost::any_cast;

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


    //Introspection
    // Functor classes for monitor handling
    ////template <typename returnT>
    class MonitorBase {
    public:
	////virtual returnT operator()() = 0;
	virtual boost::any operator()() = 0;
	virtual ~MonitorBase() {}
    };
    

    template <typename classT, typename returnT, typename argT = void>
    class MonitorFunction : public MonitorBase{
    private:
	typedef returnT (classT::*PtrMember)(argT);
	classT* object;
	const PtrMember member;
	argT data;
	
    public:
	MonitorFunction( classT* const object, PtrMember member, argT data ) :
	    object(object),
	    member(member),
	    data(data)
	{}

	    /*returnT operator()() {
          	 return (const_cast<classT*>(object)->*member)(data);
	    }*/
	    boost::any operator()() {
          	 return static_cast<boost::any>( (const_cast<classT*>(object)->*member)(data) );
	    }
    };
    
    template <typename classT, typename returnT>
    class MonitorFunction<classT, returnT, void> : public MonitorBase{
    private:
	typedef returnT (classT::*PtrMember)();
	const PtrMember member;
	classT* object;
	
    public:
	MonitorFunction( classT* const object, PtrMember member ) :
  	  member(member),
	  object(object)
	{}

	    /*returnT operator()() {
		return (const_cast<classT*>(object)->*member)();
	    }*/
	    boost::any operator()() {
		return static_cast<boost::any>( (const_cast<classT*>(object)->*member)() );
	    }
    };

    template <typename ptrT, typename idxT = void>
    class MonitorPointer : public MonitorBase{
    private:
	ptrT *data;
	idxT index;
	
    public:
	MonitorPointer( ptrT* const data, idxT index ) :
	    data(data),
	    index(index)
	{}

	    boost::any operator()() {
          	 return static_cast<boost::any>( *data[index] );
	    }
    };

    template <typename ptrT>
    class MonitorPointer<ptrT, void> : public MonitorBase{
    private:
	ptrT* data;
	
    public:
	MonitorPointer( ptrT* const data ) :
	    data(data)
	{}
	    boost::any operator()() {
          	 //return static_cast<boost::any>( const_cast<ptrT*>(data) );
		 return static_cast<boost::any>( *data );
	    }
    };

    /** Add the pointer to a introspector to an internal list, MyIntroList.
        Indicates the introspector monitors the component.
        @param name Name of the introspector that monitors the component*/
    void registerIntrospector(std::string name);
    /** Add the data to the map of monitors to specify which data to be monitored
        . The map, monitorMap, stores both the name and the handler of the data.
        @param dataName Description of the data
	@param handler Pointer to IntrospectedComponent::MonitorBase which is to be invoked
		in Introspector::getData()*/
    void registerMonitor(std::string dataName, IntrospectedComponent::MonitorBase* handler);      
    /** Find monitor indicated by "dataname" from the map. This is called in Introspector::getData()
	@param dataname name of the data*/
    std::pair<bool, IntrospectedComponent::MonitorBase*> getMonitor(std::string dataname);
    /** 'Component-push' mechanism. Component asks  its introspector(s) to pull data in.*/
    void triggerUpdate();

    /** Get the period set by defaultTimeBase, which is usually set by Component::registerClock().
        This can be used by clever components to ensure they only compute statistics data when needed.
        Returns the time between two handler function calls. (For introspectors, this means time
        between introspections.) */
    SimTime_t getFreq() {return defaultTimeBase->getFactor();}
    
    
    /** Check if current is the time for the component to push/report data (e.g. power)
        by querying its introspector.
        @param current Current cycle from component's view
        @param type Name of the introspector that component queries about push frequency*/ 
    bool isTimeToPush(Cycle_t current, const char *name);
    /** Return the value of the integer data indicated by "dataID" and "index" (if the data structure is a table).
        Each component type needs to implement its own getIntData. The function is usually called by introspector pull
        mechanism.
        @param dataID ID of the integer data
        @param index of the table (if the data structure is a table); default is set to 0 */ 
    //virtual uint64_t getIntData(int dataID, int index=0){ return 0;}
    /** Return the value of the double data indicated by "dataID" and "index" (if the data structure is a table).
        Each component type needs to implement its own getDoubleData. The function is usually called by introspector pull
        mechanism.
        @param dataID ID of the integer data
        @param index of the table (if the data structure is a table); default is set to 0 */ 
    //virtual double getDoubleData(int dataID, int index=0){ return 0;}
   

   
    typedef std::map<std::string, IntrospectedComponent::MonitorBase*> MonitorMap_t;

protected:
    /** Database of monitors (arbitrary data that a
	compopent wishes to be monitored) available through
	introspectedComponent::MonitorBase* getMonitor() */
    MonitorMap_t monitorMap;
    
	
protected:
    IntrospectedComponent(); // For serialization only
    
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
}; 

} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::IntrospectedComponent)

#endif // SST_CORE_INTROSPECTED_COMPONENT_H
