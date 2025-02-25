// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/sync/syncManager.h"

#include "sst/core/checkpointAction.h"
#include "sst/core/exit.h"
#include "sst/core/objectComms.h"
#include "sst/core/profile/syncProfileTool.h"
#include "sst/core/realtime.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sync/rankSyncParallelSkip.h"
#include "sst/core/sync/rankSyncSerialSkip.h"
#include "sst/core/sync/syncQueue.h"
#include "sst/core/sync/threadSyncDirectSkip.h"
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
RankSync*                 SyncManager::rankSync_ = nullptr;
Core::ThreadSafe::Barrier SyncManager::RankExecBarrier_[5];
Core::ThreadSafe::Barrier SyncManager::LinkUntimedBarrier_[3];
SimTime_t                 SyncManager::next_rankSync_ = MAX_SIMTIME_T;

#if SST_SYNC_PROFILING

#if SST_HIGH_RESOLUTION_CLOCK
#define SST_SYNC_PROFILE_START                                  \
    auto sim                = Simulation_impl::getSimulation(); \
    auto last_sync_type     = next_sync_type_;                  \
    auto sync_profile_start = std::chrono::high_resolution_clock::now();

#define SST_SYNC_PROFILE_STOP                                                                                 \
    auto sync_profile_stop = std::chrono::high_resolution_clock::now();                                       \
    auto sync_profile_count =                                                                                 \
        std::chrono::duration_cast<std::chrono::nanoseconds>(sync_profile_stop - sync_profile_start).count(); \
    sim_->incrementSyncTime(last_sync_type == RANK, sync_profile_count);

#else
#define SST_SYNC_PROFILE_START                                               \
    auto           sim            = Simulation_impl::getSimulation();        \
    auto           last_sync_type = next_sync_type_;                         \
    struct timeval sync_profile_stop, sync_profile_start, sync_profile_diff; \
    gettimeofday(&sync_profile_start, NULL);

#define SST_SYNC_PROFILE_STOP                                                               \
    gettimeofday(&sync_profile_stop, NULL);                                                 \
    timersub(&sync_profile_stop, &sync_profile_start, &sync_profile_diff);                  \
    auto sync_profile_count = (sync_profile_diff.tv_usec + sync_profile_diff.tv_sec * 1e6); \
    sim_->incrementSyncTime(last_sync_type == RANK, sync_profile_count);

#endif // SST_HIGH_RESOLUTION_CLOCK

#else // SST_SYNC_PROFILING
#define SST_SYNC_PROFILE_START
#define SST_SYNC_PROFILE_STOP
#endif // SST_SYNC_PROFILING

class EmptyRankSync : public RankSync
{
public:
    EmptyRankSync(const RankInfo& num_ranks) : RankSync(num_ranks) { nextSyncTime = MAX_SIMTIME_T; }
    EmptyRankSync() {} // For serialization
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

    void setSignals(int UNUSED(end), int UNUSED(usr), int UNUSED(alrm)) override {}

    bool getSignals(int& end, int& usr, int& alrm) override
    {
        end  = 0;
        usr  = 0;
        alrm = 0;
        return false;
    }

    SimTime_t getNextSyncTime() override { return nextSyncTime; }

    TimeConverter* getMaxPeriod() { return max_period; }

    uint64_t getDataSize() const override { return 0; }

    // Don't want to reset time for Empty Sync
    void setRestartTime(SimTime_t UNUSED(time)) override {}
};

class EmptyThreadSync : public ThreadSync
{
public:
    Simulation_impl* sim;

public:
    EmptyThreadSync(Simulation_impl* sim) : sim(sim) { nextSyncTime = MAX_SIMTIME_T; }
    EmptyThreadSync() {} // For serialization
    ~EmptyThreadSync() {}

    void before() override {}
    void after() override {}
    void execute() override {}
    void processLinkUntimedData() override {}
    void finalizeLinkConfigurations() override {}
    void prepareForComplete() override {}

    void setSignals(int UNUSED(end), int UNUSED(usr), int UNUSED(alrm)) override {}

    bool getSignals(int& end, int& usr, int& alrm) override
    {
        end  = 0;
        usr  = 0;
        alrm = 0;
        return false;
    }

    /** Register a Link which this Sync Object is responsible for */
    void           registerLink(const std::string& UNUSED(name), Link* UNUSED(link)) override {}
    ActivityQueue* registerRemoteLink(int UNUSED(tid), const std::string& UNUSED(name), Link* UNUSED(link)) override
    {
        return nullptr;
    }

    // Don't want to reset time for Empty Sync
    void setRestartTime(SimTime_t UNUSED(time)) override {}

