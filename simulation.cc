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
#include <sst/core/simulation.h>

#include <utility>

#include <boost/foreach.hpp>

//#include <sst/core/archive.h>
#include <sst/core/clock.h>
#include <sst/core/config.h>
#include <sst/core/configGraph.h>
#include <sst/core/heartbeat.h>
//#include <sst/core/event.h>
#include <sst/core/exit.h>
#include <sst/core/factory.h>
//#include <sst/core/graph.h>
#include <sst/core/introspector.h>
#include <sst/core/linkMap.h>
#include <sst/core/linkPair.h>
#include <sst/core/sharedRegionImpl.h>
#include <sst/core/output.h>
#include <sst/core/stopAction.h>
#include <sst/core/stringize.h>
#include <sst/core/rankSync.h>
#include <sst/core/sync.h>
#include <sst/core/syncManager.h>
#include <sst/core/syncQueue.h>
#include <sst/core/threadSync.h>
#include <sst/core/timeLord.h>
#include <sst/core/timeVortex.h>
#include <sst/core/unitAlgebra.h>

#define SST_SIMTIME_MAX  0xffffffffffffffff

using namespace SST::Statistics;

namespace SST {



TimeConverter* Simulation::minPartToTC(SimTime_t cycles) const {
    return getTimeLord()->getTimeConverter(cycles);
}

void Simulation::signalStatisticsBegin() {
    statisticsOutput->startOfSimulation();
}


void Simulation::signalStatisticsEnd() {
    statisticsOutput->endOfSimulation();
}



Simulation::~Simulation()
{
    // Clean up as best we can

    // Delete the Statistic Objects
    delete statisticsEngine;

    // Delete the timeVortex first.  This will delete all events left
    // in the queue, as well as the Sync, Exit and Clock objects.
    delete timeVortex;

    if ( sync && (my_rank.thread == 0) ) delete sync;

    // Delete all the components
    // for ( CompMap_t::iterator it = compMap.begin(); it != compMap.end(); ++it ) {
	// delete it->second;
    // }
    // compMap.clear();
    
    // Delete all the introspectors
    for ( IntroMap_t::iterator it = introMap.begin(); it != introMap.end(); ++it ) {
        delete it->second;
    }
    introMap.clear();

    // Clocks already got deleted by timeVortex, simply clear the clockMap
    clockMap.clear();
    
    // OneShots already got deleted by timeVortex, simply clear the onsShotMap
    oneShotMap.clear();
    
    // // Delete any remaining links.  This should never happen now, but
    // // when we add an API to have components build subcomponents, user
    // // error could cause LinkMaps to be left.
    // std::map<ComponentId_t,LinkMap*>::iterator it;
    // for ( it = component_links.begin(); it != component_links.end(); ++it ) {
    //     delete it->second;
    // }
    // component_links.clear();

    // Delete the links    
    // for ( CompInfoMap_t::iterator it = compInfoMap.begin(); it != compInfoMap.end(); ++it ) {
    //     std::map<std::string,Link*>& map = it->second.link_map->getLinkMap();
    //     std::map<std::string,Link*>::iterator map_it;
    //     for ( map_it = map.begin(); map_it != map.end(); ++map_it ) {
    //         delete map_it->second;
    //     }
    // }

}

Simulation*
Simulation::createSimulation(Config *config, RankInfo my_rank, RankInfo num_ranks)
{
    std::thread::id tid = std::this_thread::get_id();
    Simulation* instance = new Simulation(config, my_rank, num_ranks);

    std::lock_guard<std::mutex> lock(simulationMutex);
    instanceMap[tid] = instance;
    instanceVec.resize(num_ranks.thread);
    instanceVec[my_rank.thread] = instance;
    return instance;
}


void Simulation::shutdown()
{
    instanceMap.clear();
}



Simulation::Simulation( Config* cfg, RankInfo my_rank, RankInfo num_ranks) :
    runMode(cfg->runMode),
    timeVortex(NULL),
    threadSync(NULL),
    currentSimCycle(0),
    endSimCycle(0),
    currentPriority(0),
    endSim(false),
    my_rank(my_rank),
    num_ranks(num_ranks),
    init_phase(0),
    lastRecvdSignal(0),
    shutdown_mode(SHUTDOWN_CLEAN),
    wireUpFinished(false)
{
    sim_output.init(cfg->output_core_prefix, cfg->getVerboseLevel(), 0, Output::STDOUT);
    output_directory = "";

    // Create the Statistic Processing Engine
    statisticsEngine = new StatisticProcessingEngine();

    timeVortex = new TimeVortex;
    if( my_rank.thread == 0 ) {
        m_exit = new Exit( num_ranks.thread, timeLord.getTimeConverter("100ns"), num_ranks.rank == 1 );
    }

    if(strcmp(cfg->heartbeatPeriod.c_str(), "N") != 0 && my_rank.thread == 0) {
        sim_output.output("# Creating simulation heartbeat at period of %s.\n", cfg->heartbeatPeriod.c_str());
    	m_heartbeat = new SimulatorHeartbeat(cfg, my_rank.rank, this, timeLord.getTimeConverter(cfg->heartbeatPeriod) );
    }

    // Need to create the thread sync if there is more than one thread
    if ( num_ranks.thread > 1 ) {
        threadSync = new ThreadSync(num_ranks.thread, this);
    }
}

void
Simulation::setStopAtCycle( Config* cfg )
{
    SimTime_t stopAt = timeLord.getSimCycles(cfg->stopAtCycle,"StopAction configure");
    if ( stopAt != 0 ) {
	StopAction* sa = new StopAction();
	sa->setDeliveryTime(stopAt);
	timeVortex->insert(sa);
    }
}

Simulation::Simulation()
{
    // serialization only, so everything will be restored.  I think.
}


Component*
Simulation::createComponent( ComponentId_t id, std::string &name, 
                             Params &params )
{
    return factory->CreateComponent(id, name, params);
}

Introspector*
Simulation::createIntrospector(std::string &name, Params &params )
{
    return factory->CreateIntrospector(name, params);
}

void
Simulation::requireEvent(std::string name)
{
    factory->RequireEvent(name);
}
    
SimTime_t
Simulation::getNextActivityTime() const
{
    return timeVortex->front()->getDeliveryTime();
}

SimTime_t
Simulation::getLocalMinimumNextActivityTime()
{
    SimTime_t ret = MAX_SIMTIME_T;
    for ( auto && instance : instanceVec ) {
        SimTime_t next = instance->getNextActivityTime();
        if ( next < ret ) {
            ret = next;
        }
    }

    return ret;
}

void
Simulation::processGraphInfo( ConfigGraph& graph, const RankInfo& myRank, SimTime_t min_part )
{
    // TraceFunction trace(CALL_INFO_LONG);    
    // Set minPartTC (only thread 0 will do this)
    if ( my_rank.thread == 0 ) {
        minPartTC = minPartToTC(min_part);
    }

    // Get the minimum latencies for links between the various threads
    interThreadLatencies.resize(num_ranks.thread);
    for ( int i = 0; i < interThreadLatencies.size(); i++ ) {
        interThreadLatencies[i] = MAX_SIMTIME_T;
    }

    interThreadDependencies = false;
    if ( num_ranks.thread > 1 ) {
        // Need to determine the lookahead for the thread synchronization
        ConfigComponentMap_t comps = graph.getComponentMap();
        ConfigLinkMap_t links = graph.getLinkMap();
        // Find the minimum latency across a partition
        for ( auto iter = links.begin(); iter != links.end(); ++iter ) {
            ConfigLink &clink = *iter;
            RankInfo rank[2];
            rank[0] = comps[clink.component[0]].rank;
            rank[1] = comps[clink.component[1]].rank;
            // We only care about links that are on the same rank, but
            // different threads
            // if ( rank[0] == rank[1] ) continue;
            // if ( rank[0].rank != rank[1].rank ) continue;
            if ( rank[0] == rank[1] || 
                 rank[0].rank != rank[1].rank ) continue;
            else interThreadDependencies = true;

            // Keep track of minimum latency for each other thread
            // separately
            if ( rank[0].thread == my_rank.thread) { 
                if ( clink.getMinLatency() < interThreadLatencies[rank[1].thread] ) {
                    interThreadLatencies[rank[1].thread] = clink.getMinLatency();
                }
            }
            else if ( rank[1].thread == my_rank.thread ) {
                if ( clink.getMinLatency() < interThreadLatencies[rank[0].thread] ) {
                    interThreadLatencies[rank[0].thread] = clink.getMinLatency();
                }
            }
        }
    }
    // Create the SyncManager for this rank.  It gets created even if
    // we are single rank/single thread because it also manages the
    // Exit and Heartbeat actions.
    syncManager = new SyncManager(my_rank, num_ranks, barrier, minPartTC = minPartToTC(min_part), interThreadLatencies);
}
    
int Simulation::performWireUp( ConfigGraph& graph, const RankInfo& myRank, SimTime_t min_part )
{
    // TraceFunction trace(CALL_INFO_LONG);    
    
    // Create the Statistics Output

    // Params objects should now start verifying parameters
    Params::enableVerify();

    // Need to create the sync objects.  There are two versions, one
    // that synchronizes between threads and one that synchronizes
    // between MPI ranks.

#if 0    
    if ( num_ranks.rank > 1 ) {        
        // For now, we are doing performWireUp serially, if this ever
        // changes, then need to serialize the creation of the sync
        // objects.
        if ( my_rank.thread == 0 ) {
            sync = new SyncD(barrier);
            sync->setExit(m_exit);
            sync->setMaxPeriod( minPartTC = minPartToTC(min_part) );
            // Action* ms = sync->getMasterAction();
            // insertActivity(minPartTC->getFactor(), ms);
        }
        else {
            Action* ss = sync->getSlaveAction();
            // insertActivity(minPartTC->getFactor(), ss);
        }
        
        
    }

    // Need to determine the lookahead for the thread synchronization

    if ( num_ranks.thread > 1 ) {
        // Need to determine the lookahead for the thread synchronization
        SimTime_t look_ahead = 0xffffffffffffffffl;
        ConfigComponentMap_t comps = graph.getComponentMap();
        ConfigLinkMap_t links = graph.getLinkMap();
        // Find the minimum latency across a partition
        for ( auto iter = links.begin(); iter != links.end(); ++iter ) {
            ConfigLink &clink = *iter;
            RankInfo rank[2];
            rank[0] = comps[clink.component[0]].rank;
            rank[1] = comps[clink.component[1]].rank;
            // We only care about links that are on the same rank, but
            // different threads
            if ( rank[0] == rank[1] ) continue;
            if ( rank[0].rank != rank[1].rank ) continue;
            if ( clink.getMinLatency() < look_ahead ) {
                look_ahead = clink.getMinLatency();
            }
        }


        // Fix for case that probably doesn't matter in practice, but
        // does come up during some specific testing.  If there are no
        // links that cross the boundary and we're a single rank job,
        // we need to put in a sync interval to look for the exit
        // conditions being met.
        if ( look_ahead == MAX_SIMTIME_T ) {
            std::cout << "No links cross thread boundary" << std::endl;
        }
        if ( look_ahead == MAX_SIMTIME_T && num_ranks.rank == 1) {
            // std::cout << "No links cross thread boundary" << std::endl;
            look_ahead = timeLord.getSimCycles("1us","");
        }
        threadSync->setMaxPeriod( threadMinPartTC = minPartToTC(look_ahead) );
    }
#endif
    
    // First, go through all the components that are in this rank and
    // create the ComponentInfo object for it
    // Now, build all the components
    for ( ConfigComponentMap_t::iterator iter = graph.comps.begin();
            iter != graph.comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);
        if ( ccomp->rank == myRank ) {
            // compInfoMap[ccomp->id] = ComponentInfo(ccomp->name, ccomp->type, new LinkMap());
            compInfoMap.insert(new ComponentInfo(ccomp->id, ccomp->name, ccomp->type, new LinkMap()));
        }
    }
    
    
    // We will go through all the links and create LinkPairs for each
    // link.  We will also create a LinkMap for each component and put
    // them into a map with ComponentID as the key.
    for( ConfigLinkMap_t::iterator iter = graph.links.begin();
            iter != graph.links.end(); ++iter )
    {
        ConfigLink &clink = *iter;
        RankInfo rank[2];
        rank[0] = graph.comps[clink.component[0]].rank;
        rank[1] = graph.comps[clink.component[1]].rank;

        if ( rank[0] != myRank && rank[1] != myRank ) {
            // Nothing to be done
            continue;
        }
        // Same rank, same thread
        else if ( rank[0] == rank[1] ) {
            // Create a LinkPair to represent this link
            LinkPair lp(clink.id);

            lp.getLeft()->setLatency(clink.latency[0]);
            lp.getRight()->setLatency(clink.latency[1]);

            // Add this link to the appropriate LinkMap
            ComponentInfo* cinfo = compInfoMap.getByID(clink.component[0]);
            if ( cinfo == NULL ) {
                // This shouldn't happen and is an error
                sim_output.fatal(CALL_INFO,1,"Couldn't find ComponentInfo in map.");
            }
            cinfo->getLinkMap()->insertLink(clink.port[0],lp.getLeft());

            cinfo = compInfoMap.getByID(clink.component[1]);
            if ( cinfo == NULL ) {
                // This shouldn't happen and is an error
                sim_output.fatal(CALL_INFO,1,"Couldn't find ComponentInfo in map.");
            }
            cinfo->getLinkMap()->insertLink(clink.port[1],lp.getRight());

        }
        // If the components are not in the same thread, then the
        // SyncManager will handle things
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
            LinkPair lp(clink.id);

            lp.getLeft()->setLatency(clink.latency[local]);
            lp.getRight()->setLatency(0);
            lp.getRight()->setDefaultTimeBase(minPartToTC(1));

            // Add this link to the appropriate LinkMap for the local component
            ComponentInfo* cinfo = compInfoMap.getByID(clink.component[local]);
            if ( cinfo == NULL ) {
                // This shouldn't happen and is an error
                sim_output.fatal(CALL_INFO,1,"Couldn't find ComponentInfo in map.");
            }
            cinfo->getLinkMap()->insertLink(clink.port[local],lp.getLeft());

            // Need to register with both of the syncs (the ones for
            // both local and remote thread)

            // For local, just register link with threadSync object so
            // it can map link_id to link*
            ActivityQueue* sync_q = syncManager->registerLink(rank[remote],rank[local],clink.id,lp.getRight());

            lp.getRight()->configuredQueue = sync_q;
            lp.getRight()->initQueue = sync_q;
        }

/*        
        // Same rank, different thread
        else if ( rank[0].rank == rank[1].rank ) {
            // This is same MPI rank, different thread
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
            LinkPair lp(clink.id);

            lp.getLeft()->setLatency(clink.latency[local]);
            lp.getRight()->setLatency(0);
            lp.getRight()->setDefaultTimeBase(minPartToTC(1));


            // Add this link to the appropriate LinkMap for the local component
            ComponentInfo* cinfo = compInfoMap.getByID(clink.component[local]);
            if ( cinfo == NULL ) {
                // This shouldn't happen and is an error
                sim_output.fatal(CALL_INFO,1,"Couldn't find ComponentInfo in map.");
            }
            cinfo->getLinkMap()->insertLink(clink.port[local],lp.getLeft());

            // Need to register with both of the syncs (the ones for
            // both local and remote thread)

            // For local, just register link with threadSync object so
            // it can map link_id to link*
            threadSync->registerLink(clink.id, lp.getRight());

            // Fpor remote thread, I get the proper queue and finish
            // setting up the right link
            ActivityQueue* sync_q = instanceVec[rank[remote].thread]->threadSync->getQueueForThread(rank[local].thread);
            
            lp.getRight()->configuredQueue = sync_q;
            lp.getRight()->initQueue = sync_q;
        }
        // Different rank
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
            LinkPair lp(clink.id);

            lp.getLeft()->setLatency(clink.latency[local]);
            lp.getRight()->setLatency(0);
            lp.getRight()->setDefaultTimeBase(minPartToTC(1));


            // Add this link to the appropriate LinkMap for the local component
            ComponentInfo* cinfo = compInfoMap.getByID(clink.component[local]);
            if ( cinfo == NULL ) {
                // This shouldn't happen and is an error
                sim_output.fatal(CALL_INFO,1,"Couldn't find ComponentInfo in map.");
            }
            cinfo->getLinkMap()->insertLink(clink.port[local],lp.getLeft());

            // For the remote side, register with sync object
            ActivityQueue* sync_q = sync->registerLink(rank[remote],rank[local],clink.id,lp.getRight());
            // lp.getRight()->recvQueue = sync_q;
            lp.getRight()->configuredQueue = sync_q;
            lp.getRight()->initQueue = sync_q;
        }
*/
    }

    // Done with that edge, delete it.
