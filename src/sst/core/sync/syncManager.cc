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

#include "sst/core/sync/syncManager.h"

#include "sst/core/exit.h"
#include "sst/core/objectComms.h"
#include "sst/core/profile/syncProfileTool.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sync/rankSyncParallelSkip.h"
#include "sst/core/sync/rankSyncSerialSkip.h"
#include "sst/core/sync/threadSyncDirectSkip.h"
#include "sst/core/sync/threadSyncQueue.h"
#include "sst/core/sync/threadSyncSimpleSkip.h"
#include "sst/core/timeConverter.h"
#include "sst/core/warnmacros.h"

#include <sys/time.h>

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
RankSync*                 SyncManager::rankSync = nullptr;
Core::ThreadSafe::Barrier SyncManager::RankExecBarrier[6];
Core::ThreadSafe::Barrier SyncManager::LinkUntimedBarrier[3];
SimTime_t                 SyncManager::next_rankSync = MAX_SIMTIME_T;

#if SST_SYNC_PROFILING

#if SST_HIGH_RESOLUTION_CLOCK
#define SST_SYNC_PROFILE_START                                  \
    auto sim                = Simulation_impl::getSimulation(); \
    auto last_sync_type     = next_sync_type;                   \
    auto sync_profile_start = std::chrono::high_resolution_clock::now();

#define SST_SYNC_PROFILE_STOP                                                                                 \
    auto sync_profile_stop = std::chrono::high_resolution_clock::now();                                       \
    auto sync_profile_count =                                                                                 \
        std::chrono::duration_cast<std::chrono::nanoseconds>(sync_profile_stop - sync_profile_start).count(); \
    sim->incrementSyncTime(last_sync_type == RANK, sync_profile_count);

#else
#define SST_SYNC_PROFILE_START                                               \
    auto           sim            = Simulation_impl::getSimulation();        \
    auto           last_sync_type = next_sync_type;                          \
    struct timeval sync_profile_stop, sync_profile_start, sync_profile_diff; \
    gettimeofday(&sync_profile_start, NULL);

#define SST_SYNC_PROFILE_STOP                                                               \
    gettimeofday(&sync_profile_stop, NULL);                                                 \
    timersub(&sync_profile_stop, &sync_profile_start, &sync_profile_diff);                  \
    auto sync_profile_count = (sync_profile_diff.tv_usec + sync_profile_diff.tv_sec * 1e6); \
    sim->incrementSyncTime(last_sync_type == RANK, sync_profile_count);

#endif // SST_HIGH_RESOLUTION_CLOCK

#else // SST_SYNC_PROFILING
#define SST_SYNC_PROFILE_START
#define SST_SYNC_PROFILE_STOP
#endif // SST_SYNC_PROFILING

class EmptyRankSync : public RankSync
{
public:
    EmptyRankSync(const RankInfo& num_ranks) : RankSync(num_ranks) { nextSyncTime = MAX_SIMTIME_T; }
    ~EmptyRankSync() {}

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(
        const RankInfo& UNUSED(to_rank), const RankInfo& UNUSED(from_rank), const std::string& UNUSED(name),
        Link* UNUSED(link)) override
    {
        return nullptr;
    }

    void execute(int UNUSED(thread)) override {}
    void exchangeLinkUntimedData(int UNUSED_WO_MPI(thread), std::atomic<int>& UNUSED_WO_MPI(msg_count)) override
    {
        // Even though there are no links crossing ranks, we still
        // need to make sure every rank does the same number of init
        // cycles so the shared memory regions initialization works.

#ifdef SST_CONFIG_HAVE_MPI
        if ( thread != 0 ) { return; }

        // Do an allreduce to see if there were any messages sent
        int input = msg_count;

        int count;
        MPI_Allreduce(&input, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        msg_count = count;
#endif
    }
    void finalizeLinkConfigurations() override {}

    void prepareForComplete() override {}

    SimTime_t getNextSyncTime() override { return nextSyncTime; }

    TimeConverter* getMaxPeriod() { return max_period; }

    uint64_t getDataSize() const override { return 0; }
};

class EmptyThreadSync : public ThreadSync
{
public:
    Simulation_impl* sim;

public:
    EmptyThreadSync(Simulation_impl* sim) : sim(sim) { nextSyncTime = MAX_SIMTIME_T; }
    ~EmptyThreadSync() {}

