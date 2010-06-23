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


#ifndef SST_CORE_INTROSPECTOR_H
#define SST_CORE_INTROSPECTOR_H

#include "sst/core/component.h"
#include "sst/core/simulation.h"

namespace SST {

#if DBG_INTROSPECTOR
#define _INTROSPECTOR_DBG( fmt, args...)\
         printf( "%d:Introspector::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _INTROSPECTOR_DBG( fmt, args...)
#endif



/**
 * Main introspector object for the simulation. 
 *  All models inherit from this. 
 * Introspection interface is a unified way to gather statistics and arbitrary data from components.
 */
class Introspector: public Component {
       
    public:
	typedef std::map<Component*, int> Database_t;	

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
        Introspector( ComponentId_t id );
        virtual ~Introspector() = 0;
        
   

	// main functions for introspection
	/** Get the map, compMap, that stores ID and pointers to the components on the rank from Simulation.
	    This function is usually called in Introspector::getModels(). */
	void getCompMap();
	/** Get component of a certain type indicated by CompType on this rank.
            If CompType is blank, a list of all local components is returned.
	    This function is usually used in Introspector::Setup().
	    @param CompType Component's type*/
	std::list<Component*> getModels(const std::string CompType);
	/** Declare that this introspector will be monitoring a given component. 
	    The information of the introspector is also stored in the component's MyIntroList.
	    This function is usually used in Introspector::Setup().
	    @param c Pointer to the component that will be monitored*/
	void monitorComponent(Component* c);
	/** Store pointer to the component and the data ID of the integer data of interest in a local map.
	    This function is usually used in Introspector::Setup().
	    @param c Pointer to the component that is monitored by this introspector
	    @param dataID ID of the integer data */
	void addToIntDatabase(Component* c, int dataID); 
	/** Store pointer to the component and the data ID of the double data of interest in a local map.
	    This function is usually used in Introspector::Setup().
	    @param c Pointer to the component that is monitored by this introspector
	    @param dataID ID of the double data */
	void addToDoubleDatabase(Component* c, int dataID);
	/** Query the components it is moniroting at regular intervals to retrieve components' statistics & data. 
	    Introspector-writers will implement their own pullData function. This function calls Component::getIntData()
	    or Component::getDoubleData() to retrieve components' data, and is a good place to manipulate the data 
	    (print to screen, MPI collective communication, etc). 
	    This function can be invoked by an event handler triggered by a clock.*/
	virtual bool pullData( Cycle_t ) { return false; } 
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
	void oneTimeCollect(SimTime_t time, EventHandlerBase<bool,Event*>* functor); 


	/** List of components that this introspector is monitoring.*/
	std::list<Component*> MyCompList;
	/** Database of the integer data monitored by this introspector available through Introspector::pullData().*/
	Database_t DatabaseInt;
	/** Database of the double data monitored by this introspector available through Introspector::pullData().*/
	Database_t DatabaseDouble;
	/** Minimum value of the integer values collected from all introspectors by Introspector::collectInt().*/
	uint64_t minvalue;
	/** Maximum value of the integer values collected from all introspectors by Introspector::collectInt().*/
	uint64_t maxvalue;
	/** Result value of the reduction operation in Introspector::collectInt().*/
	uint64_t value;
	/** Data vector that holds data collected from all introspectors by Introspector::collectInt().*/
	std::vector<uint64_t> arrayvalue;


    protected:
        Introspector();
    
    private:
	CompMap_t* simCompMap;
};


} //end namespace
#endif
