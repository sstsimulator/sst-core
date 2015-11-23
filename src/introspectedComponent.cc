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


#include <sst_config.h>
#include "sst/core/serialization.h"
#include "sst/core/introspectedComponent.h"

#include <boost/foreach.hpp>
#include <string.h>

//#include "sst/core/exit.h"
//#include "sst/core/event.h"
#include "sst/core/introspector.h"
//#include "sst/core/link.h"
#include "sst/core/simulation.h"
//#include "sst/core/timeLord.h"

namespace SST {

PowerDatabase IntrospectedComponent::PDB;

IntrospectedComponent::IntrospectedComponent(ComponentId_t id) :
    Component( id )
{
}

IntrospectedComponent::IntrospectedComponent() 
{
}

/*****************************************************
* regPowerStats                                      *
* Register/update power statistics of this component * 
* in the central power database                      *
******************************************************/
void IntrospectedComponent::regPowerStats(Pdissipation_t pusage) {	
	
	std::pair<PowerDatabase::iterator, bool> res = PDB.insert(std::make_pair( getId(), pusage));
	if ( ! res.second ) {
            // key already exists for the component; update power statistics instead
            //(res.first)->second.internalPower = pusage.internalPower;
            //(res.first)->second.switchingPower = pusage.switchingPower;

	    /*(res.first)->second.TDP = pusage.TDP;
	    (res.first)->second.runtimeDynamicPower = pusage.runtimeDynamicPower;
            (res.first)->second.leakagePower = pusage.leakagePower;
            (res.first)->second.peak = pusage.peak;
            (res.first)->second.currentPower = pusage.currentPower;
            (res.first)->second.totalEnergy = pusage.totalEnergy;
            (res.first)->second.currentSimTime = pusage.currentSimTime;*/
	    (res.first)->second = pusage;
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
std::pair<bool, Pdissipation_t> IntrospectedComponent::readPowerStats(Component* c) {

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

	it = PDB.find(c->getId());
        if (it != PDB.end() )   // found key
            return std::make_pair(true, it->second);
	
	else {
	    //std::cout << "IntrospectedComponent::readPowerStats--ID: " << c->Id() << "can't find key" << std::endl;
	    return std::make_pair(false, pusage);
	}

}

/********************************************************
* registerIntrospector                                  *
* Add the pointer to the introspector to MyIntroList    *
*********************************************************/
void IntrospectedComponent::registerIntrospector(std::string name)
{
	Introspector *c;
	    
	//get the introspector
        c = Simulation::getSimulation()->getIntrospector(name.c_str());
	//put the introspector to the list
	MyIntroList.push_back(c);

}

/********************************************************
* readPowerStas                                         *
* read power statistics of this component from database *
*********************************************************/
//void IntrospectedComponent::registerMonitor(std::string dataName, typeT* dataptr);

/************************************************************
* registerMonitor                                           *
* add to the map of monitors (type of monitor is a handler) *
*********************************************************/

void IntrospectedComponent::registerMonitor(std::string dataName, IntrospectedComponent::MonitorBase* handler)
{
	std::pair<MonitorMap_t::iterator, bool> res = monitorMap.insert(std::make_pair(dataName, handler ));
	if ( ! res.second ) {
            // key already exists for the component
	    std::cout<< "IntrospectedComponent::registerMonitor-- monitor name " << (res.first)->first << " already exists." << std::endl;
	    exit(1);
	}

} 

/************************************************************
* getMonitor                                                *
* find monitor from the map; this is called in Introspector::getData() *
*********************************************************/

std::pair<bool, IntrospectedComponent::MonitorBase*> IntrospectedComponent::getMonitor(std::string dataname)
{
     MonitorMap_t::iterator i = monitorMap.find(dataname);
     if (i != monitorMap.end()) {
            ////return (boost::unsafe_any_cast<IntrospectedComponent::MonitorBase<returnT>*>(i->second));
	    return (std::make_pair(true, i->second));
     } 
     else{
	    return (std::make_pair(false, i->second));
	//std::cout<< "IntrospectedComponent::getMonitor-- data name " << dataname << " does not associate with a monitor." << std::endl;
	//exit(1);
     }
}

/********************************************************
* triggerUpdate                                         *
* used for the push mechanism				*
*********************************************************/
void IntrospectedComponent::triggerUpdate()
{
	Introspector *c;

	for (std::list<Introspector*>::iterator i = MyIntroList.begin();
	        i != MyIntroList.end(); ++i) {
     		    // trigger each introspector to update 
     		    c = *i;
		    c->triggeredUpdate();
	}
}



/*************************************************************
* isTimeToPush	                                     	     *
* return if it's time for clever component to push data	     *
**************************************************************/
bool IntrospectedComponent::isTimeToPush(Cycle_t current, const char *name){
	 Introspector *c;
	 SimTime_t pushFreq;
	 
	    
	 //get the "push" introspector
         c = Simulation::getSimulation()->getIntrospector(name);

	 //how many component cycles this should push data 
         pushFreq = c->getFreq() / this->getFreq();

	 //check if "current" is the time to push
	 if(current % pushFreq == 0)
	    return true;
	 else
	    return false;

}



template<class Archive>
void
IntrospectedComponent::serialize(Archive& ar, const unsigned int version) {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(MyIntroList);
    ar & BOOST_SERIALIZATION_NVP(monitorMap);
}
    
} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::IntrospectedComponent::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::IntrospectedComponent)
