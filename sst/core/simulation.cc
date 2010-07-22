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
#include <sst/core/stopEvent.h>
#include <sst/core/exit.h>
#include <sst/core/event.h>
#include <sst/core/config.h>
#include <sst/core/graph.h>
#include <sst/core/timeLord.h>
#include <sst/core/linkPair.h>
#include <sst/core/sync.h>
#include <sst/core/syncQueue.h>
#include <sst/core/clockEvent.h>
#include <sst/core/timeVortex.h>
#include <sst/core/archive.h>


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

TimeConverter* SimulationBase::minPartToTC(SimTime_t cycles) {
    return timeLord->getTimeConverter(cycles);
}

template<class Archive>
void
SimulationBase::serialize(Archive & ar, const unsigned int version)
{
    printf("begin SimulationBase::serialize\n");
    ar & BOOST_SERIALIZATION_NVP(factory);
    ar & BOOST_SERIALIZATION_NVP(timeLord);
    printf("end SimulationBase::serialize\n");
}


Simulation* Simulation::instance = NULL;

Simulation*
Simulation::createSimulation(Config *config)
{
    instance = new Simulation(config);
    return instance;
}


Simulation::Simulation( Config* cfg ) :
    SimulationBase(cfg), currentSimCycle(0), endSim(false)
{
//     eQueue = new EventQueue_t;
    timeVortex = new TimeVortex;
    compMap = new CompMap_t;
    introMap = new IntroMap_t;
    printf("Inserting stop event at cycle %ld\n",
           (long int)cfg->stopAtCycle);

    // KSH FIXME:  need to add this back one once things stabilize
//     if ( cfg->stopAtCycle ) {
//         StopEvent* se = new StopEvent();
//         std::pair<EventHandlerBase<bool,Event*>*,Event*> envelope;
//         envelope.first = se->getFunctor();
//         envelope.second = se;
    
//         eQueue->insert( cfg->stopAtCycle, envelope ); 
//     }

    m_exit = new Exit( this, timeLord->getTimeConverter("10ns") );
}

Simulation::Simulation()
{
    // serialization only, so everything will be restored.  I think.
}

SimTime_t Simulation::getCurrentSimCycle() {
    return currentSimCycle;
}

Component*
Simulation::createComponent( ComponentId_t id, std::string name, 
                            Component::Params_t params )
{
    return factory->CreateComponent( id, name, params );
}