//    graph.links.clear();

    // Now, build all the components
    for ( auto iter = graph.comps.begin(); iter != graph.comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);

        if (ccomp->isIntrospector) {
            Introspector* tmp;

            // _SIM_DBG("creating introspector: name=\"%s\" type=\"%s\" id=%d\n",
            // 	     name.c_str(), sdl_c->type().c_str(), (int)id );

            tmp = createIntrospector( ccomp->type, ccomp->params );
            introMap[ccomp->name] = tmp;
        }
        else if ( ccomp->rank == myRank ) {
            Component* tmp;
            //             _SIM_DBG("creating component: name=\"%s\" type=\"%s\" id=%d\n",
            // 		     name.c_str(), sdl_c->type().c_str(), (int)id );

            // Check to make sure there are any entries in the component's LinkMap
            if ( compInfoMap.getByID(ccomp->id)->getLinkMap()->empty() ) {
                printf("WARNING: Building component \"%s\" with no links assigned.\n",ccomp->name.c_str());
            }

            // Save off what statistics can be enabled before instantiating the component
            // This allows the component to register its statistics in its constructor.
            statisticEnableMap[ccomp->id] = &(ccomp->enabledStatistics);
            statisticParamsMap[ccomp->id] = &(ccomp->enabledStatParams);
            
            // compIdMap[ccomp->id] = ccomp->name;
            tmp = createComponent( ccomp->id, ccomp->type,
                    ccomp->params );
            // compMap[ccomp->name] = tmp;
            compInfoMap.getByName(ccomp->name)->setComponent(tmp);
            
            // After component is created, clear out the statisticEnableMap so 
            // we dont eat up lots of memory
            statisticEnableMap.erase(ccomp->id);
            statisticParamsMap.erase(ccomp->id);
        }
    } // end for all vertex    
    // Done with verticies, delete them;
    /*  TODO:  THREADING:  Clear only once everybody is done.
    graph.comps.clear();
    statisticEnableMap.clear();
    statisticParamsMap.clear();
    */
    wireUpFinished = true;
    // std::cout << "Done with performWireUp" << std::endl;
    return 0;
}

