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


#include <sst_config.h>

#include <boost/foreach.hpp>
#include <string.h>

#include "sst/core/component.h"
#include "sst/core/simulation.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/timeLord.h"

namespace SST {

PowerDatabase Component::PDB;


Component::~Component() {}

Component::Component(ComponentId_t id) :
    _id( id )
{
    myLinks = Simulation::getSimulation()->getComponentLinkMap(_id);
    _COMP_DBG( "new\n" );
}

/*****************************************************
* regPowerStats                                      *
* Register/update power statistics of this component * 
* in the central power database                      *
******************************************************/
void Component::regPowerStats(Pdissipation_t pusage) {	
	
	std::pair<PowerDatabase::iterator, bool> res = PDB.insert(std::make_pair( Id(), pusage));
	if ( ! res.second ) {
            // key already exists for the component; update power statistics instead
            //(res.first)->second.internalPower = pusage.internalPower;
            //(res.first)->second.switchingPower = pusage.switchingPower;
	    (res.first)->second.TDP = pusage.TDP;
	    (res.first)->second.runtimeDynamicPower = pusage.runtimeDynamicPower;
            (res.first)->second.leakagePower = pusage.leakagePower;
            (res.first)->second.peak = pusage.peak;
            (res.first)->second.currentPower = pusage.currentPower;
            (res.first)->second.totalEnergy = pusage.totalEnergy;
            (res.first)->second.currentCycle = pusage.currentCycle;
	}
	/* _COMP_DBG: for debugging purpose
	std::map<ComponentId_t, Pdissipation_t>::size_type i;
	i = PDB.size();	
        std::cout << "ID " << Id() << " is regPower. Current PDB map length is " << i << std::endl;
	*/
}

/********************************************************
* readPowerStas                                         *
* read power statistics of this component from database *
*********************************************************/
Pdissipation_t Component::readPowerStats(Component* c) {

	std::map<ComponentId_t, Pdissipation_t>::iterator it;
	Pdissipation_t pusage;
	memset(&pusage,0,sizeof(Pdissipation_t));

	/* _COMP_DBG: for debugging purpose	
	std::map<ComponentId_t, Pdissipation_t>::size_type i;
	std::cout<< "ID: " << c->Id() << " enter readPowerStats"<<std::endl;
	it = PDB.begin(); 
            while( it != PDB.end() ) { 
                std::cout<< c->Id() << ": key = " << it->first <<", value = " << it->second.currentPower << std::endl; 
		it++;
	    }
	i = PDB.size();
        std::cout << "The PDB map length is " << i << std::endl;
	*/

	it = PDB.find(c->Id());
        if (it != PDB.end() )   // found key
            return it->second;
	
	else {
	    std::cout << "Component::readPowerStats--ID: " << c->Id() << "can't find key" << std::endl;
	    return pusage;
	}

}


/*************************************************************
* registerMonitorInt                                         *
* add to the map of monitors (type of generator is integer)  *
**************************************************************/
void Component::registerMonitorInt(std::string dataName) {
	int dataID;

	dataID = getDataID(dataName);
	std::pair<Monitors::iterator, bool> res = monitorINT.insert(std::make_pair(dataName, dataID));
	if ( ! res.second ) {
            // key already exists for the component
	    std::cout<< "Component::registerMonitorInt-- generator name " << (res.first)->first << " already exists." << std::endl;
	    exit(1);
	}
}

/*************************************************************
* ifMonitorIntData		                                     *
* arbitrary statistics (integer) monitoring		     *
**************************************************************/
std::pair<bool, int> Component::ifMonitorIntData(std::string dataName) {

	std::map<std::string, int>::iterator it;
	bool flag = false;
	
	it = monitorINT.find(dataName);
        if (it != monitorINT.end() )   // found key
            flag = true;
	else
	    flag = false;

	return(std::make_pair(flag, it->second));
}

/*************************************************************
* registerMonitorDouble                                      *
* add to the map of monitors (type of generator is double)   *
**************************************************************/
void Component::registerMonitorDouble(std::string dataName) {
	int dataID;

	dataID = getDataID(dataName);
	std::pair<Monitors::iterator, bool> res = monitorDOUBLE.insert(std::make_pair(dataName, dataID));
	if ( ! res.second ) {
            // key already exists for the component
	    std::cout<< "Component::registerMonitorDouble-- generator name " << (res.first)->first << " already exists." << std::endl;
	    exit(1);
	}

}

/*************************************************************
* ifMonitorDoubleData	                                     *
* arbitrary statistics (double) monitoring		     *
**************************************************************/
std::pair<bool, int> Component::ifMonitorDoubleData(std::string dataName) {

	std::map<std::string, int>::iterator it;
	bool flag = false;
	
	it = monitorDOUBLE.find(dataName);
        if (it != monitorDOUBLE.end() )   // found key
            flag = true;
	else
	    flag = false;

	return(std::make_pair(flag, it->second));
}

/*************************************************************
* addToIntroList	                                     *
* Add the id of the introspector to MyIntroList		     *
**************************************************************/
void Component::addToIntroList(ComponentId_t id){
	
	MyIntroList.push_back(id);
}


/*************************************************************
* isTimeToPush	                                     	     *
* return if it's time for clever component to push data	     *
**************************************************************/
bool Component::isTimeToPush(Cycle_t current, const char *type){
	 Introspector *c;
	 SimTime_t pushFreq;
	 
	    
	 //get the "push" introspector
         c = Simulation::getSimulation()->getIntrospector(type);

	 //how many component cycles this should push data 
         pushFreq = c->getFreq() / this->getFreq();

	 //check if "current" is the time to push
	 if(current % pushFreq == 0)
	    return true;
	 else
	    return false;

}

/*********************************************************************
* getDataID	                                     	             *
* return the enum value of the data (statistics) that the component  *
* wants to be monitored						     *
**********************************************************************/
int Component::getDataID(std::string dataName){
	
	std::string stats_names_array[] = {/*McPAT counters*/
	    "core_temperature","branch_read", "branch_write", "RAS_read", "RAS_write",
	    "il1_read", "il1_readmiss", "IB_read", "IB_write", "BTB_read", "BTB_write",
	    "int_win_read", "int_win_write", "fp_win_read", "fp_win_write", "ROB_read", "ROB_write",
	    "iFRAT_read", "iFRAT_write", "iFRAT_search", "fFRAT_read", "fFRAT_write", "fFRAT_search", "iRRAT_write", "fRRAT_write",
	    "ifreeL_read", "ifreeL_write", "ffreeL_read", "ffreeL_write", "idcl_read", "fdcl_read",
	    "dl1_read", "dl1_readmiss", "dl1_write", "dl1_writemiss", "LSQ_read", "LSQ_write",
	    "itlb_read", "itlb_readmiss", "dtlb_read", "dtlb_readmiss",
	    "int_regfile_reads", "int_regfile_writes", "float_regfile_reads", "float_regfile_writes", "RFWIN_read", "RFWIN_write",
	    "bypass_access", "router_access",
	    "L2_read", "L2_readmiss", "L2_write", "L2_writemiss", "L3_read", "L3_readmiss", "L3_write", "L3_writemiss",
	    "L1Dir_read", "L1Dir_readmiss", "L1Dir_write", "L1Dir_writemiss", "L2Dir_read", "L2Dir_readmiss", "L2Dir_write", 		 		    "L2Dir_writemiss",
	    "memctrl_read", "memctrl_write",
	    /*sim-panalyzer counters*/
	    "alu_access", "fpu_access", "mult_access", "io_access",
	    /*zesto counters */
	    "cache_load_lookups", "cache_load_misses", "cache_store_lookups", "cache_store_misses", "writeback_lookups", "writeback_misses", 		    "prefetch_lookups", "prefetch_misses", 
	    "prefetch_insertions", "prefetch_useful_insertions", "MSHR_occupancy",
	    "MSHR_full_cycles", "WBB_insertions", "WBB_victim_insertions",
    	    "WBB_combines", "WBB_occupancy", "WBB_full_cycles",
    	    "WBB_hits", "WBB_victim_hits", "core_lookups", "core_misses", "MSHR_combos"};

	std::vector<std::string> stats_names(stats_names_array, stats_names_array + sizeof(stats_names_array) / sizeof(stats_names_array[0]) );
	unsigned int pos = std::find(stats_names.begin(), stats_names.end(), dataName) - stats_names.begin();

        if( pos < stats_names.size() )
           return (pos);
        else
           std::cout << "ERROR--Component::getDataID dataName " << dataName << " not found." << std::endl; exit(1);

}


TimeConverter* Component::registerClock( std::string freq, ClockHandler_t* handler, bool regAll) {
    TimeConverter* tc = Simulation::getSimulation()->registerClock(freq,handler);
    
    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
// 	BOOST_FOREACH( Link * lnk, linkVec ) {
// 	    lnk->setDefaultTimeBase(tc);
// 	}
	std::pair<std::string,Link*> p;
// 	BOOST_FOREACH( p, linkMap ) {
	// FIXME
	BOOST_FOREACH( p, myLinks->getLinkMap() ) {
	    p.second->setDefaultTimeBase(tc);
	}
	defaultTimeBase = tc;
    }
    return tc;
}