int Simulation::WireUp( Graph& graph, SDL_CompMap_t& sdlMap,
            int minPart, int myRank )
{
    _SIM_DBG("minPart=%d myRank=%d\n", minPart, myRank );

    for( VertexList_t::iterator iter = graph.vlist.begin();
                        iter != graph.vlist.end(); ++iter )
    {
        Vertex *v        = (*iter).second;
        int     vertRank = atoi( v->prop_list.get(GRAPH_RANK).c_str() );

        if ( vertRank == myRank)
        {
            std::string    name  = v->prop_list.get(GRAPH_COMP_NAME).c_str();
            ComponentId_t  id  = atoi(v->prop_list.get(GRAPH_ID).c_str() );
            SDL_Component* sdl_c = sdlMap[ name.c_str() ];
            Component*     tmp;
	    Introspector* itmp;

	    //create introspector on vertRank
	    if(sdl_c->isIntrospector() == true)
	    {
		_SIM_DBG("creating introspector: name=\"%s\" type=\"%s\" id=%d\n",
		     name.c_str(), sdl_c->type().c_str(), (int)id );

		tmp = createComponent( id, sdl_c->type().c_str(),
                                                        sdl_c->params );
		itmp = dynamic_cast<Introspector *> (tmp); 
		assert(itmp);

                (*introMap)[ itmp->getId() ] = itmp;
		if (itmp->getId() != id) {
	           _ABORT(Simulation, 
		     "introspector id does not match assigned id (%d)\n", (int)id);
	        }
	    }
	    //create component
	    else
	    {
                _SIM_DBG("creating component: name=\"%s\" type=\"%s\" id=%d\n",
		     name.c_str(), sdl_c->type().c_str(), (int)id );

		// Check to make sure there is a LinkMap for this component
		std::map<ComponentId_t,LinkMap*>::iterator it;
		it = component_links.find(id);
		if ( it == component_links.end() ) {
		    printf("WARNING: Building component \"%s\" with no links assigned.\n",name.c_str());
		    LinkMap* lm = new LinkMap();
		    component_links[id] = lm;
		}
		
                tmp = createComponent( id, sdl_c->type().c_str(),
                                                        sdl_c->params );
                (*compMap)[ tmp->getId() ] = tmp;
		if (tmp->getId() != id) {
	            _ABORT(Simulation, 
		     "component id does not match assigned id (%d)\n", (int)id);
	        }
	    } 
        }
	//create same type of introspector on other ranks
	else
	{   
	    std::string    name  = v->prop_list.get(GRAPH_COMP_NAME).c_str();
	    SDL_Component* sdl_c = sdlMap[ name.c_str() ];
	    Component*     tmp;
	    ComponentId_t  id  = atoi(v->prop_list.get(GRAPH_ID).c_str() );

	    if(sdl_c->isIntrospector() == true)
	    {
	        Introspector* itmp;

		_SIM_DBG("creating introspector in parallel: name=\"%s\" type=\"%s\" id=%d\n",
		     name.c_str(), sdl_c->type().c_str(), (int)id );

		tmp = createComponent( id, sdl_c->type().c_str(),
                                                        sdl_c->params );
		itmp = dynamic_cast<Introspector *> (tmp); //safe downcast
		assert(itmp);

                (*introMap)[ itmp->getId() ] = itmp;
		if (itmp->getId() != id) {
	           _ABORT(Simulation, 
		     "introspector id does not match assigned id (%d)\n", (int)id);
	        }
	    }
	}
    } // end for all vertex

    // this is a mess
    // we can have a sync object for each set of links that have the same
    // frequency but currently the partition code does not support this
    // can it support it? 
    // if minPart is 99999 we are running within 1 rank
    // we need to use something other than 99999 to indicate this
    if ( minPart < 99999 ) {
//         syncMap[0] = new Sync( timeLord->getTimeConverter(minPart) );
    }
    /*
    for( EdgeList_t::iterator iter = graph.elist.begin();
                            iter != graph.elist.end(); ++iter )
    {
        Edge *e = (*iter).second;
        int rank[2];
        rank[0] = atoi(graph.vlist[e->v(0)]->prop_list.get(GRAPH_RANK).c_str());
        rank[1] = atoi(graph.vlist[e->v(1)]->prop_list.get(GRAPH_RANK).c_str());

        if ( rank[0] != myRank && rank[1] != myRank ) { 
            continue;
        }
// 	printf("Found a link on my rank\n");
        std::string compName[2];
        std::string linkName[2];
        ComponentId_t cId[2];
	    uint64_t latency[2];
	
        std::string edge_name = e->prop_list.get(GRAPH_LINK_NAME);
//         printf("Edge: %s %d %d\n", edge_name.c_str(), e->v(0), e->v(1) );

        SDL_Link*       sdlLink;
        SDL_Component*  sdlComp;
        Vertex*         vertex;

        vertex      = graph.vlist[e->v(0)];
        sdlComp     = sdlMap[ vertex->prop_list.get(GRAPH_COMP_NAME).c_str() ];
        sdlLink     = sdlComp->links[edge_name];
        compName[0] = vertex->prop_list.get(GRAPH_COMP_NAME); 
        cId[0]      = atoi(vertex->prop_list.get(GRAPH_ID).c_str()); 
        linkName[0] = sdlLink->params["name"];
	latency[0]  = timeLord->getSimCycles(sdlLink->params["lat"], edge_name);

        vertex      = graph.vlist[e->v(1)];
        sdlComp     = sdlMap[ vertex->prop_list.get(GRAPH_COMP_NAME).c_str() ];
        sdlLink     = sdlComp->links[edge_name];
        compName[1] = vertex->prop_list.get(GRAPH_COMP_NAME); 
        cId[1]      = atoi(vertex->prop_list.get(GRAPH_ID).c_str()); 
        linkName[1] = sdlLink->params["name"];
	latency[1]  = timeLord->getSimCycles(sdlLink->params["lat"], edge_name);

        _SIM_DBG("Connecting component \"%s\" with link \"%s\" to component \"%s\" on link \"%s\"\n",
			    compName[0].c_str(),
                            linkName[0].c_str(),
                            compName[1].c_str(),
                            linkName[1].c_str());

        if ( rank[0] == rank[1] ) { 

	        _SIM_DBG("Both components are on the same rank\n");
	        if ( Connect( (*compMap)[ cId[0] ], linkName[0], latency[0], 
		                    (*compMap)[ cId[1] ], linkName[1], latency[1] ) )
            {
                _abort(Simulation,"Connect failed %s <-> %s\n",
                                  linkName[0].c_str(), linkName[1].c_str() );
            }
        } else {
            if ( myRank == rank[0] ) {
		        _SIM_DBG("This component is on one rank\n");
                syncMap[0]->registerLink( edge_name, 
                           (*compMap)[ cId[0] ]->LinkGet( linkName[0] ),
                            rank[1], latency[0] );
            } else {
		        _SIM_DBG("This component is on the other rank\n");
                syncMap[0]->registerLink( edge_name, 
                           (*compMap)[ cId[1] ]->LinkGet( linkName[1] ),
                           rank[0], latency[1] );
            }
        }
    }
    */
//     if ( ! syncMap.empty() ) {
//         syncMap[0]->exchangeFunctors();
//     }

    _SIM_DBG( "config done\n\n" );
    return 0;
}

