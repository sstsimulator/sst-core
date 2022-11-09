// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/simulation_impl.h"
// simulation_impl header should stay here

#include "sst/core/clock.h"
#include "sst/core/config.h"
#include "sst/core/configGraph.h"
#include "sst/core/exit.h"
#include "sst/core/factory.h"
#include "sst/core/heartbeat.h"
#include "sst/core/linkMap.h"
#include "sst/core/linkPair.h"
#include "sst/core/output.h"
#include "sst/core/profile/clockHandlerProfileTool.h"
#include "sst/core/profile/eventHandlerProfileTool.h"
#include "sst/core/profile/syncProfileTool.h"
#include "sst/core/shared/sharedObject.h"
#include "sst/core/statapi/statengine.h"
#include "sst/core/stopAction.h"
#include "sst/core/stringize.h"
#include "sst/core/sync/syncManager.h"
#include "sst/core/sync/syncQueue.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/timeVortex.h"
#include "sst/core/unitAlgebra.h"
#include "sst/core/warnmacros.h"

#include <cinttypes>
#include <utility>

#define SST_SIMTIME_MAX 0xffffffffffffffff

using namespace SST::Statistics;
using namespace SST::Shared;

namespace SST {

/**   Simulation functions **/

/** Static functions **/
Simulation*
Simulation::getSimulation()
{
    return Simulation_impl::instanceMap.at(std::this_thread::get_id());
}

TimeLord*
Simulation::getTimeLord()
{
    return &Simulation_impl::timeLord;
}

Output&
Simulation::getSimulationOutput()
{
    return Simulation_impl::sim_output;
}

/** Non-static functions **/
SimTime_t
Simulation_impl::getCurrentSimCycle() const
{
    return currentSimCycle;
}

SimTime_t
Simulation_impl::getEndSimCycle() const
{
    return endSimCycle;
}

int
Simulation_impl::getCurrentPriority() const
{
    return currentPriority;
}

UnitAlgebra
Simulation_impl::getElapsedSimTime() const
{
    return timeLord.getTimeBase() * getCurrentSimCycle();
}

UnitAlgebra
Simulation_impl::getEndSimTime() const
{
    return timeLord.getTimeBase() * getEndSimCycle();
}

UnitAlgebra
Simulation_impl::getFinalSimTime() const
{
    return timeLord.getTimeBase() * getEndSimCycle();
}

Simulation::~Simulation() {}

/** Simulation_impl functions **/

TimeConverter*
Simulation_impl::minPartToTC(SimTime_t cycles) const
{
    return getTimeLord()->getTimeConverter(cycles);
}

Simulation_impl::~Simulation_impl()
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

