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


#include "sst_config.h"
#include "sst/core/serialization/core.h"

#include <utility>

#include <boost/foreach.hpp>

#include <sst/core/simulation.h>
#include <sst/core/factory.h>
#include <sst/core/stopAction.h>
#include <sst/core/exit.h>
#include <sst/core/event.h>
#include <sst/core/config.h>
#include <sst/core/configGraph.h>
#include <sst/core/graph.h>
#include <sst/core/timeLord.h>
#include <sst/core/linkMap.h>
#include <sst/core/linkPair.h>
#include <sst/core/sync.h>
#include <sst/core/syncQueue.h>
#include <sst/core/clock.h>
#include <sst/core/timeVortex.h>
#include <sst/core/archive.h>

#define SST_SIMTIME_MAX  0xffffffffffffffff

namespace SST { 

SimulationBase::SimulationBase(Config *config)
{
    factory = new Factory(config->libpath);
    timeLord = new TimeLord(config->timeBase);
}

SimulationBase::~SimulationBase()
{
    //    delete factory;
    delete timeLord;
    
}

TimeConverter* SimulationBase::minPartToTC(SimTime_t cycles) const {
    return timeLord->getTimeConverter(cycles);
}

template<class Archive>
void
SimulationBase::serialize(Archive & ar, const unsigned int version)
{
    printf("begin SimulationBase::serialize\n");
    printf("  - SimulationBase::factory\n");
    ar & BOOST_SERIALIZATION_NVP(factory);
    printf("  - SimulationBase::timeLord\n");
    ar & BOOST_SERIALIZATION_NVP(timeLord);
    printf("end SimulationBase::serialize\n");
}


Simulation* Simulation::instance = NULL;

Simulation::~Simulation()
{
    // Clean up as best we can

    // Delete the timeVortex first.  This will delete all events left
    // in the queue, as well as the Sync, Exit and Clock objects.
    delete timeVortex;

    // Delete all the components
    for ( CompMap_t::iterator it = compMap.begin(); it != compMap.end(); ++it ) {
	delete it->second;
    }
    compMap.clear();
    
    // Delete all the introspectors
    for ( IntroMap_t::iterator it = introMap.begin(); it != introMap.end(); ++it ) {
	delete it->second;
    }
    introMap.clear();

    // Clocks already got deleted by timeVortex, simply clear the clockMap
    clockMap.clear();
    
    
    // Delete any remaining links.  This should never happen now, but
    // when we add an API to have components build subcomponents, user
    // error could cause LinkMaps to be left.
    std::map<ComponentId_t,LinkMap*>::iterator it;
    for ( it = component_links.begin(); it != component_links.end(); ++it ) {
	delete it->second;
    }
    component_links.clear();
}

Simulation*
Simulation::createSimulation(Config *config, int my_rank, int num_ranks)
{
    instance = new Simulation(config,my_rank,num_ranks);
    return instance;
}


Simulation::Simulation( Config* cfg, int my_rank, int num_ranks ) :
    SimulationBase(cfg), timeVortex(NULL), minPartTC( NULL ), sync(NULL), currentSimCycle(0), endSim(false), my_rank(my_rank), num_ranks(num_ranks)
{
//     eQueue = new EventQueue_t;
    timeVortex = new TimeVortex;

    SimTime_t stopAt = timeLord->getSimCycles(cfg->stopAtCycle,"StopAction configure");
    if ( stopAt != 0 ) {
	printf("Inserting stop event at cycle %s\n",
	       cfg->stopAtCycle.c_str());
	StopAction* sa = new StopAction();
	sa->setDeliveryTime(stopAt);
	timeVortex->insert(sa);
    }
    
    m_exit = new Exit( this, timeLord->getTimeConverter("100ns"), num_ranks == 1 );
}

Simulation::Simulation()
{
    // serialization only, so everything will be restored.  I think.
}

Component*
Simulation::createComponent( ComponentId_t id, std::string name, 
                             Params params )
{
    return factory->CreateComponent(id, name, params);
}

Introspector*
Simulation::createIntrospector(std::string name, Params params )
{
    return factory->CreateIntrospector(name, params);
}

void
Simulation::requireEvent(std::string name)
{
    factory->RequireEvent(name);
}

int Simulation::performWireUp( ConfigGraph& graph, int myRank )
{
    if ( num_ranks > 1 ) {
	// Find the minimum latency across a partition
	SimTime_t min_part = 0xffffffffl;
	for( ConfigLinkMap_t::iterator iter = graph.links.begin();
	     iter != graph.links.end(); ++iter )
	{
	    ConfigLink* clink = (*iter).second;
	    int rank[2];
	    rank[0] = graph.comps[clink->component[0]]->rank;
	    rank[1] = graph.comps[clink->component[1]]->rank;
	    if ( rank[0] == rank[1] ) continue;
	    if ( clink->getMinLatency() < min_part ) {
		min_part = clink->getMinLatency();
	    }	
	}
	
	sync = new Sync( minPartTC = minPartToTC(min_part) );
    }
    
    // We will go through all the links and create LinkPairs for each
    // link.  We will also create a LinkMap for each component and put
    // them into a map with ComponentID as the key.
    for( ConfigLinkMap_t::iterator iter = graph.links.begin();
                            iter != graph.links.end(); ++iter )
    {
        ConfigLink* clink = (*iter).second;
        int rank[2];
        rank[0] = graph.comps[clink->component[0]]->rank;
        rank[1] = graph.comps[clink->component[1]]->rank;

	if ( rank[0] != myRank && rank[1] != myRank ) {
	    // Nothing to be done
	    delete clink;
            continue;
	}	
        else if ( rank[0] == rank[1] ) { 
	    // Create a LinkPair to represent this link
	    LinkPair lp(clink->id);

	    lp.getLeft()->setLatency(clink->latency[0]);
	    lp.getRight()->setLatency(clink->latency[1]);

	    // Add this link to the appropriate LinkMap
	    std::map<ComponentId_t,LinkMap*>::iterator it;
	    // it = component_links.find(clink->component[0]->id);
	    it = component_links.find(clink->component[0]);
	    if ( it == component_links.end() ) {
		LinkMap* lm = new LinkMap();
		std::pair<std::map<ComponentId_t,LinkMap*>::iterator,bool> ret_val;
		// ret_val = component_links.insert(std::pair<ComponentId_t,LinkMap*>(clink->component[0]->id,lm));
		ret_val = component_links.insert(std::pair<ComponentId_t,LinkMap*>(clink->component[0],lm));
 		it = ret_val.first;
	    }
 	    it->second->insertLink(clink->port[0],lp.getLeft());

	    // it = component_links.find(clink->component[1]->id);
	    it = component_links.find(clink->component[1]);
	    if ( it == component_links.end() ) {
		LinkMap* lm = new LinkMap();
		std::pair<std::map<ComponentId_t,LinkMap*>::iterator,bool> ret_val;
		// ret_val = component_links.insert(std::pair<ComponentId_t,LinkMap*>(clink->component[1]->id,lm));
		ret_val = component_links.insert(std::pair<ComponentId_t,LinkMap*>(clink->component[1],lm));
 		it = ret_val.first;
	    }
 	    it->second->insertLink(clink->port[1],lp.getRight());

	}
	else {
	    int local, remote;
	    if ( rank[0] == myRank ) {
		local = 0;
		remote = 1;
	    }
	    else {
		local = 1;
		remote = 0;
	    }

	    // Create a LinkPair to represent this link
	    LinkPair lp(clink->id);
	    
	    lp.getLeft()->setLatency(clink->latency[local]);
	    lp.getRight()->setLatency(0);
	    lp.getRight()->setDefaultTimeBase(minPartToTC(1));
            

	    // Add this link to the appropriate LinkMap for the local component
	    std::map<ComponentId_t,LinkMap*>::iterator it;
	    it = component_links.find(clink->component[local]);
	    if ( it == component_links.end() ) {
		LinkMap* lm = new LinkMap();
		std::pair<std::map<ComponentId_t,LinkMap*>::iterator,bool> ret_val;
		ret_val = component_links.insert(std::pair<ComponentId_t,LinkMap*>(clink->component[local],lm));
 		it = ret_val.first;
	    }
 	    it->second->insertLink(clink->port[local],lp.getLeft());

	    // For the remote side, register with sync object
	    SyncQueue* sync_q = sync->registerLink(rank[remote],clink->id,lp.getRight());
	    lp.getRight()->recvQueue = sync_q;
        }

	// Done with that edge, delete it.
	delete clink;
    }

    // Now, build all the components
    for ( ConfigComponentMap_t::iterator iter = graph.comps.begin();
                            iter != graph.comps.end(); ++iter )
    {
	ConfigComponent* ccomp = (*iter).second;

        if (ccomp->isIntrospector) {
	    Introspector* tmp;

            // _SIM_DBG("creating introspector: name=\"%s\" type=\"%s\" id=%d\n",
	    // 	     name.c_str(), sdl_c->type().c_str(), (int)id );
            
            tmp = createIntrospector( ccomp->type.c_str(),ccomp->params );
            introMap[ccomp->name] = tmp;
        }
	else if ( ccomp->rank == myRank ) {
            Component* tmp;
//             _SIM_DBG("creating component: name=\"%s\" type=\"%s\" id=%d\n",
// 		     name.c_str(), sdl_c->type().c_str(), (int)id );

            // Check to make sure there is a LinkMap for this component
            std::map<ComponentId_t,LinkMap*>::iterator it;
            it = component_links.find(ccomp->id);
            if ( it == component_links.end() ) {
                printf("WARNING: Building component \"%s\" with no links assigned.\n",ccomp->name.c_str());
                LinkMap* lm = new LinkMap();
                component_links[ccomp->id] = lm;
            }

			compIdMap[ccomp->id] = ccomp->name;
            tmp = createComponent( ccomp->id, ccomp->type.c_str(),
                                   ccomp->params );
            compMap[ccomp->name] = tmp;

        }
	// Done with vertex, delete it;
	delete ccomp;
    } // end for all vertex    
    return 0;
}

void Simulation::Run() {
    _SIM_DBG( "RUN\n" );

    // Need to exchange LinkInitData objects if we are multirank
    if ( num_ranks > 1 ) {
	sync->exchangeLinkInitData();
    }
    
    for( CompMap_t::iterator iter = compMap.begin();
                            iter != compMap.end(); ++iter )
    {
        if ( (*iter).second->Setup() ) {
            _ABORT(Simulation,"Setup()\n");
        }
    }
    //for introspector
    for( IntroMap_t::iterator iter = introMap.begin();
                            iter != introMap.end(); ++iter )
    {
        if ( (*iter).second->Setup() ) {
            _ABORT(Simulation,"Setup()\n");
        }
    }

    // // Check to make sure we have something to do.  If not print error
    // // message and exit
    // if ( timeVortex->front() == NULL && num_ranks == 1 ) {
    //	std::cout << "No clocks registered and no events sent during initialization.  Exiting simulation..." << std::endl;
    //	exit(1);
    // }
    
    // Fix taken from Scott's change to mainline (SDH, Sep 27 12)
    // Put a stop event at the end of the timeVortex. Simulation will
    // only get to this is there are no other events in the queue.
    // In general, this shouldn't happen, especially for parallel
    // simulations. If it happens in a parallel simulation the 
    // simulation will likely deadlock as only some of the ranks
    // will hit the anomoly.
    StopAction* sa = new StopAction("*** Event queue empty, exiting simulation... ***");
    sa->setDeliveryTime(SST_SIMTIME_MAX);
    timeVortex->insert(sa);

    while( LIKELY( ! endSim ) ) {
 	currentSimCycle = timeVortex->front()->getDeliveryTime();

//  	Activity *ptr = timeVortex->pop();
//   	ptr->execute();
 	current_activity = timeVortex->pop();
  	current_activity->execute();
    }


    for( CompMap_t::iterator iter = compMap.begin();
                            iter != compMap.end(); ++iter )
    {
        if ( (*iter).second->Finish() ) {
            _ABORT(Simulation,"Finish()\n");
        }
    }
    //for introspector
    for( IntroMap_t::iterator iter = introMap.begin();
                            iter != introMap.end(); ++iter )
    {
        if ( (*iter).second->Finish() ) {
            _ABORT(Simulation,"Finish()\n");
        }
    }

}

SimTime_t
Simulation::getCurrentSimCycle() const
{
    return currentSimCycle; 
}

void
Simulation::printStatus(bool print_timevortex)
{
    std::cout << "Simulation: instance: " << (long) Simulation::instance << std::endl;
    std::cout << "  Current cycle: " << Simulation::instance->currentSimCycle << std::endl;
    std::cout << "  Current activity: ";
    Simulation::instance->current_activity->print("");
    
    if ( print_timevortex) Simulation::instance->timeVortex->print();
    
    
//     if (Simulation::instance != NULL) {
//         for (CompMap_t::iterator iter = instance->compMap.begin() ;
//              iter != instance->compMap.end() ; ++iter ) {
//             if (iter->second->Status()) quit = true;
//         }
//     }

//     if (quit)  _ABORT(Simulation, "Status()\n");
}

TimeConverter* Simulation::registerClock( std::string freq, Clock::HandlerBase* handler )
{
//     _SIM_DBG("freq=%f handler=%p\n", frequency, handler );
    
    TimeConverter* tcFreq = timeLord->getTimeConverter(freq);

    if ( clockMap.find( tcFreq->getFactor() ) == clockMap.end() ) {
	_SIM_DBG( "\n" );
	Clock* ce = new Clock( tcFreq );
	clockMap[ tcFreq->getFactor() ] = ce; 

	ce->setDeliveryTime( currentSimCycle + tcFreq->getFactor() );
	timeVortex->insert( ce );
    }
    clockMap[ tcFreq->getFactor() ]->registerHandler( handler );
    return tcFreq;
    
}

void Simulation::unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler) {
//     _SIM_DBG("freq=%f handler=%p\n", frequency, handler );
    
    if ( clockMap.find( tc->getFactor() ) != clockMap.end() ) {
	_SIM_DBG( "\n" );
	bool empty;
	clockMap[ tc->getFactor() ]->unregisterHandler( handler, empty );
	
    }
}