int Simulation::performWireUp( Graph& graph, SDL_CompMap_t& sdlMap,
            int minPart, int myRank )
{
    // For now only works with a single rank (though some of the
    // multi-rank code is there)

    // We will go through all the links and create LinkPairs for each
    // link.  We will also create a LinkMap for each component and put
    // them into a map with ComponentID as the key.

//     if ( minPart < 99999 ) {
	sync = new Sync( minPartToTC(minPart) );
//     }

    
    for( EdgeList_t::iterator iter = graph.elist.begin();
                            iter != graph.elist.end(); ++iter )
    {
        Edge *e = (*iter).second;
        int rank[2];
        rank[0] = atoi(graph.vlist[e->v(0)]->prop_list.get(GRAPH_RANK).c_str());
        rank[1] = atoi(graph.vlist[e->v(1)]->prop_list.get(GRAPH_RANK).c_str());

        if ( rank[0] != myRank && rank[1] != myRank ) { 
            continue;
        }
	
	int id = e->id();
//         std::string compName[2];
        std::string linkName[2];
        ComponentId_t cId[2];
	uint64_t latency[2];
	
        std::string edge_name = e->prop_list.get(GRAPH_LINK_NAME);

        SDL_Link*       sdlLink;
        SDL_Component*  sdlComp;
        Vertex*         vertex;


        vertex      = graph.vlist[e->v(0)];
        sdlComp     = sdlMap[ vertex->prop_list.get(GRAPH_COMP_NAME).c_str() ];
        sdlLink     = sdlComp->links[edge_name];
        cId[0]      = atoi(vertex->prop_list.get(GRAPH_ID).c_str()); 
	linkName[0] = sdlLink->params["name"];
 	latency[0]  = timeLord->getSimCycles(sdlLink->params["lat"], edge_name);

        vertex      = graph.vlist[e->v(1)];
        sdlComp     = sdlMap[ vertex->prop_list.get(GRAPH_COMP_NAME).c_str() ];
        sdlLink     = sdlComp->links[edge_name];
        cId[1]      = atoi(vertex->prop_list.get(GRAPH_ID).c_str()); 
	linkName[1] = sdlLink->params["name"];
	latency[1]  = timeLord->getSimCycles(sdlLink->params["lat"], edge_name);


	if ( rank[0] != myRank && rank[1] != myRank ) {
	    // Nothing to be done
	}	
        else if ( rank[0] == rank[1] ) { 
	    // Create a LinkPair to represent this link
	    LinkPair* lp = new LinkPair(id);

	    lp->getLeft()->setLatency(latency[0]);
	    lp->getRight()->setLatency(latency[1]);

	    // Add this link to the appropriate LinkMap
	    std::map<ComponentId_t,LinkMap*>::iterator it;
	    it = component_links.find(cId[0]);
	    if ( it == component_links.end() ) {
		LinkMap* lm = new LinkMap();
		std::pair<std::map<ComponentId_t,LinkMap*>::iterator,bool> ret_val;
		ret_val = component_links.insert(std::pair<ComponentId_t,LinkMap*>(cId[0],lm));
 		it = ret_val.first;
	    }
 	    it->second->insertLink(linkName[0],lp->getLeft());

	    it = component_links.find(cId[1]);
	    if ( it == component_links.end() ) {
		LinkMap* lm = new LinkMap();
		std::pair<std::map<ComponentId_t,LinkMap*>::iterator,bool> ret_val;
		ret_val = component_links.insert(std::pair<ComponentId_t,LinkMap*>(cId[1],lm));
 		it = ret_val.first;
	    }
 	    it->second->insertLink(linkName[1],lp->getRight());

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
	    LinkPair* lp = new LinkPair(id);
	    
	    lp->getLeft()->setLatency(latency[local]);
	    lp->getRight()->setLatency(0);
	    lp->getRight()->setDefaultTimeBase(minPartToTC(1));
            

	    // Add this link to the appropriate LinkMap for the local component
	    std::map<ComponentId_t,LinkMap*>::iterator it;
	    it = component_links.find(cId[local]);
	    if ( it == component_links.end() ) {
		LinkMap* lm = new LinkMap();
		std::pair<std::map<ComponentId_t,LinkMap*>::iterator,bool> ret_val;
		ret_val = component_links.insert(std::pair<ComponentId_t,LinkMap*>(cId[local],lm));
 		it = ret_val.first;
	    }
 	    it->second->insertLink(linkName[local],lp->getLeft());

	    // For the remote side, register with sync object
	    SyncQueue* sync_q = sync->registerLink(rank[remote],id,lp->getRight());
	    lp->getRight()->recvQueue = sync_q;
        }
    }

    // Now, build all the components
    
    return 0;
}