    // Clean up the profile tools
    for ( auto x : profile_tools )
        delete x.second;

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

Simulation_impl*
Simulation_impl::createSimulation(Config* config, RankInfo my_rank, RankInfo num_ranks)
{
    std::thread::id  tid      = std::this_thread::get_id();
    Simulation_impl* instance = new Simulation_impl(config, my_rank, num_ranks);

    std::lock_guard<std::mutex> lock(simulationMutex);
    instanceMap[tid] = instance;
    instanceVec.resize(num_ranks.thread);
    instanceVec[my_rank.thread] = instance;
    instance->intializeDefaultProfileTools(config->enabledProfiling());
    return instance;
}

void
Simulation_impl::shutdown()
{
    instanceMap.clear();
}

Simulation_impl::Simulation_impl(Config* cfg, RankInfo my_rank, RankInfo num_ranks) :
    Simulation(),
    timeVortex(nullptr),
    interThreadMinLatency(MAX_SIMTIME_T),
    endSim(false),
    untimed_phase(0),
    lastRecvdSignal(0),
    shutdown_mode(SHUTDOWN_CLEAN),
    wireUpFinished(false),
    runMode(cfg->runMode()),
    currentSimCycle(0),
    endSimCycle(0),
    currentPriority(0),
    my_rank(my_rank),
    num_ranks(num_ranks),
    run_phase_start_time(0.0),
    run_phase_total_time(0.0),
    init_phase_start_time(0.0),
    init_phase_total_time(0.0),
    complete_phase_start_time(0.0),
    complete_phase_total_time(0.0)
{
    sim_output.init(cfg->output_core_prefix(), cfg->verbose(), 0, Output::STDOUT);
    output_directory = cfg->output_directory();
    Params p;
    // params get passed twice - both the params and a ctor argument
    direct_interthread = cfg->interthread_links();
    std::string timevortex_type(cfg->timeVortex());
    if ( direct_interthread && num_ranks.thread > 1 ) timevortex_type = timevortex_type + ".ts";
    timeVortex = factory->Create<TimeVortex>(timevortex_type, p);
    if ( my_rank.thread == 0 ) { m_exit = new Exit(num_ranks.thread, num_ranks.rank == 1); }

    if ( cfg->heartbeatPeriod() != "" && my_rank.thread == 0 ) {
        sim_output.output("# Creating simulation heartbeat at period of %s.\n", cfg->heartbeatPeriod().c_str());
        m_heartbeat =
            new SimulatorHeartbeat(cfg, my_rank.rank, this, timeLord.getTimeConverter(cfg->heartbeatPeriod()));
    }

    // Need to create the thread sync if there is more than one thread
    if ( num_ranks.thread > 1 ) {}
}

void
Simulation_impl::setStopAtCycle(Config* cfg)
{
    SimTime_t stopAt = timeLord.getSimCycles(cfg->stop_at(), "StopAction configure");
    if ( stopAt != 0 ) {
        StopAction* sa = new StopAction();
        sa->setDeliveryTime(stopAt);
        timeVortex->insert(sa);
    }
}

Component*
Simulation_impl::createComponent(ComponentId_t id, const std::string& name, Params& params)
{
    return factory->CreateComponent(id, name, params);
}

void
Simulation_impl::requireEvent(const std::string& name)
{
    factory->RequireEvent(name);
}

void
Simulation_impl::requireLibrary(const std::string& name)
{
    factory->requireLibrary(name);
}

SimTime_t
Simulation_impl::getNextActivityTime() const
{
    return timeVortex->front()->getDeliveryTime();
}

SimTime_t
Simulation_impl::getLocalMinimumNextActivityTime()
{
    SimTime_t ret = MAX_SIMTIME_T;
    for ( auto&& instance : instanceVec ) {
        SimTime_t next = instance->getNextActivityTime();
        if ( next < ret ) { ret = next; }
    }
    return ret;
}

void
Simulation_impl::processGraphInfo(ConfigGraph& graph, const RankInfo& UNUSED(myRank), SimTime_t min_part)
{
    // Set minPartTC (only thread 0 will do this)
    Simulation_impl::minPart = min_part;
    if ( my_rank.thread == 0 ) { minPartTC = minPartToTC(min_part); }

    // Get the minimum latencies for links between the various threads
    interThreadLatencies.resize(num_ranks.thread);
    for ( size_t i = 0; i < interThreadLatencies.size(); i++ ) {
        interThreadLatencies[i] = MAX_SIMTIME_T;
    }

    interThreadMinLatency  = MAX_SIMTIME_T;
    int cross_thread_links = 0;
    if ( num_ranks.thread > 1 ) {
        // Need to determine the lookahead for the thread synchronization
        ConfigComponentMap_t comps = graph.getComponentMap();
        ConfigLinkMap_t      links = graph.getLinkMap();
        // Find the minimum latency across a partition
        for ( auto iter = links.begin(); iter != links.end(); ++iter ) {
            ConfigLink* clink = *iter;
            RankInfo    rank[2];
            rank[0] = comps[COMPONENT_ID_MASK(clink->component[0])]->rank;
            rank[1] = comps[COMPONENT_ID_MASK(clink->component[1])]->rank;
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
            if ( clink->getMinLatency() < interThreadMinLatency ) { interThreadMinLatency = clink->getMinLatency(); }

            // Now check only those latencies that directly impact this
            // thread.  Keep track of minimum latency for each other
            // thread separately
            if ( rank[0].thread == my_rank.thread ) {
                if ( clink->getMinLatency() < interThreadLatencies[rank[1].thread] ) {
                    interThreadLatencies[rank[1].thread] = clink->getMinLatency();
                }
            }
            else if ( rank[1].thread == my_rank.thread ) {
                if ( clink->getMinLatency() < interThreadLatencies[rank[0].thread] ) {
                    interThreadLatencies[rank[0].thread] = clink->getMinLatency();
                }
            }
        }
    }
    // Create the SyncManager for this rank.  It gets created even if
    // we are single rank/single thread because it also manages the
    // Exit and Heartbeat actions.
    syncManager =
        new SyncManager(my_rank, num_ranks, minPartTC = minPartToTC(min_part), min_part, interThreadLatencies);

    // Check to see if the SyncManager profile tool is installed
    auto* tool = getProfileTool<Profile::SyncProfileTool>(SST_PROFILE_TOOL_SYNC);

    if ( tool != nullptr ) {
        // Add the receive profiler to the handler
        syncManager->addProfileTool(tool);
    }


    // Determine if this thread is independent.  That means there is
    // no need to synchronize with any other threads or ranks.
    if ( min_part == MAX_SIMTIME_T && cross_thread_links == 0 ) { independent = true; }
    else {
        independent = false;
    }
}

int
Simulation_impl::prepareLinks(ConfigGraph& graph, const RankInfo& myRank, SimTime_t UNUSED(min_part))
{
    // First, go through all the components that are in this rank and
    // create the ComponentInfo object for it
    // Now, build all the components
    for ( ConfigComponentMap_t::iterator iter = graph.comps.begin(); iter != graph.comps.end(); ++iter ) {
        ConfigComponent* ccomp = *iter;
        if ( ccomp->rank == myRank ) {
            compInfoMap.insert(new ComponentInfo(ccomp, ccomp->name, nullptr, new LinkMap()));
        }
    }

    // We will go through all the links and create LinkPairs for each
    // link.  We will also create a LinkMap for each component and put
    // them into a map with ComponentID as the key.
    for ( ConfigLinkMap_t::iterator iter = graph.links.begin(); iter != graph.links.end(); ++iter ) {
        ConfigLink* clink = *iter;
        RankInfo    rank[2];
        rank[0] = graph.comps[COMPONENT_ID_MASK(clink->component[0])]->rank;
        rank[1] = graph.comps[COMPONENT_ID_MASK(clink->component[1])]->rank;

        if ( rank[0] != myRank && rank[1] != myRank ) {
            // Nothing to be done
            continue;
        }
        // Same rank, same thread
        else if ( rank[0] == rank[1] ) {
            // Check to see if this is loopback link
            if ( clink->component[0] == clink->component[1] && clink->port[0] == clink->port[1] ) {
                // This is a loopback, so there is only one link
                Link* link      = new Link(clink->order);
                link->pair_link = link;
                link->setLatency(clink->latency[0]);

                // Add this link to the appropriate LinkMap
                ComponentInfo* cinfo = compInfoMap.getByID(clink->component[0]);
                if ( cinfo == nullptr ) {
                    // This shouldn't happen and is an error
                    sim_output.fatal(CALL_INFO, 1, "Couldn't find ComponentInfo in map.");
                }
                cinfo->getLinkMap()->insertLink(clink->port[0], link);
            }
            else {
                // Create a LinkPair to represent this link
                LinkPair lp(clink->order);

                lp.getLeft()->setLatency(clink->latency[0]);
                lp.getRight()->setLatency(clink->latency[1]);

                // Add this link to the appropriate LinkMap
                ComponentInfo* cinfo = compInfoMap.getByID(clink->component[0]);
                if ( cinfo == nullptr ) {
                    // This shouldn't happen and is an error
                    sim_output.fatal(CALL_INFO, 1, "Couldn't find ComponentInfo in map.");
                }
                cinfo->getLinkMap()->insertLink(clink->port[0], lp.getLeft());

                cinfo = compInfoMap.getByID(clink->component[1]);
                if ( cinfo == nullptr ) {
                    // This shouldn't happen and is an error
                    sim_output.fatal(CALL_INFO, 1, "Couldn't find ComponentInfo in map.");
                }
                cinfo->getLinkMap()->insertLink(clink->port[1], lp.getRight());
            }
        }
        // If we are on same rank, different threads and we are doing
        // direct_interthread links
        else if ( (rank[0].rank == rank[1].rank) && direct_interthread ) {
            int local;
            if ( rank[0] == myRank ) { local = 0; }
            else {
                local = 1;
            }

            Link* link = new Link(clink->order);
            link->setLatency(clink->latency[local]);
            if ( cross_thread_links.find(clink->id) != cross_thread_links.end() ) {
                // The other side already initialized.  Hook them
                // together as a pair.
                Link* other_link      = cross_thread_links[clink->id];
                link->pair_link       = other_link;
                other_link->pair_link = link;
                // Remove entry from map
                cross_thread_links.erase(clink->id);
            }
            else {
                // Nothing to do until the other side is created.
                // Just add myself to the map so the other side can
                // find me later.
                cross_thread_links[clink->id] = link;
            }

            ComponentInfo* cinfo = compInfoMap.getByID(clink->component[local]);
            if ( cinfo == nullptr ) { sim_output.fatal(CALL_INFO, 1, "Couldn't find ComponentInfo in map."); }
            cinfo->getLinkMap()->insertLink(clink->port[local], link);
        }
        // If the components are not in the same thread, then the
        // SyncManager will handle things
        else {
            int local, remote;
            if ( rank[0] == myRank ) {
                local  = 0;
                remote = 1;
            }
            else {
                local  = 1;
                remote = 0;
            }

            // Create a LinkPair to represent this link
            LinkPair lp(clink->order);

            lp.getLeft()->setLatency(clink->latency[local]);
            lp.getRight()->setLatency(0);
            lp.getRight()->setDefaultTimeBase(minPartToTC(1));

            // Add this link to the appropriate LinkMap for the local component
            ComponentInfo* cinfo = compInfoMap.getByID(clink->component[local]);
            if ( cinfo == nullptr ) {
                // This shouldn't happen and is an error
                sim_output.fatal(CALL_INFO, 1, "Couldn't find ComponentInfo in map.");
            }
            cinfo->getLinkMap()->insertLink(clink->port[local], lp.getLeft());

            // Need to register with both of the syncs (the ones for
            // both local and remote thread)

            // For local, just register link with threadSync object so
            // it can map link_id to link*
            ActivityQueue* sync_q = syncManager->registerLink(rank[remote], rank[local], clink->name, lp.getRight());

            lp.getLeft()->send_queue = sync_q;
            lp.getRight()->setAsSyncLink();
        }
    }
    return 0;
}


int
Simulation_impl::performWireUp(ConfigGraph& graph, const RankInfo& myRank, SimTime_t UNUSED(min_part))
{
    // Params objects should now start verifying parameters
    Params::enableVerify();


    // Now, build all the components
    for ( auto iter = graph.comps.begin(); iter != graph.comps.end(); ++iter ) {
        ConfigComponent* ccomp = *iter;

        if ( ccomp->rank == myRank ) {
            Component* tmp;

            // Check to make sure there are any entries in the component's LinkMap
            ComponentInfo* cinfo = compInfoMap.getByID(ccomp->id);
            if ( !cinfo->hasLinks() ) {
                printf("WARNING: Building component \"%s\" with no links assigned.\n", ccomp->name.c_str());
            }

            tmp = createComponent(ccomp->id, ccomp->type, ccomp->params);

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

void
Simulation_impl::exchangeLinkInfo()
{
    syncManager->exchangeLinkInfo();
}

void
Simulation_impl::initialize()
{
    init_phase_start_time = sst_get_cpu_time();
    bool done             = false;
    initBarrier.wait();
    if ( my_rank.thread == 0 ) { SharedObject::manager.updateState(false); }

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
        if ( my_rank.thread == 0 ) { SharedObject::manager.updateState(false); }
        untimed_phase++;
    } while ( !done );

    init_phase_total_time = sst_get_cpu_time() - init_phase_start_time;

    // Walk through all the links and call finalizeConfiguration

    for ( auto& i : compInfoMap ) {
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

void
Simulation_impl::complete()
{
    complete_phase_start_time = sst_get_cpu_time();
    completeBarrier.wait();
    untimed_phase = 0;
    // Walk through all the links and call prepareForComplete()
    for ( auto& i : compInfoMap ) {
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
    } while ( !done );
    complete_phase_total_time = sst_get_cpu_time() - complete_phase_start_time;
}

void
Simulation_impl::setup()
{

    setupBarrier.wait();

    /* Enforce finalization of SharedObjects */
    if ( my_rank.thread == 0 ) { SharedObject::manager.updateState(true); }

    setupBarrier.wait();

    for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter ) {
        (*iter)->getComponent()->setup();
    }

    setupBarrier.wait();
}

void
Simulation_impl::run()
{
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
    if ( my_rank.thread == 0 ) StatisticProcessingEngine::getInstance()->startOfSimulation();

    std::string header = std::to_string(my_rank.rank);
    header += ", ";
    header += std::to_string(my_rank.thread);
    header += ":  ";

#if SST_PERFORMANCE_INSTRUMENTING
    std::string filename = "rank_" + std::to_string(my_rank.rank);
    filename += "_thread_" + std::to_string(my_rank.thread);
    fp = fopen(filename.c_str(), "w");
#endif

#if SST_RUNTIME_PROFILING
#if SST_HIGH_RESOLUTION_CLOCK
    auto start = std::chrono::high_resolution_clock::now();
#else
    gettimeofday(&start, NULL);
#endif
#endif

    run_phase_start_time = sst_get_cpu_time();

    while ( LIKELY(!endSim) ) {
        current_activity = timeVortex->pop();
        currentSimCycle  = current_activity->getDeliveryTime();
        currentPriority  = current_activity->getPriority();
        current_activity->execute();

#if SST_PERIODIC_PRINT
        periodicCounter++;
#endif

        if ( UNLIKELY(0 != lastRecvdSignal) ) {
            switch ( lastRecvdSignal ) {
            case SIGUSR1:
                printStatus(false);
                break;
            case SIGUSR2:
                printStatus(true);
                break;
            case SIGALRM:
            case SIGINT:
            case SIGTERM:
                shutdown_mode = SHUTDOWN_SIGNAL;
                sim_output.output("EMERGENCY SHUTDOWN (%u,%u)!\n", my_rank.rank, my_rank.thread);
                sim_output.output(
                    "# Simulated time:                  %s\n", getElapsedSimTime().toStringBestSI().c_str());
                endSim = true;
                break;
            default:
                break;
            }
            lastRecvdSignal = 0;
        }

#if SST_PERIODIC_PRINT
        if ( periodicCounter >= SST_PERIODIC_PRINT_THRESHOLD ) {
#if SST_RUNTIME_PROFILING
#if SST_HIGH_RESOLUTION_CLOCK
            auto finish = std::chrono::high_resolution_clock::now();
            runtime     = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
#else
            gettimeofday(&end, NULL);
            timersub(&end, &start, &diff);
            runtime = diff.tv_usec + diff.tv_sec * 1e6;
#endif
#endif
            periodicCounter = 0;
            printPerformanceInfo();
        }
#endif
    }
    /* We shouldn't need to do this, but to be safe... */

    runBarrier.wait(); // TODO<- Is this needed?

    run_phase_total_time = sst_get_cpu_time() - run_phase_start_time;

    // If we have no links that are cut by a partition, we need to do
    // a final check to get the right simulated time.
    if ( minPart == MAX_SIMTIME_T && num_ranks.rank > 1 && my_rank.thread == 0 ) {
        endSimCycle = m_exit->computeEndTime();
    }

#if SST_RUNTIME_PROFILING
#if SST_HIGH_RESOLUTION_CLOCK
    auto finish = std::chrono::high_resolution_clock::now();
    runtime     = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
#else
    gettimeofday(&end, NULL);
    timersub(&end, &start, &diff);
    runtime = diff.tv_usec + diff.tv_sec * 1e6;
#endif
#endif

#if SST_PERFORMANCE_INSTRUMENTING
    printPerformanceInfo();
    fclose(fp);
#endif

    if ( num_ranks.rank != 1 && num_ranks.thread == 0 ) delete m_exit;
}

void
Simulation_impl::emergencyShutdown()
{
    std::lock_guard<std::mutex> lock(simulationMutex);

    for ( auto&& instance : instanceVec ) {
        instance->shutdown_mode = SHUTDOWN_EMERGENCY;
        instance->endSim        = true;
        //// Function not available with gcc 4.6
        // atomic_thread_fence(std::memory_order_acquire);
        for ( auto&& iter : instance->compInfoMap ) {
            if ( iter->getComponent() ) iter->getComponent()->emergencyShutdown();
        }
    }
}

// If this version is called, we need to set the end time in the exit
// object as well
void
Simulation_impl::endSimulation(void)
{
    m_exit->setEndTime(currentSimCycle);
    endSimulation(currentSimCycle);
}

void
Simulation_impl::endSimulation(SimTime_t end)
{
    // shutdown_mode = SHUTDOWN_CLEAN;

    // This is a collective operation across threads.  All threads
    // must enter and set flag before any will exit.
    // exitBarrier.wait();

    endSimCycle = end;
    endSim      = true;

    exitBarrier.wait();
}

void
Simulation_impl::finish()
{

    currentSimCycle = endSimCycle;

    for ( auto iter = compInfoMap.begin(); iter != compInfoMap.end(); ++iter ) {
        (*iter)->getComponent()->finish();
    }

    finishBarrier.wait();

    switch ( shutdown_mode ) {
    case SHUTDOWN_CLEAN:
        break;
    case SHUTDOWN_SIGNAL:
    case SHUTDOWN_EMERGENCY:
        for ( auto&& iter : compInfoMap ) {
            iter->getComponent()->emergencyShutdown();
        }
        sim_output.output("EMERGENCY SHUTDOWN Complete (%u,%u)!\n", my_rank.rank, my_rank.thread);
    }

    finishBarrier.wait();

    // Tell the Statistics Engine that the simulation is ending
    if ( my_rank.thread == 0 ) { StatisticProcessingEngine::getInstance()->endOfSimulation(); }
}

void
Simulation_impl::setSignal(int signal)
{
    for ( auto& instance : instanceVec )
        instance->lastRecvdSignal = signal;
}

void
Simulation_impl::printStatus(bool fullStatus)
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

double
Simulation_impl::getRunPhaseElapsedRealTime() const
{
    if ( run_phase_start_time == 0.0 ) return 0.0; // Not in run phase yet
    if ( run_phase_total_time == 0.0 ) { return sst_get_cpu_time() - run_phase_start_time; }
    else {
        return run_phase_total_time;
    }
}

double
Simulation_impl::getInitPhaseElapsedRealTime() const
{
    if ( init_phase_start_time == 0.0 ) return 0.0; // Not in init phase yet
    if ( init_phase_total_time == 0.0 ) { return sst_get_cpu_time() - init_phase_start_time; }
    else {
        return init_phase_total_time;
    }
}

double
Simulation_impl::getCompletePhaseElapsedRealTime() const
{
    if ( complete_phase_start_time == 0.0 ) return 0.0; // Not in complete phase yet
    if ( complete_phase_total_time == 0.0 ) { return sst_get_cpu_time() - complete_phase_start_time; }
    else {
        return complete_phase_total_time;
    }
}

TimeConverter*
Simulation_impl::registerClock(const std::string& freq, Clock::HandlerBase* handler, int priority)
{
    TimeConverter* tcFreq = timeLord.getTimeConverter(freq);
    return registerClock(tcFreq, handler, priority);
}

TimeConverter*
Simulation_impl::registerClock(const UnitAlgebra& freq, Clock::HandlerBase* handler, int priority)
{
    TimeConverter* tcFreq = timeLord.getTimeConverter(freq);
    return registerClock(tcFreq, handler, priority);
}

TimeConverter*
Simulation_impl::registerClock(TimeConverter* tcFreq, Clock::HandlerBase* handler, int priority)
{
    clockMap_t::key_type mapKey = std::make_pair(tcFreq->getFactor(), priority);
    if ( clockMap.find(mapKey) == clockMap.end() ) {
        Clock* ce        = new Clock(tcFreq, priority);
        clockMap[mapKey] = ce;

        ce->schedule();
    }
    clockMap[mapKey]->registerHandler(handler);
    return tcFreq;
}

Cycle_t
Simulation_impl::reregisterClock(TimeConverter* tc, Clock::HandlerBase* handler, int priority)
{
    clockMap_t::key_type mapKey = std::make_pair(tc->getFactor(), priority);
    if ( clockMap.find(mapKey) == clockMap.end() ) {
        Output out("Simulation: @R:@t:", 0, 0, Output::STDERR);
        out.fatal(CALL_INFO, 1, "Tried to reregister with a clock that was not previously registered, exiting...\n");
    }
    clockMap[mapKey]->registerHandler(handler);
    return clockMap[mapKey]->getNextCycle();
}

Cycle_t
Simulation_impl::getNextClockCycle(TimeConverter* tc, int priority)
{
    clockMap_t::key_type mapKey = std::make_pair(tc->getFactor(), priority);
    if ( clockMap.find(mapKey) == clockMap.end() ) {
        Output out("Simulation: @R:@t:", 0, 0, Output::STDERR);
        out.fatal(
            CALL_INFO, 1, "Call to getNextClockCycle() on a clock that was not previously registered, exiting...\n");
    }
    return clockMap[mapKey]->getNextCycle();
}

void
Simulation_impl::unregisterClock(TimeConverter* tc, Clock::HandlerBase* handler, int priority)
{
    clockMap_t::key_type mapKey = std::make_pair(tc->getFactor(), priority);
    if ( clockMap.find(mapKey) != clockMap.end() ) {
        bool empty;
        clockMap[mapKey]->unregisterHandler(handler, empty);
    }
}

TimeConverter*
Simulation_impl::registerOneShot(const std::string& timeDelay, OneShot::HandlerBase* handler, int priority)
{
    return registerOneShot(UnitAlgebra(timeDelay), handler, priority);
}

TimeConverter*
Simulation_impl::registerOneShot(const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler, int priority)
{
    TimeConverter*       tcTimeDelay = timeLord.getTimeConverter(timeDelay);
    clockMap_t::key_type mapKey      = std::make_pair(tcTimeDelay->getFactor(), priority);

    // Search the oneShot map for a oneShot with the associated timeDelay factor
    if ( oneShotMap.find(mapKey) == oneShotMap.end() ) {
        // OneShot with the specific timeDelay not found,
        // create a new one and add it to the map of OneShots
        OneShot* ose       = new OneShot(tcTimeDelay, priority);
        oneShotMap[mapKey] = ose;
    }

    // Add the handler to the OneShots list of handlers, Also the
    // registerHandler will schedule the oneShot to fire in the future
    oneShotMap[mapKey]->registerHandler(handler);
    return tcTimeDelay;
}

void
Simulation_impl::insertActivity(SimTime_t time, Activity* ev)
{
    ev->setDeliveryTime(time);
    timeVortex->insert(ev);
}

uint64_t
Simulation_impl::getTimeVortexMaxDepth() const
{
    return timeVortex->getMaxDepth();
}

uint64_t
Simulation_impl::getTimeVortexCurrentDepth() const
{
    return timeVortex->getCurrentDepth();
}

uint64_t
Simulation_impl::getSyncQueueDataSize() const
{
    return syncManager->getDataSize();
}

Statistics::StatisticProcessingEngine*
Simulation_impl::getStatisticsProcessingEngine(void) const
{
    return Statistics::StatisticProcessingEngine::getInstance();
}

#if SST_EVENT_PROFILING
void
Simulation_impl::incrementSerialCounters(uint64_t count)
{
    rankLatency += count;
    ++rankExchangeCounter;
}

void
Simulation_impl::incrementExchangeCounters(uint64_t events, uint64_t bytes)
{
    rankExchangeEvents += events;
    rankExchangeBytes += bytes;
}
#endif // SST_EVENT_PROFILING


#if SST_SYNC_PROFILING
void
Simulation_impl::incrementSyncTime(bool rankSync, uint64_t count)
{
    if ( rankSync ) {
        ++rankSyncCounter;
        rankSyncTime += count;
    }
    else {
        ++threadSyncCounter;
        threadSyncTime += count;
    }
}
#endif // SST_SYNC_PROFILING


// Function to allow for easy serialization of threads while debugging
// code
void
wait_my_turn_start(Core::ThreadSafe::Barrier& barrier, int thread, int UNUSED(total_threads))
{
    // Everyone barriers
    barrier.wait();
    // Now barrier until it's my turn
    for ( int i = 0; i < thread; i++ ) {
        barrier.wait();
    }
}

void
wait_my_turn_end(Core::ThreadSafe::Barrier& barrier, int thread, int total_threads)
{

    // Wait for all the threads after me to finish
    for ( int i = thread; i < total_threads; i++ ) {
        barrier.wait();
    }
    // All barrier
    barrier.wait();
}

void
Simulation_impl::resizeBarriers(uint32_t nthr)
{
    initBarrier.resize(nthr);
    completeBarrier.resize(nthr);
    setupBarrier.resize(nthr);
    runBarrier.resize(nthr);
    exitBarrier.resize(nthr);
    finishBarrier.resize(nthr);
}


void
Simulation_impl::intializeDefaultProfileTools(const std::string& config)
{
    // Need to parse the profile string.  Format is:
    // point=type(key=value,key=value); point=type(...)

    // type is optional.  If not specified, a default will be used.  Params are optional.

    // First split on semicolon
    std::vector<std::string> tokens;
    SST::tokenize(tokens, config, ";", true);

    for ( auto& x : tokens ) {
        // Need to get the profile point name, type and params
        std::string point;
        std::string type;
        std::string param_str;

        auto start = 0;
        auto end   = x.find(":");
        point      = x.substr(start, end - start);
        trim(point);
        if ( end != std::string::npos ) {
            // type was specifed, get it
            start = end + 1;
            end   = x.find("(", start);
            type  = x.substr(start, end - start);
            trim(type);
            if ( end != std::string::npos ) {
                // Get the params
                start = end + 1;
                end   = x.find(")", start);
                if ( end == std::string::npos ) {
                    sim_output.fatal(
                        CALL_INFO_LONG, 1,
                        "ERROR: Invalid format for argument passed to --enable-profiling.  Argument should be a "
                        "semi-colon "
                        "separated list where each item specified details for a given profiling point using the "
                        "following format: point=type(key=value,key=value,...).  Params are optional and can only be "
                        "specified if a type is supplied.  Type is also optional and a default type will be used if "
                        "not specified.\n");
                }
                param_str = x.substr(start, end - start);
                trim(param_str);
            }
        }

        Params params;
        if ( param_str.length() > 0 ) {
            std::vector<std::string> pairs;
            SST::tokenize(pairs, param_str, ",", true);
            for ( auto& y : pairs ) {
                std::vector<std::string> kv;
                SST::tokenize(kv, y, "=", true);
                if ( kv.size() < 2 ) {
                    sim_output.fatal(
                        CALL_INFO_LONG, 1, "ERROR: Invalid format for params (%s), format should be key=value\n",
                        y.c_str());
                }
                params.insert(kv[0], kv[1]);
            }
        }

        // Initialize the point
        if ( point == "" ) {
            // Do nothing
        }
        else if ( point == "event" ) {
            if ( type == "" ) type = "sst.profile.handler.event.time.high_resolution";
            auto* tool = Factory::getFactory()->CreateProfileTool<SST::Profile::EventHandlerProfileTool>(
                type, SST_PROFILE_TOOL_EVENT, "Default Event Handler Profile Tool", params);
            profile_tools[SST_PROFILE_TOOL_EVENT] = tool;
        }
        else if ( point == "clock" ) {
            if ( type == "" ) type = "sst.profile.handler.clock.time.high_resolution";
            auto* tool = Factory::getFactory()->CreateProfileTool<SST::Profile::ClockHandlerProfileTool>(
                type, SST_PROFILE_TOOL_CLOCK, "Default Clock Handler Profile Tool", params);
            profile_tools[SST_PROFILE_TOOL_CLOCK] = tool;
        }
        else if ( point == "sync" ) {
            if ( type == "" ) type = "sst.profile.sync.time.high_resolution";
            auto* tool = Factory::getFactory()->CreateProfileTool<SST::Profile::SyncProfileTool>(
                type, SST_PROFILE_TOOL_SYNC, "Default SYNC Profile Tool", params);
            profile_tools[SST_PROFILE_TOOL_SYNC] = tool;
        }
        else {
            // FATAL
            sim_output.fatal(
                CALL_INFO_LONG, 1, "ERROR: Unknown profiling point specified with --enable-profiling: %s\n",
                point.c_str());
        }
    }
}

void
Simulation_impl::printProfilingInfo(FILE* fp)
{
    // If no profile tools are installed, return without doing
    // anything
    if ( profile_tools.size() == 0 ) return;

    // Print out a header if printing to stdout
    if ( fp == stdout && my_rank.rank == 0 && my_rank.thread == 0 ) {
        fprintf(fp, "\n------------------------------------------------------------\n");
        fprintf(fp, "Profiling Output:\n");
    }

    // Print the rank and thread.  Profiling output is serialized
    // through both ranks and threads.
    fprintf(fp, "Rank = %" PRIu32 ", thread = %" PRIu32 ":\n", my_rank.rank, my_rank.thread);

    for ( auto tool : profile_tools ) {
        fprintf(fp, "\n");
        tool.second->outputData(fp);
    }

    // Print footer if printing on stdout
    if ( fp == stdout && my_rank.rank == num_ranks.rank - 1 && my_rank.thread == num_ranks.thread - 1 ) {
        fprintf(fp, "------------------------------------------------------------\n");
    }
}

#if SST_PERFORMANCE_INSTRUMENTING
void
Simulation_impl::printPerformanceInfo()
{
#if SST_RUNTIME_PROFILING
    fprintf(fp, "///Print at %.6fs\n", (double)runtime / clockDivisor);
#endif

    // Iterate through components and find all handlers mapped to that component
    // If handler mapping is not populated, prints out raw clock handler times

#if SST_EVENT_PROFILING
    // Rank only information
    fprintf(fp, "Serialization Information:\n");
    fprintf(fp, "Rank total serialization time: %" PRIu64 " %s\n", rankLatency, clockResolution.c_str());
    fprintf(fp, "Rank pairwise sync count: %" PRIu64 "\n", rankExchangeCounter);
    fprintf(fp, "Rank total events sent: %" PRIu64 "\n", rankExchangeEvents);
    fprintf(fp, "Rank total bytes sent: %" PRIu64 "\n", rankExchangeBytes);
    fprintf(
        fp, "Rank average sync serialization time: %.6f %s/sync\n",
        (rankExchangeCounter == 0 ? 0.0 : (double)rankLatency / rankExchangeCounter), clockResolution.c_str());
    fprintf(
        fp, "Rank average sync bytes sent: %.6f bytes/sync\n",
        (rankExchangeCounter == 0 ? 0.0 : (double)rankExchangeBytes / rankExchangeCounter));
    fprintf(
        fp, "Rank average sync serialization time: %.6f %s/sync\n",
        (rankExchangeCounter == 0 ? 0.0 : (double)rankLatency / rankExchangeCounter), clockResolution.c_str());
    fprintf(
        fp, "Rank average event bytes sent: %.6f bytes/event\n",
        (rankExchangeEvents == 0 ? 0.0 : (double)rankExchangeBytes / rankExchangeEvents));
#endif // SST_EVENT_PROFILING

#if SST_SYNC_PROFILING
    fprintf(fp, "Synchronization Information:\n");
    fprintf(fp, "Thread-only sync (apart from Rank syncs):\n");
    fprintf(fp, "Thread-sync count: %" PRIu64 "\n", threadSyncCounter);
    fprintf(fp, "Thread-sync total execution time: %.6f s\n", (double)threadSyncTime / clockDivisor);
    fprintf(
        fp, "Thread-sync average execution time: %.6f %s/sync\n",
        (threadSyncCounter == 0.0 ? 0.0 : (double)threadSyncTime / threadSyncCounter), clockResolution.c_str());
    fprintf(fp, "Rank Sync (including associated thread syncs):\n");
    fprintf(fp, "Rank sync count: %" PRIu64 "\n", rankSyncCounter);
    fprintf(fp, "Rank sync total execution time: %.6f s\n", (double)rankSyncTime / clockDivisor);
    fprintf(
        fp, "Rank sync average execution time: %.6f %s/sync\n",
        (rankSyncCounter == 0.0 ? 0.0 : (double)rankSyncTime / rankSyncCounter), clockResolution.c_str());
    fprintf(fp, "All sync count:  %" PRIu64 "\n", threadSyncCounter + rankSyncCounter);
    fprintf(fp, "All sync execution time: %.6f s\n", (double)(threadSyncTime + rankSyncTime) / clockDivisor);
    fprintf(
        fp, "All sync average execution time: %.6f %s/sync\n",
        ((threadSyncCounter + rankSyncCounter) == 0
             ? 0.0
             : (double)(threadSyncTime + rankSyncTime) / (threadSyncCounter + rankSyncCounter)),
        clockResolution.c_str());
    fprintf(fp, "\n");
#endif // SST_SYNC_PROFILING
}
#endif

/* Define statics */
Factory*                  Simulation_impl::factory;
TimeLord                  Simulation_impl::timeLord;
std::map<LinkId_t, Link*> Simulation_impl::cross_thread_links;
Output                    Simulation_impl::sim_output;
Core::ThreadSafe::Barrier Simulation_impl::initBarrier;
Core::ThreadSafe::Barrier Simulation_impl::completeBarrier;
Core::ThreadSafe::Barrier Simulation_impl::setupBarrier;
Core::ThreadSafe::Barrier Simulation_impl::runBarrier;
Core::ThreadSafe::Barrier Simulation_impl::exitBarrier;
Core::ThreadSafe::Barrier Simulation_impl::finishBarrier;
std::mutex                Simulation_impl::simulationMutex;
TimeConverter*            Simulation_impl::minPartTC = nullptr;
SimTime_t                 Simulation_impl::minPart;

/* Define statics (Simulation) */
std::unordered_map<std::thread::id, Simulation_impl*> Simulation_impl::instanceMap;
std::vector<Simulation_impl*>                         Simulation_impl::instanceVec;
std::atomic<int>                                      Simulation_impl::untimed_msg_count;
Exit*                                                 Simulation_impl::m_exit;

} // namespace SST