void Simulation::initialize() {
    // TraceFunction trace(CALL_INFO_LONG);    
    bool done = false;
    barrier.wait();
    if ( my_rank.thread == 0 ) sharedRegionManager->updateState(false);

    do {

        barrier.wait();
        if ( my_rank.thread == 0 ) init_msg_count = 0;
        barrier.wait();
        
        
        for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter ) {
            (*iter)->getComponent()->init(init_phase);
        }

        barrier.wait();
        syncManager->exchangeLinkInitData(init_msg_count);
        barrier.wait();
        // We're done if no new messages were sent
        if ( init_msg_count == 0 ) done = true;
        if ( my_rank.thread == 0 ) sharedRegionManager->updateState(false);

        init_phase++;
    } while ( !done);

    // Walk through all the links and call finalizeConfiguration
    for ( auto i = compInfoMap.begin(); i != compInfoMap.end(); ++i) {
        std::map<std::string,Link*>& map = (*i)->getLinkMap()->getLinkMap();
        for ( std::map<std::string,Link*>::iterator j = map.begin(); j != map.end(); ++j ) {
            (*j).second->finalizeConfiguration();
        }
    }
#if 0
    if ( num_ranks.rank > 1 && my_rank.thread == 0 ) {
        sync->finalizeLinkConfigurations();
    }

    if ( num_ranks.thread > 1 ) {
        threadSync->finalizeLinkConfigurations();
    }
