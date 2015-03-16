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

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/introspector.h"

//#include <boost/foreach.hpp>
#ifdef HAVE_MPI
#include <boost/mpi.hpp>
#endif

//#include "sst/core/exit.h"
#include "sst/core/introspectAction.h"
#include "sst/core/simulation.h"
//#include "sst/core/timeLord.h"


namespace SST {


Introspector::Introspector()
{
}

TimeConverter*
Introspector::registerClock( std::string freq, Clock::HandlerBase* handler)
{
    defaultTimeBase = Simulation::getSimulation()->registerClock(freq,handler);
    return defaultTimeBase;
}


std::list<IntrospectedComponent*> Introspector::getModelsByName(const std::string CompName)
{
    const CompMap_t &CompMap = Simulation::getSimulation()->getComponentMap();

     CompMap_t::const_iterator i = CompMap.find(CompName);
     if (i != CompMap.end()) {
            MyCompList.push_back(dynamic_cast<IntrospectedComponent*>(i->second));
     } 

    return MyCompList;
}


std::list<IntrospectedComponent*> Introspector::getModelsByType(const std::string CompType)
{
    const CompMap_t &CompMap = Simulation::getSimulation()->getComponentMap();

    for( CompMap_t::const_iterator iter = CompMap.begin();
    	                    iter != CompMap.end(); ++iter )
    {
	//printf("CompMap has %s with id = %lu\n", (*iter).second->type.c_str(), (*iter).second->Id());
        if (CompType.empty() == true)
	    MyCompList.push_back(dynamic_cast<IntrospectedComponent*>((*iter).second));   
	else if ( (*iter).second->type.compare(CompType) == 0){
	    //printf("Introspector will monitor ID %lu\n", (*iter).second->Id());
	    MyCompList.push_back(dynamic_cast<IntrospectedComponent*>((*iter).second));
	}
    }
    return MyCompList;
}



void
Introspector::collectInt(collect_type ctype, uint64_t invalue, mpi_operation op, int rank)
{
#ifdef HAVE_MPI
    boost::mpi::communicator world; 

    switch(ctype)
    {
        case 0:  //gather
	    if (world.rank() == 0) {
    		 gather( world, invalue, arrayvalue, 0); 
  	    } else {
    		 gather(world, invalue, 0);
  	    }
	    break;	
	case 1:  //all gather
	    all_gather( world, invalue, arrayvalue);
	    break;	
	case 2:  //broadcast
	    if (world.rank() == rank)
	        value = invalue;
	    broadcast(world, value, rank);
	    break;	
	case 3:  //reduce
	    switch(op)
	    {
		case 0: //minimum
	            if (world.rank() == 0) {
    		        reduce( world, invalue, minvalue, boost::mpi::minimum<int>(), 0); 
    		        //std::cout << "The minimum value is " << minvalue << std::endl;
  	            } else {
    		        reduce(world, invalue, boost::mpi::minimum<int>(), 0);
  	            }
		    break;
		case 1: //maximum
	            if (world.rank() == 0) {
    		        reduce( world, invalue, maxvalue, boost::mpi::maximum<int>(), 0); 
    		        //std::cout << "The maximum value is " << maxvalue << std::endl;
  	            } else {
    		        reduce(world, invalue, boost::mpi::maximum<int>(), 0);
  	            }
		    break;
		case 2: //sum
		    if (world.rank() == 0) {
    		        reduce( world, invalue, value, std::plus<int>(), 0); 
    		        //std::cout << "The value is " << value << std::endl;
  	            } else {
    		        reduce(world, invalue, std::plus<int>(), 0);
  	            }
		    break;
		default:
		    break;
	    }
	    break;
	case 4:  //all reduce
	    switch(op)
	    {
		case 0: //minimum	            
    		        all_reduce( world, invalue, minvalue, boost::mpi::minimum<int>()); 
		    break;
		case 1: //maximum            
    		        all_reduce( world, invalue, maxvalue, boost::mpi::maximum<int>());       
		    break;
		case 2: //sum   
    		        all_reduce( world, invalue, value, std::plus<int>()); 
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

template<class Archive>
void
Introspector::serialize(Archive& ar, const unsigned int version) {
//    ar & BOOST_SERIALIZATION_NVP(MyCompList);
    ar & BOOST_SERIALIZATION_NVP(minvalue);
    ar & BOOST_SERIALIZATION_NVP(maxvalue);
    ar & BOOST_SERIALIZATION_NVP(value);
    ar & BOOST_SERIALIZATION_NVP(arrayvalue);
    ar & BOOST_SERIALIZATION_NVP(defaultTimeBase);
}

} //namespace SST

SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Introspector::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Introspector)