void Simulation::insertActivity(SimTime_t time, Activity* ev) {
    ev->setDeliveryTime(time);
    timeVortex->insert(ev);

}


template<class Archive>
void
Simulation::serialize(Archive & ar, const unsigned int version)
{
    printf("begin Simulation::serialize\n");
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SimulationBase);

    printf("  - Simulation::timeVortex\n");
    ar & BOOST_SERIALIZATION_NVP(timeVortex);

    printf("  - Simulation::sync (%p)\n", sync);
    ar & BOOST_SERIALIZATION_NVP(sync);

    printf("  - Simulation::compMap\n");
    ar & BOOST_SERIALIZATION_NVP(compMap);

    printf("  - Simulation::introMap\n");
    ar & BOOST_SERIALIZATION_NVP(introMap);
    printf("  - Simulation::clockMap\n");
    ar & BOOST_SERIALIZATION_NVP(clockMap);
    printf("  - Simulation::currentSimCycle\n");
    ar & BOOST_SERIALIZATION_NVP(currentSimCycle);
    printf("  - Simulation::m_exit\n");
    ar & BOOST_SERIALIZATION_NVP(m_exit);
    printf("  - Simulation::endSim\n");
    ar & BOOST_SERIALIZATION_NVP(endSim);
    printf("  - Simulation::my_rank\n");
    ar & BOOST_SERIALIZATION_NVP(my_rank);
    printf("  - Simulation::num_ranks\n");
    ar & BOOST_SERIALIZATION_NVP(num_ranks);

    printf("  - Simulation::component_links\n");
    ar & BOOST_SERIALIZATION_NVP(component_links);

    printf("end Simulation::serialize\n");
}

} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::SimulationBase::serialize)
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Simulation::serialize)

BOOST_CLASS_EXPORT_IMPLEMENT(SST::SimulationBase)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Simulation)