#endif
    syncManager->finalizeLinkConfigurations();

}

void Simulation::setup() {  

    // TraceFunction(CALL_INFO_LONG);    
    // Output tmp_debug("@r: @t:  ",5,-1,Output::FILE);

    barrier.wait();
    
    for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter ) {
        (*iter)->getComponent()->setup();
    }

    barrier.wait();

    //for introspector
    for( IntroMap_t::iterator iter = introMap.begin();
                            iter != introMap.end(); ++iter )
    {
      (*iter).second->setup();
    }

    barrier.wait();
    /* Enforce finalization of shared regions */
    if ( my_rank.thread == 0 ) sharedRegionManager->updateState(true);

}

void Simulation::run() {
    // TraceFunction trace(CALL_INFO_LONG);

    // Put a stop event at the end of the timeVortex. Simulation will
    // only get to this is there are no other events in the queue.
    // In general, this shouldn't happen, especially for parallel
    // simulations. If it happens in a parallel simulation the 
    // simulation will likely deadlock as only some of the ranks
    // will hit the anomoly.
    StopAction* sa = new StopAction("*** Event queue empty, exiting simulation... ***");
    sa->setDeliveryTime(SST_SIMTIME_MAX);
    timeVortex->insert(sa);

    // Tell the Statistics Engine that the simulation is beginning
    statisticsEngine->startOfSimulation();

    // wait_my_turn_start(barrier, my_rank.thread, num_ranks.thread);
    // sim_output.output("%d: Start main event loop\n",my_rank.thread);
    std::string header = SST::to_string(my_rank.rank);
    header += ", ";
    header += SST::to_string(my_rank.thread);
    header += ":  ";
    // wait_my_turn_end(barrier, my_rank.thread, num_ranks.thread);
    while( LIKELY( ! endSim ) ) {
        currentSimCycle = timeVortex->front()->getDeliveryTime();
        currentPriority = timeVortex->front()->getPriority();
        current_activity = timeVortex->pop();
        // current_activity->print(header, sim_output);
        current_activity->execute();


        if ( UNLIKELY( 0 != lastRecvdSignal ) ) {
            switch ( lastRecvdSignal ) {
            case SIGUSR1: printStatus(false); break;
            case SIGUSR2: printStatus(true); break;
            case SIGINT:
            case SIGTERM:
                ThreadSync::disable();
                shutdown_mode = SHUTDOWN_SIGNAL;
                sim_output.output("EMERGENCY SHUTDOWN (%u,%u)!\n",
                        my_rank.rank, my_rank.thread);
                sim_output.output("# Simulated time:                  %s\n",
                        getElapsedSimTime().toStringBestSI().c_str());
                endSim = true;
                break;
            default: break;
            }
            lastRecvdSignal = 0;
        }
    }
    /* We shouldn't need to do this, but to be safe... */
    ThreadSync::disable();

    // fprintf(stderr, "thread %u waiting on runLoop finish barrier\n", my_rank.thread);
    barrier.wait();  // TODO<- Is this needed?
    // fprintf(stderr, "thread %u released from runLoop finish barrier\n", my_rank.thread);
    if (num_ranks.rank != 1 && num_ranks.thread == 0) delete m_exit;


    // Tell the Statistics Engine that the simulation is ending
    statisticsEngine->endOfSimulation();
}