    /** Serialization for checkpoint support */
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

    for ( uint32_t i = my_rank + 1; i < num_ranks_.rank; ++i ) {
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

void
SyncManager::setupSyncObjects()
{
    if ( rank_.thread == 0 ) {
        for ( auto& b : RankExecBarrier_ ) {
            b.resize(num_ranks_.thread);
        }
        for ( auto& b : LinkUntimedBarrier_ ) {
            b.resize(num_ranks_.thread);
        }
        if ( min_part_ != MAX_SIMTIME_T ) {
            if ( num_ranks_.thread == 1 ) { rankSync_ = new RankSyncSerialSkip(num_ranks_); }
            else {
                rankSync_ = new RankSyncParallelSkip(num_ranks_);
            }
        }
        else {
            rankSync_ = new EmptyRankSync(num_ranks_);
        }
    }

    // Need to check to see if there are any inter-thread
    // dependencies.  If not, EmptyThreadSync, otherwise use one
    // of the active threadsyncs.
    SimTime_t interthread_minlat = sim_->getInterThreadMinLatency();
    if ( num_ranks_.thread > 1 && interthread_minlat != MAX_SIMTIME_T ) {
        if ( Simulation_impl::getSimulation()->direct_interthread ) {
            threadSync_ = new ThreadSyncDirectSkip(num_ranks_.thread, rank_.thread, Simulation_impl::getSimulation());
        }
        else {
            threadSync_ = new ThreadSyncSimpleSkip(num_ranks_.thread, rank_.thread, Simulation_impl::getSimulation());
        }
    }
    else {
        threadSync_ = new EmptyThreadSync(Simulation_impl::getSimulation());
    }
}

SyncManager::SyncManager(
    const RankInfo& rank, const RankInfo& num_ranks, SimTime_t min_part,
    const std::vector<SimTime_t>& UNUSED(interThreadLatencies), RealTimeManager* real_time) :
    Action(),
    rank_(rank),
    num_ranks_(num_ranks),
    threadSync_(nullptr),
    min_part_(min_part),
    real_time_(real_time)
{
    sim_ = Simulation_impl::getSimulation();

    setupSyncObjects();

    exit_       = sim_->getExit();
    checkpoint_ = sim_->getCheckpointAction();

    setPriority(SYNCPRIORITY);
}

SyncManager::SyncManager()
{
    sim_ = Simulation_impl::getSimulation();
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
        threadSync_->registerLink(name, link);

        // Need to get target queue from the remote ThreadSync
        ThreadSync* remoteSync = Simulation_impl::instanceVec_[to_rank.thread]->syncManager->threadSync_;
        return remoteSync->registerRemoteLink(from_rank.thread, name, link);
    }
    else {
        // Different rank.  Send info onto the RankSync
        return rankSync_->registerLink(to_rank, from_rank, name, link);
    }
}

void
SyncManager::exchangeLinkInfo()
{
    rankSync_->exchangeLinkInfo(rank_.rank);
}

void
SyncManager::execute()
{
    SST_SYNC_PROFILE_START

    if ( profile_tools_ ) profile_tools_->syncManagerStart();

    bool signals_received;
    int  sig_end;
    int  sig_usr;
    int  sig_alrm;

    SimTime_t next_checkpoint_time = MAX_SIMTIME_T;

    switch ( next_sync_type_ ) {
    case RANK:
        // Need to make sure all threads have reached the sync to
        // guarantee that all events have been sent to the appropriate
        // queues.
        RankExecBarrier_[0].wait();
        // GV: At this point thread 0 is in the sync, new signals will be deferred to the next sync

        // For a rank sync, we will force a thread sync first.  This
        // will ensure that all events sent between threads will be
        // flushed into their respective TimeVortices.  We need to do
        // this to enable any skip ahead optimizations.
        threadSync_->before();

        // Need to make sure everyone has made it through the mutex
        // and the min time computation is complete
        RankExecBarrier_[1].wait();

        if ( rank_.thread == 0 ) {
            real_time_->getSignals(sig_end, sig_usr, sig_alrm);
            rankSync_->setSignals(sig_end, sig_usr, sig_alrm);
        }
        // Now call the actual RankSync.  No barrier needed here
        // because all threads will wait on thread 0 before doing
        // anything
        rankSync_->execute(rank_.thread);

        // Once out of rankSync, signals have been exchanged
        RankExecBarrier_[2].wait();

        // Now call the threadSync after() call
        threadSync_->after();

        // Handle signals
        signals_received = rankSync_->getSignals(sig_end, sig_usr, sig_alrm);

        // Handle any signals
        if ( sig_end )
            real_time_->performSignal(sig_end);
        else if ( signals_received ) {
            if ( sig_usr ) real_time_->performSignal(sig_usr);
            if ( sig_alrm ) real_time_->performSignal(sig_alrm);
        }

        // Generate checkpoint if needed
        next_checkpoint_time = checkpoint_->check(getDeliveryTime());

        // No barrier needed. Either the check failed and no
        // checkpoint happened, so no global activity, or the
        // checkpoint happened and the last thing that happens in the
        // checkpoint code is a barrier.

        if ( exit_ != nullptr && rank_.thread == 0 ) exit_->check();

        RankExecBarrier_[3].wait();

        if ( exit_->getGlobalCount() == 0 ) { endSimulation(exit_->getEndTime()); }

        break;
    case THREAD:
        if ( num_ranks_.rank == 1 && rank_.thread == 0 ) {
            real_time_->getSignals(sig_end, sig_usr, sig_alrm);
            threadSync_->setSignals(sig_end, sig_usr, sig_alrm);
        }
        threadSync_->execute();

        // Handle signals for multi-threaded runs/no MPI
        if ( num_ranks_.rank == 1 ) {
            signals_received = threadSync_->getSignals(sig_end, sig_usr, sig_alrm);
            if ( sig_end )
                real_time_->performSignal(sig_end);
            else if ( signals_received ) {
                if ( sig_usr ) real_time_->performSignal(sig_usr);
                if ( sig_alrm ) real_time_->performSignal(sig_alrm);
            }
            next_checkpoint_time = checkpoint_->check(getDeliveryTime());
        }

        if ( /*num_ranks_+.rank == 1*/ min_part_ == MAX_SIMTIME_T ) {
            if ( exit_->getRefCount() == 0 ) { endSimulation(exit_->getEndTime()); }
        }


        break;
    default:
        break;
    }
    computeNextInsert(next_checkpoint_time);
    RankExecBarrier_[4].wait();

    if ( profile_tools_ ) profile_tools_->syncManagerEnd();

    SST_SYNC_PROFILE_STOP
}

/** Cause an exchange of Untimed Data to occur */
void
SyncManager::exchangeLinkUntimedData(std::atomic<int>& msg_count)
{
    LinkUntimedBarrier_[0].wait();
    threadSync_->processLinkUntimedData();
    LinkUntimedBarrier_[1].wait();
    rankSync_->exchangeLinkUntimedData(rank_.thread, msg_count);
    LinkUntimedBarrier_[2].wait();
}

/** Finish link configuration */
void
SyncManager::finalizeLinkConfigurations()
{
    threadSync_->finalizeLinkConfigurations();
    // Only thread 0 should call finalize on rankSync
    if ( rank_.thread == 0 ) rankSync_->finalizeLinkConfigurations();

    // Need to figure out what sync comes first and insert object into
    // TimeVortex
    if ( num_ranks_.rank == 1 && num_ranks_.thread == 1 ) return;
    if ( checkpoint_ )
        computeNextInsert(checkpoint_->getNextCheckpointSimTime());
    else
        computeNextInsert();
}

/** Prepare for complete() phase */
void
SyncManager::prepareForComplete()
{
    threadSync_->prepareForComplete();
    // Only thread 0 should call finalize on rankSync
    if ( rank_.thread == 0 ) rankSync_->prepareForComplete();
}

void
SyncManager::computeNextInsert(SimTime_t next_checkpoint_time)
{
    SimTime_t next_rank_sync   = rankSync_->getNextSyncTime();
    SimTime_t next_thread_sync = threadSync_->getNextSyncTime();

    SimTime_t next_sync_time = next_thread_sync;
    next_sync_type_          = THREAD;

    if ( next_rank_sync <= next_thread_sync ) {
        next_sync_type_ = RANK;
        next_sync_time  = next_rank_sync;
    }

    if ( next_checkpoint_time < next_sync_time ) {
        next_sync_time = next_checkpoint_time;
        if ( next_rank_sync == MAX_SIMTIME_T ) {
            // Single rank job
            next_sync_type_ = THREAD;
        }
        else {
            // If using multiple ranks, must do a full rank sync
            next_sync_type_ = RANK;
        }
    }

    sim_->insertActivity(next_sync_time, this);
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
    return rankSync_->getDataSize();
}

void
SyncManager::addProfileTool(Profile::SyncProfileTool* tool)
{
    if ( !profile_tools_ ) profile_tools_ = new SyncProfileToolList();
    profile_tools_->addProfileTool(tool);
}

} // namespace SST
