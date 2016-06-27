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

#include "sst_config.h"
#include "sst/core/introspector.h"

//#include <boost/foreach.hpp>
#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif

//#include "sst/core/exit.h"
#include "sst/core/componentInfo.h"
#include "sst/core/introspectAction.h"
#include "sst/core/simulation.h"
//#include "sst/core/timeLord.h"


namespace SST {


Introspector::Introspector()
{
    int size = 1;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif
    arrayvalue = new uint64_t[size];
}

TimeConverter*
Introspector::registerClock( std::string freq, Clock::HandlerBase* handler)
{
    defaultTimeBase = Simulation::getSimulation()->registerClock(freq,handler);
    return defaultTimeBase;
}


std::list<IntrospectedComponent*> Introspector::getModelsByName(const std::string CompName)
{
    Component* comp = Simulation::getSimulation()->getComponent(CompName);
    
    if (comp != NULL) {
            MyCompList.push_back(dynamic_cast<IntrospectedComponent*>(comp));
     } 

    return MyCompList;
}


std::list<IntrospectedComponent*> Introspector::getModelsByType(const std::string CompType)
{
    const ComponentInfoMap& CompMap = Simulation::getSimulation()->getComponentInfoMap();

    for( auto iter = CompMap.begin(); iter != CompMap.end(); ++iter ) {
        if (CompType.empty() == true)
            MyCompList.push_back(dynamic_cast<IntrospectedComponent*>((*iter)->getComponent()));   
        else if ( (*iter)->getType().compare(CompType) == 0){
            MyCompList.push_back(dynamic_cast<IntrospectedComponent*>((*iter)->getComponent()));
        }
    }
    return MyCompList;
}



void
Introspector::collectInt(collect_type ctype, uint64_t invalue, mpi_operation op, int rank)
{
#ifdef SST_CONFIG_HAVE_MPI
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        
    switch(ctype)
    {
    case 0:  //gather
	    // if (my_rank == 0) {
    	// 	 // gather( world, invalue, arrayvalue, 0); 
  	    // } else {
    	// 	 gather(world, invalue, 0);
  	    // }
        MPI_Gather( &invalue, 1, MPI_UINT64_T,
                    &arrayvalue, world_size, MPI_UINT64_T,
                    0, MPI_COMM_WORLD); 
	    break;	
	case 1:  //all gather
	    // all_gather( world, invalue, arrayvalue);
        MPI_Allgather( &invalue, 1, MPI_UINT64_T,
                       &arrayvalue, world_size, MPI_UINT64_T,
                       MPI_COMM_WORLD); 
	    break;	
	case 2:  //broadcast
	    if (my_rank == rank)
	        value = invalue;
	    // broadcast(world, value, rank);
        MPI_Bcast( &value, 1, MPI_UINT64_T, rank, MPI_COMM_WORLD );
	    break;	
	case 3:  //reduce
	    switch(op)
	    {
		case 0: //minimum
            MPI_Reduce(&invalue, &minvalue, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD);
		    break;
		case 1: //maximum
            MPI_Reduce(&invalue, &maxvalue, 1, MPI_UINT64_T, MPI_MAX, 0, MPI_COMM_WORLD);
		    break;
		case 2: //sum
            MPI_Reduce(&invalue, &value, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
		    break;
		default:
		    break;
	    }
	    break;
	case 4:  //all reduce
	    switch(op)
        {
		case 0: //minimum	            
            MPI_Allreduce(&invalue, &minvalue, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD);
		    break;
		case 1: //maximum            
            MPI_Allreduce(&invalue, &maxvalue, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
		    break;
		case 2: //sum   
            MPI_Allreduce(&invalue, &value, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
		    break;
		default:
		    break;
	    }
	    break;	
	default:
	    break;	
    }
#else
    switch(ctype)
    {
    case 0:  //gather
	case 1:  //all gather
        arrayvalue[0] = invalue;
	    break;	
	case 2:  //broadcast
        value = invalue;
	    break;	
	case 3:  //reduce
	    switch(op)
	    {
		case 0: //minimum
            minvalue = invalue;
            break;
		case 1: //maximum
            maxvalue = invalue;
		    break;
		case 2: //sum
            value = invalue;
		    break;
		default:
		    break;
	    }
	    break;
	case 4:  //all reduce
	    switch(op)
	    {
		case 0: //minimum	            
            minvalue = invalue;
		    break;
		case 1: //maximum            
            maxvalue = invalue;
		    break;
		case 2: //sum   
            value = invalue;
		    break;
		default:
		    break;
	    }
	    break;	
	default:
	    break;	
    }
#endif
}


void Introspector::oneTimeCollect(SimTime_t time, Event::HandlerBase* functor){

	Simulation *sim = Simulation::getSimulation();
	IntrospectAction* act = new IntrospectAction(functor);

	sim->insertActivity(time, act); 
}

} //namespace SST
