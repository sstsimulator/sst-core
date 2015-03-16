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

#ifndef SST_CORE_INTROSPECTOR_H
#define SST_CORE_INTROSPECTOR_H

#include <sst/core/serialization.h>

#include "sst/core/clock.h"
#include "sst/core/introspectedComponent.h"
//#include "sst/core/simulation.h"
//#include "sst/core/timeConverter.h"

namespace SST {

#if DBG_INTROSPECTOR
#define _INTROSPECTOR_DBG( fmt, args...)                                \
    printf( "%d:Introspector::%s():%d: " fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _INTROSPECTOR_DBG( fmt, args...)
#endif


/**
 * Main introspector object for the simulation. 
 *  All models inherit from this. 
 * Introspection interface is a unified way to gather statistics and arbitrary data from components.
 */

class Introspector {
public:

    /** Types of boost MPI collective operations that introspector can perform.*/ 
    enum collect_type { GATHER, ALLGATHER, BROADCAST, REDUCE, ALLREDUCE};
    /** Types of funciton objects for the reduce collective.*/
    enum mpi_operation {
        MINIMUM,
        MAXIMUM,
        SUM,
        NOT_APPLICABLE
    }; 

    /** Constructor. Generally only called by the factory class. 
        @param id Unique introspector ID*/
    Introspector();
    virtual ~Introspector() {}

    /** Called after all components/introspectors have been constructed, but before
        simulation time has begun. */
    virtual void setup( ) { }
    /** Called after simulation completes, but before objects are
        destroyed. A good place to print out statistics. */
    virtual void finish( ) { }
        
    /** Get component of a certain type indicated by CompName on this rank.
	Name is unique so the fuction actually returns a list with only one component.
        This function is usually used in Introspector::setup().
        @param CompName Component's name*/
    std::list<IntrospectedComponent*> getModelsByName(const std::string CompName); 
    /** Get component of a certain type indicated by CompType on this rank.
        If CompType is blank, a list of all local components is returned.
        This function is usually used in Introspector::setup().
        @param CompType Component's type*/
    std::list<IntrospectedComponent*> getModelsByType(const std::string CompType);

    /** Query the components indicated by "c" to retrieve components' statistics & data.
	Return the value of the data indicated by "dataname".
        The function is usually called by introspector-pull mechanism in Introspector::triggeredUpdate().
	@param c Pointer to the component
        @param dataname Name of the data */ 
    template <typename typeT> 
    typeT getData(IntrospectedComponent* c, std::string dataname);
     /** Introspector-writers will implement their own triggeredUpdate function.
	 This function calls Introspector::getData() to retrieve components' data, 
	and is a good place to manipulate the data 
	(print to screen, MPI collective communication, etc).  */
    virtual bool triggeredUpdate() { return false; } 
    
   
    /** Introspectors communicate among themselves with Boost MPI to exchange their integer data, invalue. 
        This function initiates a specific type of collective communicaiton indicated by ctype. The data 
        are operated based on ctype and on the MPI operation, op. An introspector type can have periodic 
        collective communication by calling this function in a member function registered with an event handler that is 
        triggered by a clock.
        @param ctype Types of collective communication. Currently supported options are Broadcast, (all)gather, 
        and (all)reduce
        @param invalue  The local value to be communicated
        @param op Types of the MPI operations for the (all)reduce algorithm to combine the values. Currently 
        supported options are summarize, minimum and maximum. Default is set to none
        @param rank The rank where the introspector resides. Default is set to 0. If ctype is broadcast, 
        rank here indicates the rank that will transmitting the value*/
    void collectInt(collect_type ctype, uint64_t invalue, mpi_operation op=NOT_APPLICABLE, int rank=0);
    /** One time introspectors collective communication. The event handling functors that calls a given member 
        communication function is inserted into the queue and will be triggered at time specified by, time. 
        The introspector-write implements their own communication function if they want something other than 
        the basic collective operations, broadcast, (all)reduce and (all)gather.
        @param time The simulation time when the introspectors will communicate among themselves
        @param functor Event handling functor that invokes member communication function*/
	void oneTimeCollect(SimTime_t time, Event::HandlerBase* functor); 


    /** List of components that this introspector is monitoring.*/
    std::list<IntrospectedComponent*> MyCompList;
    /** Minimum value of the integer values collected from all introspectors by Introspector::collectInt().*/
    uint64_t minvalue;
    /** Maximum value of the integer values collected from all introspectors by Introspector::collectInt().*/
    uint64_t maxvalue;
    /** Result value of the reduction operation in Introspector::collectInt().*/
    uint64_t value;
    /** Data vector that holds data collected from all introspectors by Introspector::collectInt().*/
    std::vector<uint64_t> arrayvalue;

    /** Registers a clock for this component.
        @param freq Frequency for the clock in SI units
        @param handler Pointer to Clock::HandlerBase which is to be invoked
        at the specified interval
        @param regAll Should this clock perioud be used as the default
        time base for all of the links connected to this component
    */
    TimeConverter* registerClock( std::string freq, Clock::HandlerBase* handler);
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);

    SimTime_t getFreq() {return defaultTimeBase->getFactor();}

protected:
    /** Timebase used if no other timebase is specified for calls like
        Component::getCurrentSimTime(). Often set by Component::registerClock()
        function */
    TimeConverter* defaultTimeBase;

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
};

template <typename typeT> 
typeT Introspector::getData(IntrospectedComponent* c, std::string dataname)
{
    IntrospectedComponent::MonitorBase* monitor;
    std::pair<bool, IntrospectedComponent::MonitorBase*> p;    
   
    p = c->getMonitor(dataname);

    //check if the component cares about this data
    if (p.first){
        monitor = p.second;
        return any_cast<typeT>( (*monitor)());
    }
    else 
	return (-9999); //return an unreasonable number for now
}

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Introspector)

#endif // SST_CORE_INTROSPECTOR_H