void Simulation::emergencyShutdown()
{
    std::lock_guard<std::mutex> lock(simulationMutex);

    for ( auto && instance : instanceVec ) {
        instance->shutdown_mode = SHUTDOWN_EMERGENCY;
        instance->endSim = true;
        //// Function not available with gcc 4.6
        //atomic_thread_fence(std::memory_order_acquire);
        for ( auto && iter : instance->compInfoMap ) {
            if ( iter->getComponent() )
                iter->getComponent()->emergencyShutdown();
        }
    }

}


void Simulation::endSimulation(SimTime_t end)
{
    // shutdown_mode = SHUTDOWN_CLEAN;

    // This is a collective operation across threads.  All threads
    // must enter and set flag before any will exit.
    // exit_barrier.wait();

    endSimCycle = end;
    endSim = true;

    exit_barrier.wait();


}
    

void Simulation::finish() {

    for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter )
    {
        (*iter)->getComponent()->finish();
    }
    //for introspector
    for( IntroMap_t::iterator iter = introMap.begin();
                            iter != introMap.end(); ++iter )
    {
      (*iter).second->finish();
    }

    switch ( shutdown_mode ) {
    case SHUTDOWN_CLEAN:
        break;
    case SHUTDOWN_SIGNAL:
    case SHUTDOWN_EMERGENCY:
        for ( auto && iter : compInfoMap ) {
            iter->getComponent()->emergencyShutdown();
        }
        sim_output.output("EMERGENCY SHUTDOWN Complete (%u,%u)!\n",
                my_rank.rank, my_rank.thread);
    }

}