TimeConverter* Component::registerTimeBase( std::string base, bool regAll) {
    TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(base);
    
    // if regAll is true set tc as the default for the component and
    // for all the links
    if ( regAll ) {
// 	BOOST_FOREACH( Link * lnk, linkMap ) {
// 	    lnk->setDefaultTimeBase(tc);
// 	}
	std::pair<std::string,Link*> p;
// 	BOOST_FOREACH( p, linkMap ) {
	// FIXME
	BOOST_FOREACH( p, myLinks->getLinkMap() ) {
	    p.second->setDefaultTimeBase(tc);
	}
	defaultTimeBase = tc;
    }
    return tc;
}

Link* Component::LinkAdd( std::string name, Event::Handler_t* functor )
{
    // Need to add error checking at some point...
    Link* tmp = myLinks->getLink(name);
    tmp->setFunctor(functor);
    // If no functor, this is a polling link
    if ( functor == NULL ) {
	tmp->setPolling();
    }
    return tmp;
}

Link* Component::selfLink( std::string name, Event::Handler_t* handler )
{
//     Link* link = new Link(handler);
//     link->Connect(link,0);
//     return link;
    Link* link = new SelfLink();
    link->setLatency(0);
    link->setFunctor(handler);
    if ( handler == NULL ) {
	link->setPolling();
    }
    return link;
}
    
    
SimTime_t Component::getCurrentSimTime(TimeConverter *tc) {
    return tc->convertFromCoreTime(Simulation::getSimulation()->getCurrentSimCycle());
}

SimTime_t Component::getCurrentSimTime(std::string base) {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getTimeConverter(base));

}
    
SimTime_t Component::getCurrentSimTimeNano() {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getNano());
}

SimTime_t Component::getCurrentSimTimeMicro() {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getMicro());
}

SimTime_t Component::getCurrentSimTimeMilli() {
    return getCurrentSimTime(Simulation::getSimulation()->getTimeLord()->getMilli());
}

bool Component::registerExit()
{
    return Simulation::getSimulation()->getExit()->refInc( Id() ); 
}

bool Component::unregisterExit()
{
    return Simulation::getSimulation()->getExit()->refDec( Id() ); 
}
    
    
} // namespace SST

