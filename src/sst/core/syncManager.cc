// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/syncManager.h"

#include "sst/core/exit.h"
#include "sst/core/simulation.h"
#include "sst/core/syncBase.h"
#include "sst/core/threadSyncQueue.h"
#include "sst/core/timeConverter.h"

#include "sst/core/rankSyncSerialSkip.h"
#include "sst/core/rankSyncParallelSkip.h"
#include "sst/core/rankSyncParallelSkip.h"
#include "sst/core/threadSyncSimpleSkip.h"

namespace SST {

// Static data members
NewRankSync* SyncManager::rankSync = NULL;
Core::ThreadSafe::Barrier SyncManager::RankExecBarrier[6];
Core::ThreadSafe::Barrier SyncManager::LinkInitBarrier[3];
SimTime_t SyncManager::next_rankSync = MAX_SIMTIME_T;

class EmptyRankSync : public NewRankSync {
public:
    EmptyRankSync() {
        nextSyncTime = MAX_SIMTIME_T;
    }
    ~EmptyRankSync() {}

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(const RankInfo& to_rank __attribute__((unused)), const RankInfo& from_rank __attribute__((unused)), LinkId_t link_id __attribute__((unused)), Link* link __attribute__((unused))) { return NULL; }

    void execute(int thread __attribute__((unused))) {}
    void exchangeLinkInitData(int thread __attribute__((unused)), std::atomic<int>& msg_count __attribute__((unused))) {}
    void finalizeLinkConfigurations() {}

    SimTime_t getNextSyncTime() { return nextSyncTime; }

    TimeConverter* getMaxPeriod() {return max_period;}

    uint64_t getDataSize() const { return 0; }    
};

class EmptyThreadSync : public NewThreadSync {
public:
    EmptyThreadSync () {
        nextSyncTime = MAX_SIMTIME_T;
    }
    ~EmptyThreadSync() {}

    void before() {}
    void after() {}
    void execute() {}
    void processLinkInitData() {}
    void finalizeLinkConfigurations() {}

    /** Register a Link which this Sync Object is responsible for */
    void registerLink(LinkId_t link_id __attribute__((unused)), Link* link __attribute__((unused))) {}
    ActivityQueue* getQueueForThread(int tid __attribute__((unused))) { return NULL; }
};


SyncManager::SyncManager(const RankInfo& rank, const RankInfo& num_ranks, TimeConverter* minPartTC, SimTime_t min_part, const std::vector<SimTime_t>& interThreadLatencies __attribute__((unused))) :
    Action(),
    rank(rank),
    num_ranks(num_ranks),
    threadSync(NULL),
    min_part(min_part)
{
    // TraceFunction trace(CALL_INFO_LONG);    

    sim = Simulation::getSimulation();


    if ( rank.thread == 0  ) {
        // if ( num_ranks.rank > 1 ) {
        for ( auto &b : RankExecBarrier ) { b.resize(num_ranks.thread); }
        for ( auto &b : LinkInitBarrier ) { b.resize(num_ranks.thread); }
        if ( min_part != MAX_SIMTIME_T ) {
            if ( num_ranks.thread == 1 ) {
                rankSync = new RankSyncSerialSkip(/*num_ranks,*/ minPartTC);
            }
            else {
                rankSync = new RankSyncParallelSkip(num_ranks, minPartTC);
            }
        }
        else {
            rankSync = new EmptyRankSync();
        }
    }

    // Need to check to see if there are any inter-thread
    // dependencies.  If not, EmptyThreadSync, otherwise use one
    // of the active threadsyncs.
    SimTime_t interthread_minlat = sim->getInterThreadMinLatency();
    if ( num_ranks.thread > 1 && interthread_minlat != MAX_SIMTIME_T ) {
        threadSync = new ThreadSyncSimpleSkip(num_ranks.thread, rank.thread, Simulation::getSimulation());
    }
    else {
        threadSync = new EmptyThreadSync();
    }

    exit = sim->getExit();

    setPriority(SYNCPRIORITY);
}


SyncManager::~SyncManager() {}

/** Register a Link which this Sync Object is responsible for */
ActivityQueue*
SyncManager::registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link)
{
    // TraceFunction trace(CALL_INFO_LONG);    
    if ( to_rank == from_rank ) {
        // trace.getOutput().output(CALL_INFO, "The impossible happened\n");
        return NULL;  // This should never happen
    }

    if ( to_rank.rank == from_rank.rank ) {
        // TraceFunction trace2(CALL_INFO);
        // Same rank, different thread.  Need to send the right data
        // to the two ThreadSync objects for the threads on either
        // side of the link

        // For the local ThreadSync, just need to register the link
        threadSync->registerLink(link_id, link);

        // Need to get target queue from the remote ThreadSync
        NewThreadSync* remoteSync = Simulation::instanceVec[to_rank.thread]->syncManager->threadSync;
        // trace.getOutput().output(CALL_INFO,"queue = %p\n",remoteSync->getQueueForThread(from_rank.thread));
        return remoteSync->getQueueForThread(from_rank.thread);
    }
    else {
        // Different rank.  Send info onto the RankSync
        return rankSync->registerLink(to_rank, from_rank, link_id, link);
    }
}