    void before() override {}
    void after() override {}
    void execute() override {}
    void processLinkUntimedData() override {}
    void finalizeLinkConfigurations() override {}
    void prepareForComplete() override {}

    /** Register a Link which this Sync Object is responsible for */
    void           registerLink(const std::string& UNUSED(name), Link* UNUSED(link)) override {}
    ActivityQueue* registerRemoteLink(int UNUSED(tid), const std::string& UNUSED(name), Link* UNUSED(link)) override
    {
        return nullptr;
    }
};

void
RankSync::exchangeLinkInfo(uint32_t UNUSED_WO_MPI(my_rank))
{
    // Function will not compile if MPI is not configured
#ifdef SST_CONFIG_HAVE_MPI
    // For now, we will simply have rank 0 exchange with everyone and
    // then rank 1, etc.  Will look to optimize later (more
    // parallelism in ranks sending)

    // Need to exchange with each partner.  For those with lower
    // ranks, I will recv first, then send.  For those with higher
    // ranks, I will send first, then recv.
    for ( uint32_t i = 0; i < my_rank; ++i ) {
        // I'm the high rank, so recv is first
        // std::map<std::string, uintptr_t> data;
        std::vector<std::pair<std::string, uintptr_t>> data;

        Comms::recv(i, 0, data);
        Comms::send(i, 0, link_maps[i]);

        // Process the data
        for ( auto x : data ) {
            auto it = link_maps[i].find(x.first);
            if ( it == link_maps[i].end() ) {
                // No matching link found
                Simulation_impl::getSimulationOutput().output(
                    "WARNING: Unmatched link found in rank link exchange: %s (from rank %d to rank %d)\n",
                    x.first.c_str(), i, my_rank);
            }
            Link* link = reinterpret_cast<Link*>(it->second);
            link->pair_link->setDeliveryInfo(x.second);
        }
        data.clear();
        link_maps[i].clear();
    }

    for ( uint32_t i = my_rank + 1; i < num_ranks.rank; ++i ) {
        // I'm the low rank, so send it first
        // std::map<std::string, uintptr_t> data;
        std::vector<std::pair<std::string, uintptr_t>> data;

        Comms::send(i, 0, link_maps[i]);
        Comms::recv(i, 0, data);

        // Process the data
        for ( auto x : data ) {
            auto it = link_maps[i].find(x.first);
            if ( it == link_maps[i].end() ) {
                // No matching link found
                Simulation_impl::getSimulationOutput().output(
                    "WARNING: Unmatched link found in rank link exchange: %s (from rank %d to rank %d)\n",
                    x.first.c_str(), i, my_rank);
            }
            Link* link = reinterpret_cast<Link*>(it->second);
            link->pair_link->setDeliveryInfo(x.second);
        }
        data.clear();
        link_maps[i].clear();
    }
#endif
}

// Class used to hold the list of profile tools installed in the SyncManager
class SyncProfileToolList
{
public:
    SyncProfileToolList() {}

    void syncManagerStart()
    {
        for ( auto* x : tools )
            x->syncManagerStart();
    }

    void syncManagerEnd()
    {
        for ( auto* x : tools )
            x->syncManagerEnd();
    }