void Simulation::Run() {
    _SIM_DBG( "RUN\n" );

    for( CompMap_t::iterator iter = compMap->begin();
                            iter != compMap->end(); ++iter )
    {
        if ( (*iter).second->Setup() ) {
            _ABORT(Simulation,"Setup()\n");
        }
    }
    //for introspector
    for( IntroMap_t::iterator iter = introMap->begin();
                            iter != introMap->end(); ++iter )
    {
        if ( (*iter).second->Setup() ) {
            _ABORT(Simulation,"Setup()\n");
        }
    }

    printf("Starting main event loop\n");
    while( LIKELY( ! endSim ) ) {
 	currentSimCycle = timeVortex->front()->getDeliveryTime();

 	Activity *ptr = timeVortex->pop();
  	ptr->execute();
    }

    for( CompMap_t::iterator iter = compMap->begin();
                            iter != compMap->end(); ++iter )
    {
        if ( (*iter).second->Finish() ) {
            _ABORT(Simulation,"Finish()\n");
        }
    }
    //for introspector
    for( IntroMap_t::iterator iter = introMap->begin();
                            iter != introMap->end(); ++iter )
    {
        if ( (*iter).second->Finish() ) {
            _ABORT(Simulation,"Finish()\n");
        }
    }

}

void
Simulation::printStatus(void)
{
    bool quit = false;

    printf("Simulation: instance: 0x%lx\n", (long) Simulation::instance);

    if (Simulation::instance != NULL) {
        for (CompMap_t::iterator iter = instance->compMap->begin() ;
             iter != instance->compMap->end() ; ++iter ) {
            if (iter->second->Status()) quit = true;
        }
    }

    if (quit)  _ABORT(Simulation, "Status()\n");
}