void
SyncManager::execute(void)
{
    // TraceFunction trace(CALL_INFO_LONG);    
    // trace.getOutput().output(CALL_INFO, "next_sync_type @ switch = %d\n", next_sync_type);
    switch ( next_sync_type ) {
    case RANK:
        // Need to make sure all threads have reached the sync to
        // guarantee that all events have been sent to the appropriate
        // queues.
        RankExecBarrier[0].wait();
        
        // For a rank sync, we will force a thread sync first.  This
        // will ensure that all events sent between threads will be
        // flushed into their repective TimeVortices.  We need to do
        // this to enable any skip ahead optimizations.
        threadSync->before();
        // trace.getOutput().output(CALL_INFO, "Complete threadSync->before()\n");

        // Need to make sure everyone has made it through the mutex
        // and the min time computation is complete
        RankExecBarrier[1].wait();
        
        // Now call the actual RankSync
        // trace.getOutput().output(CALL_INFO, "About to enter rankSync->execute()\n");
        rankSync->execute(rank.thread);
        // trace.getOutput().output(CALL_INFO, "Complete rankSync->execute()\n");

        RankExecBarrier[2].wait();

        // Now call the threadSync after() call
        threadSync->after();
        // trace.getOutput().output(CALL_INFO, "Complete threadSync->after()\n");

        RankExecBarrier[3].wait();
        
        if ( exit != NULL && rank.thread == 0 ) exit->check();

        RankExecBarrier[4].wait();

        if ( exit->getGlobalCount() == 0 ) {
            endSimulation(exit->getEndTime());
        }

        break;
    case THREAD:

        threadSync->execute();
        
        if ( /*num_ranks.rank == 1*/ min_part == MAX_SIMTIME_T ) {
            if ( exit->getRefCount() == 0 ) {
                endSimulation(exit->getEndTime());
            }
        }
        // if ( exit != NULL ) exit->check();
        
        break;
    default:
        break;
    }
    computeNextInsert();
    // trace.getOutput().output(CALL_INFO, "next_sync_type = %d\n", next_sync_type);
    RankExecBarrier[5].wait();
}

/** Cause an exchange of Initialization Data to occur */
void
SyncManager::exchangeLinkInitData(std::atomic<int>& msg_count)
{
    // TraceFunction trace(CALL_INFO_LONG);    
    LinkInitBarrier[0].wait();
    threadSync->processLinkInitData();
    LinkInitBarrier[1].wait();
    rankSync->exchangeLinkInitData(rank.thread, msg_count);
    LinkInitBarrier[2].wait();
}

/** Finish link configuration */
void
SyncManager::finalizeLinkConfigurations()
{
    // TraceFunction trace(CALL_INFO_LONG);    
    threadSync->finalizeLinkConfigurations();
    // Only thread 0 should call finalize on rankSync
    if ( rank.thread == 0 ) rankSync->finalizeLinkConfigurations();

    // Need to figure out what sync comes first and insert object into
    // TimeVortex
    computeNextInsert();
}

void
SyncManager::computeNextInsert()
{
    // TraceFunction trace(CALL_INFO_LONG);    
    // trace.getOutput().output(CALL_INFO,"%" PRIu64 ", %" PRIu64 ", %" PRIu64 "\n",
    //                          sim->getCurrentSimCycle(), rankSync->getNextSyncTime(), threadSync->getNextSyncTime());
    if ( rankSync->getNextSyncTime() <= threadSync->getNextSyncTime() ) {
        next_sync_type = RANK;
        sim->insertActivity(rankSync->getNextSyncTime(), this);
        // sim->getSimulationOutput().output(CALL_INFO,"Next insert at: %" PRIu64 " (rank)\n",rankSync->getNextSyncTime());
    }
    else {
        next_sync_type = THREAD;
        sim->insertActivity(threadSync->getNextSyncTime(), this);
        // sim->getSimulationOutput().output(CALL_INFO,"Next insert at: %" PRIu64 " (thread)\n",threadSync->getNextSyncTime());
    }
}

void
SyncManager::print(const std::string& header, Output &out) const
{
    out.output("%s SyncManager to be delivered at %" PRIu64
               " with priority %d\n",
               header.c_str(), getDeliveryTime(), getPriority());
}
} // namespace SST


