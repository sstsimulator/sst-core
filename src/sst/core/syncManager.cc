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

#include <sst/core/warnmacros.h>
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

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#define UNUSED_WO_MPI(x) x
#else
#define UNUSED_WO_MPI(x) UNUSED(x)
#endif

namespace SST {

// Static data members
NewRankSync* SyncManager::rankSync = NULL;
Core::ThreadSafe::Barrier SyncManager::RankExecBarrier[6];
Core::ThreadSafe::Barrier SyncManager::LinkUntimedBarrier[3];
SimTime_t SyncManager::next_rankSync = MAX_SIMTIME_T;

class EmptyRankSync : public NewRankSync {
public:
    EmptyRankSync() {
        nextSyncTime = MAX_SIMTIME_T;
    }
    ~EmptyRankSync() {}

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(const RankInfo& UNUSED(to_rank), const RankInfo& UNUSED(from_rank), LinkId_t UNUSED(link_id), Link* UNUSED(link)) override { return NULL; }

    void execute(int UNUSED(thread)) override {}
    void exchangeLinkUntimedData(int UNUSED_WO_MPI(thread), std::atomic<int>& UNUSED_WO_MPI(msg_count)) override {
        // Even though there are no links crossing ranks, we still
        // need to make sure every rank does the same number of init
        // cycles so the shared memory regions initialization works.
        
#ifdef SST_CONFIG_HAVE_MPI
        if ( thread != 0 ) {
            return;
        }
        
        // Do an allreduce to see if there were any messages sent
        int input = msg_count;

        int count;
        MPI_Allreduce( &input, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
        msg_count = count;
#endif
    }
    void finalizeLinkConfigurations() override {}

    void prepareForComplete() override {}

    SimTime_t getNextSyncTime() override { return nextSyncTime; }

    TimeConverter* getMaxPeriod() {return max_period;}

    uint64_t getDataSize() const override { return 0; }
};

class EmptyThreadSync : public NewThreadSync {
public:
    EmptyThreadSync () {
        nextSyncTime = MAX_SIMTIME_T;
    }
    ~EmptyThreadSync() {}

    void before() override {}
    void after() override {}
    void execute() override {}
    void processLinkUntimedData() override {}
    void finalizeLinkConfigurations() override {}
    void prepareForComplete() override {}

    /** Register a Link which this Sync Object is responsible for */
    void registerLink(LinkId_t UNUSED(link_id), Link* UNUSED(link)) override {}
    ActivityQueue* getQueueForThread(int UNUSED(tid)) override { return NULL; }
};


SyncManager::SyncManager(const RankInfo& rank, const RankInfo& num_ranks, TimeConverter* minPartTC, SimTime_t min_part, const std::vector<SimTime_t>& UNUSED(interThreadLatencies)) :
    Action(),
    rank(rank),
    num_ranks(num_ranks),
    threadSync(NULL),
    min_part(min_part)
{
    sim = Simulation::getSimulation();


    if ( rank.thread == 0  ) {
        for ( auto &b : RankExecBarrier ) { b.resize(num_ranks.thread); }
        for ( auto &b : LinkUntimedBarrier ) { b.resize(num_ranks.thread); }
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
    if ( to_rank == from_rank ) {
        return NULL;  // This should never happen
    }

    if ( to_rank.rank == from_rank.rank ) {
        // Same rank, different thread.  Need to send the right data
        // to the two ThreadSync objects for the threads on either
        // side of the link

        // For the local ThreadSync, just need to register the link
        threadSync->registerLink(link_id, link);

        // Need to get target queue from the remote ThreadSync
        NewThreadSync* remoteSync = Simulation::instanceVec[to_rank.thread]->syncManager->threadSync;
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
    switch ( next_sync_type ) {
    case RANK:
        // Need to make sure all threads have reached the sync to
        // guarantee that all events have been sent to the appropriate
        // queues.
        RankExecBarrier[0].wait();
        
        // For a rank sync, we will force a thread sync first.  This
        // will ensure that all events sent between threads will be
        // flushed into their respective TimeVortices.  We need to do
        // this to enable any skip ahead optimizations.
        threadSync->before();
        // trace.getOutput().output(CALL_INFO, "Complete threadSync->before()\n");

        // Need to make sure everyone has made it through the mutex
        // and the min time computation is complete
        RankExecBarrier[1].wait();
        
        // Now call the actual RankSync
        rankSync->execute(rank.thread);

        RankExecBarrier[2].wait();

        // Now call the threadSync after() call
        threadSync->after();

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
        
        break;
    default:
        break;
    }
    computeNextInsert();
    RankExecBarrier[5].wait();
}

/** Cause an exchange of Untimed Data to occur */
void
SyncManager::exchangeLinkUntimedData(std::atomic<int>& msg_count)
{
    // TraceFunction trace(CALL_INFO_LONG);    
    LinkUntimedBarrier[0].wait();
    threadSync->processLinkUntimedData();
    LinkUntimedBarrier[1].wait();
    rankSync->exchangeLinkUntimedData(rank.thread, msg_count);
    LinkUntimedBarrier[2].wait();
}

/** Finish link configuration */
void
SyncManager::finalizeLinkConfigurations()
{
    threadSync->finalizeLinkConfigurations();
    // Only thread 0 should call finalize on rankSync
    if ( rank.thread == 0 ) rankSync->finalizeLinkConfigurations();

    // Need to figure out what sync comes first and insert object into
    // TimeVortex
    computeNextInsert();
}

/** Prepare for complete() phase */
void
SyncManager::prepareForComplete()
{
    threadSync->prepareForComplete();
    // Only thread 0 should call finalize on rankSync
    if ( rank.thread == 0 ) rankSync->prepareForComplete();
}

void
SyncManager::computeNextInsert()
{
    if ( rankSync->getNextSyncTime() <= threadSync->getNextSyncTime() ) {
        next_sync_type = RANK;
        sim->insertActivity(rankSync->getNextSyncTime(), this);
    }
    else {
        next_sync_type = THREAD;
        sim->insertActivity(threadSync->getNextSyncTime(), this);
    }
}

void
SyncManager::print(const std::string& header, Output &out) const
{
    out.output("%s SyncManager to be delivered at %" PRIu64
               " with priority %d\n",
               header.c_str(), getDeliveryTime(), getPriority());
}

uint64_t SyncManager::getDataSize() const
{
    return rankSync->getDataSize();
}


} // namespace SST