std::string Simulation::EventName( Event *e )
{
    std::string eventType = "Unknown"; 
    if ( dynamic_cast< StopEvent* >( e ) ) {
        eventType = "StopEvent";
    }
    if ( dynamic_cast< ClockEvent* >( e ) ) {
        eventType = "ClockEvent";
    }
//     if ( dynamic_cast< SyncEvent* >( e ) ) {
//         eventType = "SyncEvent";
//     }
    if ( dynamic_cast< Event* >( e ) ) {
        eventType = "Event";
    }
    return eventType;
}

TimeConverter* Simulation::registerClock( std::string freq, ClockHandler_t* handler )
{
//     _SIM_DBG("freq=%f handler=%p\n", frequency, handler );
    
    TimeConverter* tcFreq = timeLord->getTimeConverter(freq);

    if ( clockMap.find( tcFreq->getFactor() ) == clockMap.end() ) {
	_SIM_DBG( "\n" );
	ClockEvent* ce = new ClockEvent( tcFreq );
	clockMap[ tcFreq->getFactor() ] = ce; 

	ce->setDeliveryTime( currentSimCycle + tcFreq->getFactor() );
	timeVortex->insert( ce );
    }
    clockMap[ tcFreq->getFactor() ]->HandlerRegister( ClockEvent::DEFAULT, handler );
    return tcFreq;
    
}

void Simulation::unregisterClock(TimeConverter *tc, ClockHandler_t* handler) {
//     _SIM_DBG("freq=%f handler=%p\n", frequency, handler );
    
    if ( clockMap.find( tc->getFactor() ) != clockMap.end() ) {
	_SIM_DBG( "\n" );
	bool empty;
	clockMap[ tc->getFactor() ]->HandlerUnregister( 
						       ClockEvent::DEFAULT , handler, empty );
	
	if ( empty == 0 ) {
	    clockMap.erase( tc->getFactor() );
	}
    }
}

// void Simulation::insertEvent(SimTime_t time, Activity* ev, EventHandlerBase<bool,Activity*>* functor) {
void Simulation::insertActivity(SimTime_t time, Activity* ev) {
//     std::pair<EventHandlerBase<bool,Activity*>*,Activity*> envelope;
//     envelope.first = NULL;
//     envelope.second = ev;

//     eQueue->insert(time,envelope);
    ev->setDeliveryTime(time);
    timeVortex->insert(ev);
}


template<class Archive>
void
Simulation::serialize(Archive & ar, const unsigned int version)
{
    printf("begin Simulation::serialize\n");
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SimulationBase);
    printf("Simulation::serialize about to serialize timeVortex\n");
    ar & BOOST_SERIALIZATION_NVP(timeVortex);
    printf("Simulation::serialize about to serialize sync\n");
    ar & BOOST_SERIALIZATION_NVP(sync);
    printf("Simulation::serialize about to serialize compMap\n");
    ar & BOOST_SERIALIZATION_NVP(compMap);
    printf("Simulation::serialize about to serialize introMap\n");
    ar & BOOST_SERIALIZATION_NVP(introMap);
    printf("Simulation::serialize about to serialize clockMap\n");
    ar & BOOST_SERIALIZATION_NVP(clockMap);
    printf("Simulation::serialize about to serialize currentSimCycle\n");
    ar & BOOST_SERIALIZATION_NVP(currentSimCycle);
    printf("Simulation::serialize about to serialize m_exit\n");
    ar & BOOST_SERIALIZATION_NVP(m_exit);
    printf("Simulation::serialize about to serialize endSim\n");
    ar & BOOST_SERIALIZATION_NVP(endSim);
    printf("Simulation::serialize about to serialize component_links\n");
    ar & BOOST_SERIALIZATION_NVP(component_links);
    printf("end Simulation::serialize\n");
}

} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::SimulationBase::serialize)
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Simulation::serialize)