const SimTime_t&
Simulation::getCurrentSimCycle() const
{
    return currentSimCycle; 
}

SimTime_t
Simulation::getEndSimCycle() const
{
    return endSimCycle; 
}

int
Simulation::getCurrentPriority() const
{
    return currentPriority; 
}


// void Simulation::getElapsedSimTime(double *value, char *prefix) const
// {
//     static const char siprefix[] = {
//         'a', 'f', 'p', 'n', 'u', 'm', ' '
//     };
//     const int max = sizeof(siprefix) / sizeof(char);

//     double val = getCurrentSimCycle() * 1000;
//     int lvl = 0;
//     while ( (lvl < max) && (val > 1000.0) ) {
//         val /= 1000.0;
//         ++lvl;
//     }
//     *value = val;
//     *prefix = siprefix[lvl];

// }

UnitAlgebra Simulation::getElapsedSimTime() const
{
    return timeLord.getTimeBase() * getCurrentSimCycle();
}

UnitAlgebra Simulation::getFinalSimTime() const
{
    return timeLord.getTimeBase() * getEndSimCycle();
}

void Simulation::setSignal(int signal)
{
    for ( auto &instance : instanceVec )
        instance->lastRecvdSignal = signal;
}

void Simulation::printStatus(bool fullStatus)
{
    Output out("SimStatus: @R:@t:", 0, 0, Output::STDERR);
    out.output("\tCurrentSimCycle:  %" PRIu64 "\n", currentSimCycle);

    if ( fullStatus ) {
        timeVortex->print(out);
        out.output("---- Components: ----\n");
        for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter ) {
            (*iter)->getComponent()->printStatus(out);
        }
    }


}