    /**
       Adds a profile tool the the list and registers this handler
       with the profile tool
    */
    void addProfileTool(Profile::SyncProfileTool* tool) { tools.push_back(tool); }

private:
    std::vector<Profile::SyncProfileTool*> tools;
};


SyncManager::SyncManager(
    const RankInfo& rank, const RankInfo& num_ranks, TimeConverter* minPartTC, SimTime_t min_part,
    const std::vector<SimTime_t>& UNUSED(interThreadLatencies)) :
    Action(),
    rank(rank),
    num_ranks(num_ranks),
    threadSync(nullptr),
    min_part(min_part)
{
    sim = Simulation_impl::getSimulation();

    if ( rank.thread == 0 ) {
        for ( auto& b : RankExecBarrier ) {
            b.resize(num_ranks.thread);
        }
        for ( auto& b : LinkUntimedBarrier ) {
            b.resize(num_ranks.thread);
        }
        if ( min_part != MAX_SIMTIME_T ) {
            if ( num_ranks.thread == 1 ) { rankSync = new RankSyncSerialSkip(num_ranks, minPartTC); }
            else {
                rankSync = new RankSyncParallelSkip(num_ranks, minPartTC);
            }
        }
        else {
            rankSync = new EmptyRankSync(num_ranks);
        }
    }

    // Need to check to see if there are any inter-thread
    // dependencies.  If not, EmptyThreadSync, otherwise use one
    // of the active threadsyncs.
    SimTime_t interthread_minlat = sim->getInterThreadMinLatency();
    if ( num_ranks.thread > 1 && interthread_minlat != MAX_SIMTIME_T ) {
        if ( Simulation_impl::getSimulation()->direct_interthread ) {
            threadSync = new ThreadSyncDirectSkip(num_ranks.thread, rank.thread, Simulation_impl::getSimulation());
        }
        else {
            threadSync = new ThreadSyncSimpleSkip(num_ranks.thread, rank.thread, Simulation_impl::getSimulation());
        }
    }
    else {
        threadSync = new EmptyThreadSync(Simulation_impl::getSimulation());
    }

    exit = sim->getExit();

    setPriority(SYNCPRIORITY);
}

SyncManager::~SyncManager() {}

/** Register a Link which this Sync Object is responsible for */
ActivityQueue*
SyncManager::registerLink(const RankInfo& to_rank, const RankInfo& from_rank, const std::string& name, Link* link)
{
    if ( to_rank == from_rank ) {
        return nullptr; // This should never happen
    }

    if ( to_rank.rank == from_rank.rank ) {
        // Same rank, different thread.  Need to send the right data
        // to the two ThreadSync objects for the threads on either
        // side of the link

        // For the local ThreadSync, just need to register the link
        threadSync->registerLink(name, link);

        // Need to get target queue from the remote ThreadSync
        ThreadSync* remoteSync = Simulation_impl::instanceVec[to_rank.thread]->syncManager->threadSync;
        return remoteSync->registerRemoteLink(from_rank.thread, name, link);
    }
    else {
        // Different rank.  Send info onto the RankSync
        return rankSync->registerLink(to_rank, from_rank, name, link);
    }
}

void
SyncManager::exchangeLinkInfo()
{
    rankSync->exchangeLinkInfo(rank.rank);
}

void
SyncManager::execute(void)
{

    SST_SYNC_PROFILE_START

    if ( profile_tools ) profile_tools->syncManagerStart();

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

        // Need to make sure everyone has made it through the mutex
        // and the min time computation is complete
        RankExecBarrier[1].wait();

        // Now call the actual RankSync
        rankSync->execute(rank.thread);

        RankExecBarrier[2].wait();

        // Now call the threadSync after() call
        threadSync->after();

        RankExecBarrier[3].wait();

        if ( exit != nullptr && rank.thread == 0 ) exit->check();

        RankExecBarrier[4].wait();

        if ( exit->getGlobalCount() == 0 ) { endSimulation(exit->getEndTime()); }

        break;
    case THREAD:

        threadSync->execute();

        if ( /*num_ranks.rank == 1*/ min_part == MAX_SIMTIME_T ) {
            if ( exit->getRefCount() == 0 ) { endSimulation(exit->getEndTime()); }
        }

        break;
    default:
        break;
    }
    computeNextInsert();
    RankExecBarrier[5].wait();

    if ( profile_tools ) profile_tools->syncManagerEnd();

    SST_SYNC_PROFILE_STOP
}

/** Cause an exchange of Untimed Data to occur */
void
SyncManager::exchangeLinkUntimedData(std::atomic<int>& msg_count)
{
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
SyncManager::print(const std::string& header, Output& out) const
{
    out.output(
        "%s SyncManager to be delivered at %" PRIu64 " with priority %d\n", header.c_str(), getDeliveryTime(),
        getPriority());
}

uint64_t
SyncManager::getDataSize() const
{
    return rankSync->getDataSize();
}

void
SyncManager::addProfileTool(Profile::SyncProfileTool* tool)
{
    if ( !profile_tools ) profile_tools = new SyncProfileToolList();
    profile_tools->addProfileTool(tool);
}

} // namespace SST
