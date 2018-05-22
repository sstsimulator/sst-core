// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include <sst/core/simulation.h>
#include <sst/core/warnmacros.h>

#include <utility>

//#include <sst/core/archive.h>
#include <sst/core/clock.h>
#include <sst/core/config.h>
#include <sst/core/configGraph.h>
#include <sst/core/heartbeat.h>
//#include <sst/core/event.h>
#include <sst/core/exit.h>
#include <sst/core/factory.h>
//#include <sst/core/graph.h>
#include <sst/core/linkMap.h>
#include <sst/core/linkPair.h>
#include <sst/core/sharedRegionImpl.h>
#include <sst/core/output.h>
#include <sst/core/stopAction.h>
#include <sst/core/stringize.h>
#include <sst/core/syncBase.h>
#include <sst/core/syncManager.h>
#include <sst/core/syncQueue.h>
#include <sst/core/threadSync.h>
#include <sst/core/timeConverter.h>
#include <sst/core/timeLord.h>
#include <sst/core/timeVortex.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/statapi/statengine.h>

#define SST_SIMTIME_MAX  0xffffffffffffffff

using namespace SST::Statistics;

namespace SST {



TimeConverter* Simulation::minPartToTC(SimTime_t cycles) const {
    return getTimeLord()->getTimeConverter(cycles);
}


Simulation::~Simulation()
{
    // Clean up as best we can

    // Delete the timeVortex first.  This will delete all events left
    // in the queue, as well as the Sync, Exit and Clock objects.
    delete timeVortex;

    // Delete all the components
    // for ( CompMap_t::iterator it = compMap.begin(); it != compMap.end(); ++it ) {
	// delete it->second;
    // }
    // compMap.clear();
    

    // Clocks already got deleted by timeVortex, simply clear the clockMap
    clockMap.clear();
    
    // OneShots already got deleted by timeVortex, simply clear the onsShotMap
    oneShotMap.clear();

    // Clear out Components
    compInfoMap.clear();
    
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
Simulation::createSimulation(Config *config, RankInfo my_rank, RankInfo num_ranks, SimTime_t min_part)
{
    std::thread::id tid = std::this_thread::get_id();
    Simulation* instance = new Simulation(config, my_rank, num_ranks, min_part);

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



Simulation::Simulation( Config* cfg, RankInfo my_rank, RankInfo num_ranks, SimTime_t min_part) :
    runMode(cfg->runMode),
    timeVortex(NULL),
    interThreadMinLatency(MAX_SIMTIME_T),
    threadSync(NULL),
    currentSimCycle(0),
    endSimCycle(0),
    currentPriority(0),
    endSim(false),
    my_rank(my_rank),
    num_ranks(num_ranks),
    untimed_phase(0),
    lastRecvdSignal(0),
    shutdown_mode(SHUTDOWN_CLEAN),
    wireUpFinished(false)
{
    sim_output.init(cfg->output_core_prefix, cfg->getVerboseLevel(), 0, Output::STDOUT);
    output_directory = "";

    Params p;
    timeVortex = static_cast<TimeVortex*>(factory->CreateModule(cfg->timeVortex,p));
    if( my_rank.thread == 0 ) {
        m_exit = new Exit( num_ranks.thread, timeLord.getTimeConverter("100ns"), min_part == MAX_SIMTIME_T );
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
Simulation::processGraphInfo( ConfigGraph& graph, const RankInfo& UNUSED(myRank), SimTime_t min_part )
{
    // Set minPartTC (only thread 0 will do this)
    Simulation::minPart = min_part;
    if ( my_rank.thread == 0 ) {
        minPartTC = minPartToTC(min_part);
    }

    // Get the minimum latencies for links between the various threads
    interThreadLatencies.resize(num_ranks.thread);
    for ( size_t i = 0; i < interThreadLatencies.size(); i++ ) {
        interThreadLatencies[i] = MAX_SIMTIME_T;
    }

    interThreadMinLatency = MAX_SIMTIME_T;
    int cross_thread_links = 0;
    if ( num_ranks.thread > 1 ) {
        // Need to determine the lookahead for the thread synchronization
        ConfigComponentMap_t comps = graph.getComponentMap();
        ConfigLinkMap_t links = graph.getLinkMap();
        // Find the minimum latency across a partition
        for ( auto iter = links.begin(); iter != links.end(); ++iter ) {
            ConfigLink &clink = *iter;
            RankInfo rank[2];
            rank[0] = comps[COMPONENT_ID_MASK(clink.component[0])].rank;
            rank[1] = comps[COMPONENT_ID_MASK(clink.component[1])].rank;
            // We only care about links that are on my rank, but
            // different threads

            // Neither endpoint is on my rank
            if ( rank[0].rank != my_rank.rank && rank[1].rank != my_rank.rank ) continue;
            // Rank and thread are the same
            if ( rank[0] == rank[1] ) continue;
            // Different ranks, so doesn't affect interthread dependencies
            if ( rank[0].rank != rank[1].rank ) continue;

            // At this point, we know that both endpoints are on this
            // rank, but on different threads.  Therefore, they
            // contribute to the interThreadMinLatency.
            cross_thread_links++;
            if ( clink.getMinLatency() < interThreadMinLatency ) {
                interThreadMinLatency = clink.getMinLatency();
            }

            // Now check only those latencies that directly impact this
            // thread.  Keep track of minimum latency for each other
            // thread separately
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
    syncManager = new SyncManager(my_rank, num_ranks, minPartTC = minPartToTC(min_part), min_part, interThreadLatencies);

    // Determine if this thread is independent.  That means there is
    // no need to synchronize with any other threads or ranks.
    if ( min_part == MAX_SIMTIME_T && cross_thread_links == 0 ) {
        independent = true;
    }
    else {
        independent = false;
    }
}
    
int Simulation::performWireUp( ConfigGraph& graph, const RankInfo& myRank, SimTime_t UNUSED(min_part))
{
    // Create the Statistics Engine
    if ( myRank.thread == 0 ) {
    }

    // Params objects should now start verifying parameters
    Params::enableVerify();


    // First, go through all the components that are in this rank and
    // create the ComponentInfo object for it
    // Now, build all the components
    for ( ConfigComponentMap_t::iterator iter = graph.comps.begin();
            iter != graph.comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);
        if ( ccomp->rank == myRank ) {
            compInfoMap.insert(new ComponentInfo(ccomp, ccomp->name, new LinkMap()));
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
        rank[0] = graph.comps[COMPONENT_ID_MASK(clink.component[0])].rank;
        rank[1] = graph.comps[COMPONENT_ID_MASK(clink.component[1])].rank;

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
            lp.getRight()->untimedQueue = sync_q;
        }

    }

    // Done with that edge, delete it.
//    graph.links.clear();

    // Now, build all the components
    for ( auto iter = graph.comps.begin(); iter != graph.comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);

        if ( ccomp->rank == myRank ) {
            Component* tmp;

            // Check to make sure there are any entries in the component's LinkMap
            // TODO:  IS this still a valid warning?  Subcomponents may be the link owners
            ComponentInfo *cinfo = compInfoMap.getByID(ccomp->id);
            if ( cinfo->getAllLinkIds().empty() ) {
                printf("WARNING: Building component \"%s\" with no links assigned.\n",ccomp->name.c_str());
            }

            tmp = createComponent( ccomp->id, ccomp->type, ccomp->params );

            cinfo->setComponent(tmp);
        }
    } // end for all vertex
    // Done with vertices, delete them;
    /*  TODO:  THREADING:  Clear only once everybody is done.
    graph.comps.clear();
    */
    wireUpFinished = true;
    // std::cout << "Done with performWireUp" << std::endl;
    return 0;
}

void Simulation::initialize() {
    bool done = false;
    initBarrier.wait();
    if ( my_rank.thread == 0 ) sharedRegionManager->updateState(false);

    do {
        initBarrier.wait();
        if ( my_rank.thread == 0 ) untimed_msg_count = 0;
        initBarrier.wait();
                
        for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter ) {
            // printf("Calling init on %s: %p\n",(*iter)->getName().c_str(),(*iter)->getComponent());
            (*iter)->getComponent()->init(untimed_phase);
        }

        initBarrier.wait();
        syncManager->exchangeLinkUntimedData(untimed_msg_count);
        initBarrier.wait();
        // We're done if no new messages were sent
        if ( untimed_msg_count == 0 ) done = true;
        if ( my_rank.thread == 0 ) sharedRegionManager->updateState(false);

        untimed_phase++;
    } while ( !done);

    // Walk through all the links and call finalizeConfiguration

    for ( auto &i : compInfoMap ) {
        i->finalizeLinkConfiguration();
    }
#if 0
    for ( auto i = compInfoMap.begin(); i != compInfoMap.end(); ++i) {
        std::map<std::string,Link*>& map = (*i)->getLinkMap()->getLinkMap();
        for ( std::map<std::string,Link*>::iterator j = map.begin(); j != map.end(); ++j ) {
            (*j).second->finalizeConfiguration();
        }
    }
#endif
    syncManager->finalizeLinkConfigurations();

}

void Simulation::complete() {

    completeBarrier.wait();
    untimed_phase = 0;
    // Walk through all the links and call prepareForComplete()
    for ( auto &i : compInfoMap ) {
        i->prepareForComplete();
    }

    syncManager->prepareForComplete();

    bool done = false;
    completeBarrier.wait();

    do {
        completeBarrier.wait();
        if ( my_rank.thread == 0 ) untimed_msg_count = 0;
        completeBarrier.wait();
                
        for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter ) {
            (*iter)->getComponent()->complete(untimed_phase);
        }

        completeBarrier.wait();
        syncManager->exchangeLinkUntimedData(untimed_msg_count);
        completeBarrier.wait();
        // We're done if no new messages were sent
        if ( untimed_msg_count == 0 ) done = true;

        untimed_phase++;
    } while ( !done);


}

void Simulation::setup() {  

    setupBarrier.wait();
    
    for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter ) {
        (*iter)->getComponent()->setup();
    }

    setupBarrier.wait();

    /* Enforce finalization of shared regions */
    if ( my_rank.thread == 0 ) sharedRegionManager->updateState(true);

}

void Simulation::run() {
    // Put a stop event at the end of the timeVortex. Simulation will
    // only get to this is there are no other events in the queue.
    // In general, this shouldn't happen, especially for parallel
    // simulations. If it happens in a parallel simulation the 
    // simulation will likely deadlock as only some of the ranks
    // will hit the anomaly.
    StopAction* sa = new StopAction("*** Event queue empty, exiting simulation... ***");
    sa->setDeliveryTime(SST_SIMTIME_MAX);
    timeVortex->insert(sa);

    // If this is an independent thread and we have no components,
    // just end
    if ( independent ) {
        if ( compInfoMap.empty() ) {
            // std::cout << "Thread " << my_rank.thread << " is exiting with nothing to do" << std::endl;
            StopAction* sa = new StopAction();
            sa->setDeliveryTime(0);
            timeVortex->insert(sa);
        }
    }
    
    // Tell the Statistics Engine that the simulation is beginning
    if ( my_rank.thread == 0 )
        StatisticProcessingEngine::getInstance()->startOfSimulation();

    std::string header = SST::to_string(my_rank.rank);
    header += ", ";
    header += SST::to_string(my_rank.thread);
    header += ":  ";
    while( LIKELY( ! endSim ) ) {
        currentSimCycle = timeVortex->front()->getDeliveryTime();
        currentPriority = timeVortex->front()->getPriority();
        current_activity = timeVortex->pop();
        current_activity->execute();


        if ( UNLIKELY( 0 != lastRecvdSignal ) ) {
            switch ( lastRecvdSignal ) {
            case SIGUSR1: printStatus(false); break;
            case SIGUSR2: printStatus(true); break;
            case SIGALRM:
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

    runBarrier.wait();  // TODO<- Is this needed?
    if (num_ranks.rank != 1 && num_ranks.thread == 0) delete m_exit;
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
    // exitBarrier.wait();

    // if ( my_rank.thread == 1 ) sim_output.fatal(CALL_INFO,-1,"endSimulation called with end = %llu\n", end);
    endSimCycle = end;
    endSim = true;

    exitBarrier.wait();


}
    

void Simulation::finish() {

    currentSimCycle = endSimCycle;

    for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter )
    {
        (*iter)->getComponent()->finish();
    }

    finishBarrier.wait();
    
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

    finishBarrier.wait();
    
    // Tell the Statistics Engine that the simulation is ending
    if ( my_rank.thread == 0 ) {
        StatisticProcessingEngine::getInstance()->endOfSimulation();
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

TimeConverter* Simulation::registerClock(const std::string& freq, Clock::HandlerBase* handler, int priority)
{
    TimeConverter* tcFreq = timeLord.getTimeConverter(freq);
    return registerClock(tcFreq, handler, priority);
}

TimeConverter* Simulation::registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority)
{
    TimeConverter* tcFreq = timeLord.getTimeConverter(freq);
    return registerClock(tcFreq, handler, priority);
}

TimeConverter* Simulation::registerClock(TimeConverter *tcFreq, Clock::HandlerBase* handler, int priority)
{
    clockMap_t::key_type mapKey = std::make_pair(tcFreq->getFactor(), priority);
    if ( clockMap.find( mapKey ) == clockMap.end() ) {
        Clock* ce = new Clock( tcFreq, priority );
        clockMap[ mapKey ] = ce;

        ce->schedule();
    }
    clockMap[ mapKey ]->registerHandler( handler );
    return tcFreq;
}


Cycle_t Simulation::reregisterClock( TimeConverter* tc, Clock::HandlerBase* handler, int priority )
{
    clockMap_t::key_type mapKey = std::make_pair(tc->getFactor(), priority);
    if ( clockMap.find( mapKey ) == clockMap.end() ) {
        Output out("Simulation: @R:@t:", 0, 0, Output::STDERR);
        out.fatal(CALL_INFO, 1, "Tried to reregister with a clock that was not previously registered, exiting...\n");
    }
    clockMap[ mapKey ]->registerHandler( handler );
    return clockMap[ mapKey ]->getNextCycle();
}

Cycle_t Simulation::getNextClockCycle(TimeConverter* tc, int priority) {
    clockMap_t::key_type mapKey = std::make_pair(tc->getFactor(), priority);
    if ( clockMap.find( mapKey ) == clockMap.end() ) {
        Output out("Simulation: @R:@t:", 0, 0, Output::STDERR);
        out.fatal(CALL_INFO, -1,
                "Call to getNextClockCycle() on a clock that was not previously registered, exiting...\n");
    }
    return clockMap[ mapKey ]->getNextCycle();
}

void Simulation::unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler, int priority) {
    clockMap_t::key_type mapKey = std::make_pair(tc->getFactor(), priority);
    if ( clockMap.find( mapKey ) != clockMap.end() ) {
        bool empty;
        clockMap[ mapKey ]->unregisterHandler( handler, empty );
    }
}

TimeConverter* Simulation::registerOneShot(std::string timeDelay, OneShot::HandlerBase* handler, int priority)
{
    return registerOneShot(UnitAlgebra(timeDelay), handler, priority);
}

TimeConverter* Simulation::registerOneShot(const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler, int priority)
{
    TimeConverter* tcTimeDelay = timeLord.getTimeConverter(timeDelay);
    clockMap_t::key_type mapKey = std::make_pair(tcTimeDelay->getFactor(), priority);

    // Search the oneShot map for a oneShot with the associated timeDelay factor
    if (oneShotMap.find( mapKey ) == oneShotMap.end()) {
        // OneShot with the specific timeDelay not found,
        // create a new one and add it to the map of OneShots
        OneShot* ose = new OneShot(tcTimeDelay, priority);
        oneShotMap[ mapKey ] = ose;
    }

    // Add the handler to the OneShots list of handlers, Also the
    // registerHandler will schedule the oneShot to fire in the future
    oneShotMap[ mapKey ]->registerHandler(handler);
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
    return syncManager->getDataSize();
}

Statistics::StatisticProcessingEngine* Simulation::getStatisticsProcessingEngine(void) const
{
    return Statistics::StatisticProcessingEngine::getInstance();
}

// Function to allow for easy serialization of threads while debugging
// code
void wait_my_turn_start(Core::ThreadSafe::Barrier& barrier, int thread, int UNUSED(total_threads)) {
    // Everyone barriers
    barrier.wait();
    // Now barrier until it's my turn
    for ( int i = 0; i < thread; i++ ) {
        barrier.wait();
    }
}

void wait_my_turn_end(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads) {

    // Wait for all the threads after me to finish
    for ( int i = thread; i < total_threads; i++ ) {
        barrier.wait();
    }
    // All barrier
    barrier.wait();
}



void Simulation::resizeBarriers(uint32_t nthr) {
    initBarrier.resize(nthr);
    completeBarrier.resize(nthr);
    setupBarrier.resize(nthr);
    runBarrier.resize(nthr);
    exitBarrier.resize(nthr);
    finishBarrier.resize(nthr);
}


/* Define statics */
Factory* Simulation::factory;
TimeLord Simulation::timeLord;
Output Simulation::sim_output;
Core::ThreadSafe::Barrier Simulation::initBarrier;
Core::ThreadSafe::Barrier Simulation::completeBarrier;
Core::ThreadSafe::Barrier Simulation::setupBarrier;
Core::ThreadSafe::Barrier Simulation::runBarrier;
Core::ThreadSafe::Barrier Simulation::exitBarrier;
Core::ThreadSafe::Barrier Simulation::finishBarrier;
std::mutex Simulation::simulationMutex;
TimeConverter* Simulation::minPartTC = NULL;
SimTime_t Simulation::minPart;



/* Define statics (Simulation) */
SharedRegionManager* Simulation::sharedRegionManager = new SharedRegionManagerImpl();
std::unordered_map<std::thread::id, Simulation*> Simulation::instanceMap;
std::vector<Simulation*> Simulation::instanceVec;
std::atomic<int> Simulation::untimed_msg_count;
Exit* Simulation::m_exit;


} // namespace SST