TimeConverter* Simulation::registerClock( std::string freq, Clock::HandlerBase* handler )
{
//     _SIM_DBG("freq=%f handler=%p\n", frequency, handler );
    
    TimeConverter* tcFreq = timeLord.getTimeConverter(freq);

    if ( clockMap.find( tcFreq->getFactor() ) == clockMap.end() ) {
        Clock* ce = new Clock( tcFreq );
        clockMap[ tcFreq->getFactor() ] = ce; 
        
        // ce->setDeliveryTime( currentSimCycle + tcFreq->getFactor() );
        // timeVortex->insert( ce );
        ce->schedule();
    }
    clockMap[ tcFreq->getFactor() ]->registerHandler( handler );
    return tcFreq;    
}

TimeConverter* Simulation::registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler)
{
    TimeConverter* tcFreq = timeLord.getTimeConverter(freq);
    
    if ( clockMap.find( tcFreq->getFactor() ) == clockMap.end() ) {
        Clock* ce = new Clock( tcFreq );
        clockMap[ tcFreq->getFactor() ] = ce; 

        ce->schedule();
    }
    clockMap[ tcFreq->getFactor() ]->registerHandler( handler );
    return tcFreq;
}

Cycle_t Simulation::reregisterClock( TimeConverter* tc, Clock::HandlerBase* handler )
{
    if ( clockMap.find( tc->getFactor() ) == clockMap.end() ) {
        Output out("Simulation: @R:@t:", 0, 0, Output::STDERR);
        out.fatal(CALL_INFO, 1, "Tried to reregister with a clock that was not previously registered, exiting...\n");
    }
    clockMap[ tc->getFactor() ]->registerHandler( handler );
    return clockMap[tc->getFactor()]->getNextCycle();
    
}

Cycle_t Simulation::getNextClockCycle(TimeConverter* tc) {
    if ( clockMap.find( tc->getFactor() ) == clockMap.end() ) {
	Output out("Simulation: @R:@t:", 0, 0, Output::STDERR);
	out.fatal(CALL_INFO, -1,
		  "Call to getNextClockCycle() on a clock that was not previously registered, exiting...\n");
    }
    return clockMap[tc->getFactor()]->getNextCycle();
}

void Simulation::unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler) {
//     _SIM_DBG("freq=%f handler=%p\n", frequency, handler );
    
    if ( clockMap.find( tc->getFactor() ) != clockMap.end() ) {
	bool empty;
	clockMap[ tc->getFactor() ]->unregisterHandler( handler, empty );
	
    }
}

TimeConverter* Simulation::registerOneShot(std::string timeDelay, OneShot::HandlerBase* handler)
{
    return registerOneShot(UnitAlgebra(timeDelay), handler);
}

TimeConverter* Simulation::registerOneShot(const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler)
{
    TimeConverter* tcTimeDelay = timeLord.getTimeConverter(timeDelay);

    // Search the oneShot map for a oneShot with the associated timeDelay factor
    if (oneShotMap.find(tcTimeDelay->getFactor()) == oneShotMap.end()) {
        // OneShot with the specific timeDelay not found, 
        // create a new one and add it to the map of OneShots
        OneShot* ose = new OneShot(tcTimeDelay);
        oneShotMap[tcTimeDelay->getFactor()] = ose; 
    }
    
    // Add the handler to the OneShots list of handlers, Also the
    // registerHandler will schedule the oneShot to fire in the future
    oneShotMap[tcTimeDelay->getFactor()]->registerHandler(handler);
    return tcTimeDelay;
}

void Simulation::insertActivity(SimTime_t time, Activity* ev) {
    ev->setDeliveryTime(time);
    timeVortex->insert(ev);

}

uint64_t Simulation::getTimeVortexMaxDepth() const {
    return timeVortex->getMaxDepth();
}

uint64_t Simulation::getTimeVortexCurrentDepth() const {
    return timeVortex->getCurrentDepth();
}

uint64_t Simulation::getSyncQueueDataSize() const {
    if ( num_ranks.rank == 1 || my_rank.thread > 0 ) return 0;
    return sync->getDataSize();
}

    
template<class Archive>
void
Simulation::serialize(Archive & ar, const unsigned int version)
{
    printf("begin Simulation::serialize\n");

    ar & BOOST_SERIALIZATION_NVP(output_directory);

    printf("  - Simulation::timeVortex\n");
    ar & BOOST_SERIALIZATION_NVP(timeVortex);

    printf("  - Simulation::sync (%p)\n", sync);
    ar & BOOST_SERIALIZATION_NVP(sync);

    // printf("  - Simulation::compMap\n");
    // ar & BOOST_SERIALIZATION_NVP(compMap);

    printf("  - Simulation::introMap\n");
    ar & BOOST_SERIALIZATION_NVP(introMap);
    printf("  - Simulation::clockMap\n");
    ar & BOOST_SERIALIZATION_NVP(clockMap);
    printf("  - Simulation::oneShotMap\n");
    ar & BOOST_SERIALIZATION_NVP(oneShotMap);
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
    printf("  - Simulation::statisticsEngine\n");
    ar & BOOST_SERIALIZATION_NVP(statisticsEngine);

    // printf("  - Simulation::compInfoMap\n");
    // ar & BOOST_SERIALIZATION_NVP(compInfoMap);

    printf("end Simulation::serialize\n");
}


// Function to allow for easy serialization of threads while debugging
// code
void wait_my_turn_start(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads) {
    // Everyone barriers
    barrier.wait();
    // Now barrier until it's my turn
    for ( int i = 0; i < thread; i++ ) {
        barrier.wait();
    }
}

// Uses Simulation's barrier
void wait_my_turn_start() {
    Core::ThreadSafe::Barrier& barrier = Simulation::getThreadBarrier();
    int thread = Simulation::getSimulation()->my_rank.thread;
    int total_threads = Simulation::getSimulation()->num_ranks.thread;
    wait_my_turn_start(barrier, thread, total_threads);
}

void wait_my_turn_end(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads) {

    // Wait for all the threads after me to finish
    for ( int i = thread; i < total_threads; i++ ) {
        barrier.wait();
    }
    // All barrier
    barrier.wait();
}

// Uses Simulation's barrier
void wait_my_turn_end() {
    Core::ThreadSafe::Barrier& barrier = Simulation::getThreadBarrier();
    int thread = Simulation::getSimulation()->my_rank.thread;
    int total_threads = Simulation::getSimulation()->num_ranks.thread;
    wait_my_turn_end(barrier, thread, total_threads);
}


/* Define statics */
Factory* Simulation::factory;
TimeLord Simulation::timeLord;
Statistics::StatisticOutput* Simulation::statisticsOutput;
Output Simulation::sim_output;
Core::ThreadSafe::Barrier Simulation::barrier;
Core::ThreadSafe::Barrier Simulation::exit_barrier;
std::mutex Simulation::simulationMutex;
TimeConverter* Simulation::minPartTC = NULL;
SyncBase* Simulation::sync = NULL;



/* Define statics (Simulation) */
SharedRegionManager* Simulation::sharedRegionManager = new SharedRegionManagerImpl();
std::unordered_map<std::thread::id, Simulation*> Simulation::instanceMap;
std::vector<Simulation*> Simulation::instanceVec;
std::atomic<int> Simulation::init_msg_count;
Exit* Simulation::m_exit;


} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Simulation::serialize)

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Simulation)
