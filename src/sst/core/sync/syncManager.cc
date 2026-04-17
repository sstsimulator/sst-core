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
#include "sst/core/interactiveConsole.h"
#include "sst/core/objectComms.h"
#include "sst/core/profile/syncProfileTool.h"
#include "sst/core/realtime.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_mpi.h"
#include "sst/core/sync/rankSyncParallelSkip.h"
#include "sst/core/sync/rankSyncSerialSkip.h"
#include "sst/core/sync/syncQueue.h"
#include "sst/core/sync/threadSyncDirectSkip.h"
#include "sst/core/sync/threadSyncSimpleSkip.h"
#include "sst/core/timeConverter.h"
#include "sst/core/util/bit_util.h"

#include <atomic>
#include <cinttypes>
#include <sys/time.h>
#include <unistd.h>

namespace SST {

class InteractiveConsole;

// Static data members
RankSync*                 SyncManager::rankSync_ = nullptr;
Core::ThreadSafe::Barrier SyncManager::RankExecBarrier_[6];
Core::ThreadSafe::Barrier SyncManager::LinkUntimedBarrier_[3];
SimTime_t                 SyncManager::next_rankSync_ = MAX_SIMTIME_T;

class EmptyRankSync : public RankSync
{
public:
    explicit EmptyRankSync(const RankInfo& num_ranks) :
        RankSync(num_ranks)
    {
        nextSyncTime = MAX_SIMTIME_T;
    }
    EmptyRankSync()  = default; // For serialization
    ~EmptyRankSync() = default;

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(
        const RankInfo& UNUSED(to_rank), const RankInfo& UNUSED(from_rank), Link* UNUSED(link)) override
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
        if ( thread != 0 ) {
            return;
        }

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

    void setShutdownFlags(bool UNUSED(enter_shutdown), 
                Simulation_impl::ShutdownMode_t UNUSED(shutdown_mode)) override {}

    void setCkptFlag(bool UNUSED(generate_ckpt)) override {}
    void setFlags(bool UNUSED(enter_interactive), bool UNUSED(enter_shutdown), 
                Simulation_impl::ShutdownMode_t UNUSED(shutdown_mode)) override {}

    void getShutdownFlags( bool& UNUSED(enter_shutdown), Simulation_impl::ShutdownMode_t& UNUSED(shutdown_mode)) override {}
    void getCkptFlag(bool& UNUSED(generate_ckpt)) override {}
    void getFlags( bool& UNUSED(enter_interactive), bool& UNUSED(enter_shutdown), Simulation_impl::ShutdownMode_t& UNUSED(shutdown_mode)) override {}

     /** Clear interactive flags before next run */
    void clearFlags() override {}
    void interactiveExchange() override {}
    void shutdownExchange() override {}

    SimTime_t getNextSyncTime() override { return nextSyncTime; }

    uint64_t getDataSize() const override { return 0; }

    // Don't want to reset time for Empty Sync
    void setRestartTime(SimTime_t UNUSED(time)) override {}

};

class EmptyThreadSync : public ThreadSync
{
public:
    Simulation_impl* sim;

public:
    explicit EmptyThreadSync(Simulation_impl* sim) :
        sim(sim)
    {
        nextSyncTime = MAX_SIMTIME_T;
    }
    EmptyThreadSync()  = default; // For serialization
    ~EmptyThreadSync() = default;

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

    void setShutdownFlags(bool UNUSED(enter_shutdown), 
                Simulation_impl::ShutdownMode_t UNUSED(shutdown_mode)) override {}

    void setFlags(bool UNUSED(enter_interactive), bool UNUSED(enter_shutdown), 
                Simulation_impl::ShutdownMode_t UNUSED(shutdown_mode)) override {}

    void getShutdownFlags( bool& UNUSED(enter_shutdown), Simulation_impl::ShutdownMode_t& UNUSED(shutdown_mode)) override {}

    void getFlags( bool& UNUSED(enter_interactive), bool& UNUSED(enter_shutdown), Simulation_impl::ShutdownMode_t& UNUSED(shutdown_mode)) override {}

     /** Clear interactive flags before next run */
    void clearFlags() override {}

    /** Register a Link which this Sync Object is responsible for */
    void           registerLink(Link* UNUSED(link)) override {}
    ActivityQueue* registerRemoteLink(int UNUSED(tid), Link* UNUSED(link)) override { return nullptr; }

    // Don't want to reset time for Empty Sync
    void setRestartTime(SimTime_t UNUSED(time)) override {}

    /** Serialization for checkpoint support */
};

SimTime_t
ThreadSync::updateMinimumLatency(SimTime_t lat)
{
    static Core::ThreadSafe::Spinlock lock;
    static SimTime_t                  min_latency = bit_util::type_max<SimTime_t>;

    std::scoped_lock slock(lock);
    if ( lat < min_latency ) min_latency = lat;

    return min_latency;
}

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
        std::vector<std::pair<LinkId_t, uintptr_t>> data;

        // Sort the links before sending/receiving
        std::sort(link_maps[i].begin(), link_maps[i].end(),
            [](std::pair<LinkId_t, uintptr_t> const& a, std::pair<LinkId_t, uintptr_t> const& b) {
                return a.first < b.first;
            });

        Comms::recv(i, 0, data);
        Comms::send(i, 0, link_maps[i]);

        // Process the data.  Both lists are sorted by ID, so the corresponding entries in the lists should be the same
        // Link.  For serial loaded and partitioned runs, this should be correct by construction (i.e. we had a valid
        // ConfigGraph so each partition will have the correct links).  For parallel loads, this was also checked during
        // the exchange to settle on the link ID for cross partition links.

        // First check to make sure the vectors are the same size
        if ( data.size() != link_maps[i].size() ) {
            Simulation_impl::getSimulationOutput().fatal(CALL_INFO, EXIT_FAILURE,
                "ERROR: Number of links in link exchange not the same for ranks %d and %d\n", i, my_rank);
        }

        for ( size_t x = 0; x < data.size(); ++x ) {
            if ( data[x].first != link_maps[i][x].first ) {
                Simulation_impl::getSimulationOutput().fatal(CALL_INFO, EXIT_FAILURE,
                    "ERROR: Unmatched links in link exchange for ranks %d and %d\n", i, my_rank);
            }
            Link* link = reinterpret_cast<Link*>(link_maps[i][x].second);
            link->pair_link->setDeliveryInfo(data[x].second);
        }
        data.clear();
    }

    for ( uint32_t i = my_rank + 1; i < num_ranks_.rank; ++i ) {
        // I'm the low rank, so send it first
        std::vector<std::pair<LinkId_t, uintptr_t>> data;

        // Sort the links before sending/receiving
        std::sort(link_maps[i].begin(), link_maps[i].end(),
            [](std::pair<LinkId_t, uintptr_t> const& a, std::pair<LinkId_t, uintptr_t> const& b) {
                return a.first < b.first;
            });

        Comms::send(i, 0, link_maps[i]);
        Comms::recv(i, 0, data);

        // Process the data.  Both lists are sorted by ID, so the corresponding entries in the lists should be the same
        // Link.  For serial loaded and partitioned runs, this should be correct by construction (i.e. we had a valid
        // ConfigGraph so each partition will have the correct links).  For parallel loads, this was also checked during
        // the exchange to settle on the link ID for cross partition links.

        // First check to make sure the vectors are the same size
        if ( data.size() != link_maps[i].size() ) {
            Simulation_impl::getSimulationOutput().fatal(CALL_INFO, EXIT_FAILURE,
                "ERROR: Number of links in link exchange not the same for ranks %d and %d\n", i, my_rank);
        }

        for ( size_t x = 0; x < data.size(); ++x ) {
            if ( data[x].first != link_maps[i][x].first ) {
                Simulation_impl::getSimulationOutput().fatal(CALL_INFO, EXIT_FAILURE,
                    "ERROR: Unmatched links in link exchange for ranks %d and %d\n", i, my_rank);
            }
            Link* link = reinterpret_cast<Link*>(link_maps[i][x].second);
            link->pair_link->setDeliveryInfo(data[x].second);
        }
        data.clear();
    }
#endif
}

SimTime_t
RankSync::findSyncInterval(uint32_t UNUSED_WO_MPI(my_rank))
{
    // We need to find the lowest latency link globally.  Get the lowest for this rank, then we'll do a global reduction
    // to get the final result.
    SimTime_t low_latency = bit_util::type_max<SimTime_t>;

    // Function will not compile if MPI is not configured
#ifdef SST_CONFIG_HAVE_MPI
    // Need to exchange with each partner.  For those with lower
    // ranks, I will recv first, then send.  For those with higher
    // ranks, I will send first, then recv.
    for ( uint32_t i = 0; i < my_rank; ++i ) {
        // Need to update the data in link_maps with the total send latency on the link and then switch it to contain
        // the remote link ptr
        for ( auto& x : link_maps[i] ) {
            // Get the link ptr
            Link* local = reinterpret_cast<Link*>(x.second);

            // We are going to get just the send latency, which is contained in local->pair_link.  The local Link will
            // only have latency if addRecvLatency() was called, but we will account for that later.
            SimTime_t latency = local->pair_link->latency;

            // Update the data
            x.first  = latency;
            // The ptr to the remote link is in pair_link->delivery_info
            x.second = local->pair_link->delivery_info;

            // We don't need to resort the data because it comes across with a uintptr_t version of the local pointer to
            // the Link.
        }

        std::vector<std::pair<SimTime_t, uintptr_t>> data;

        // I'm the high rank, so recv is first
        Comms::recv(i, 0, data);
        Comms::send(i, 0, link_maps[i]);

        // Process the data. Get the Link and the send latency and add any additional receive latency that was added
        // with addRecvLatency(). Compare again current minimum and update if necessary.

        for ( auto& x : data ) {
            Link* local = reinterpret_cast<Link*>(x.second);

            // Added receive latency is found in local->latency
            SimTime_t total_latency = x.first + local->latency;
            if ( total_latency < low_latency ) low_latency = total_latency;
        }
        // Clear data for the next iteration
        data.clear();

        // We're done with the data in link_maps[i]
        link_maps[i].clear();
    }
    for ( uint32_t i = my_rank + 1; i < num_ranks_.rank; ++i ) {
        // Need to update the data in link_maps with the total send latency on the link and then switch it to contain
        // the remote link ptr
        for ( auto& x : link_maps[i] ) {
            // Get the link ptr
            Link* local = reinterpret_cast<Link*>(x.second);

            // We are going to get just the send latency, which is contained in local->pair_link.  The local Link will
            // only have latency if addRecvLatency() was called, but we will account for that later.
            SimTime_t latency = local->pair_link->latency;

            // Update the data
            x.first  = latency;
            // The ptr to the remote link is in pair_link->delivery_info
            x.second = local->pair_link->delivery_info;

            // We don't need to resort the data because it comes across with a uintptr_t version of the local pointer to
            // the Link.
        }

        std::vector<std::pair<SimTime_t, uintptr_t>> data;

        // I'm the low rank, so send is first
        Comms::send(i, 0, link_maps[i]);
        Comms::recv(i, 0, data);

        // Process the data. Get the Link and the send latency and add any additional receive latency that was added
        // with addRecvLatency(). Compare again current minimum and update if necessary.

        for ( auto& x : data ) {
            Link* local = reinterpret_cast<Link*>(x.second);

            // Added receive latency is found in local->latency
            SimTime_t total_latency = x.first + local->latency;
            if ( total_latency < low_latency ) low_latency = total_latency;
        }
        // Clear data for the next iteration
        data.clear();

        // We're done with the data in link_maps[i]
        link_maps[i].clear();
    }

    SimTime_t local_low = low_latency;
    MPI_Allreduce(&local_low, &low_latency, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD);
#endif
    return low_latency;
}

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
            if ( num_ranks_.thread == 1 ) {
                rankSync_ = new RankSyncSerialSkip(num_ranks_);
            }
            else {
                rankSync_ = new RankSyncParallelSkip(num_ranks_);
            }
        }
        else {
            rankSync_ = new EmptyRankSync(num_ranks_);
            if (num_ranks_.rank > 1)
                sim_->getSimulationOutput().output("WARNING: EmptyRankSync: Checkpoint and interactive debug disabled\n");
        }
    }

    // Need to check to see if there are any inter-thread
    // dependencies.  If not, EmptyThreadSync, otherwise use one
    // of the active threadsyncs.
    SimTime_t interthread_minlat = sim_->getInterThreadMinLatency();
    if ( num_ranks_.thread > 1 && interthread_minlat != MAX_SIMTIME_T ) {
        if ( sim_->direct_interthread ) {
            threadSync_ = new ThreadSyncDirectSkip(num_ranks_.thread, rank_.thread, sim_);
        }
        else {
            threadSync_ = new ThreadSyncSimpleSkip(num_ranks_.thread, rank_.thread, sim_);
        }
    }
    else {
        threadSync_ = new EmptyThreadSync(sim_);
    }
}

SyncManager::SyncManager(const RankInfo& rank, const RankInfo& num_ranks, SimTime_t min_part,
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
    ic_barrier_.resize(num_ranks_.thread);

    setPriority(SYNCPRIORITY);
}

SyncManager::SyncManager()
{
    sim_ = Simulation_impl::getSimulation();
}

/** Register a Link which this Sync Object is responsible for */
ActivityQueue*
SyncManager::registerLink(const RankInfo& to_rank, const RankInfo& from_rank, Link* link)
{
    if ( to_rank == from_rank ) {
        return nullptr; // This should never happen
    }

    if ( to_rank.rank == from_rank.rank ) {
        // Same rank, different thread.  Need to send the right data
        // to the two ThreadSync objects for the threads on either
        // side of the link

        // For the local ThreadSync, just need to register the link
        threadSync_->registerLink(link);

        // Need to get target queue from the remote ThreadSync
        ThreadSync* remoteSync = Simulation_impl::instanceVec_[to_rank.thread]->syncManager->threadSync_;
        return remoteSync->registerRemoteLink(from_rank.thread, link);
    }
    else {
        // Different rank.  Send info onto the RankSync
        return rankSync_->registerLink(to_rank, from_rank, link);
    }
}

void
SyncManager::exchangeLinkInfo()
{
    rankSync_->exchangeLinkInfo(rank_.rank);
}

SimTime_t
SyncManager::findRankSyncInterval()
{
    SimTime_t interval = rankSync_->findSyncInterval(rank_.rank);
    // If there are no cross-rank links, then interval will be MAX_SIMTIME_T.  If this happens, we need to change over
    // to an EmptyRankSync
    if ( interval == MAX_SIMTIME_T ) {
        delete rankSync_;
        rankSync_ = new EmptyRankSync(num_ranks_);
    }
    rankSync_->setMaxPeriod(interval);
    return interval;
}

void
SyncManager::updateMinPart()
{
    min_part_ = rankSync_->getMaxPeriod();
}

SimTime_t
SyncManager::findThreadSyncInterval()
{
    SimTime_t interval = threadSync_->findSyncInterval();
    // If there are no cross-rank links, then interval will be MAX_SIMTIME_T.  If this happens, we need to change over
    // to an EmptyThreadSync
    if ( interval == MAX_SIMTIME_T ) {
        delete threadSync_;
        threadSync_ = new EmptyThreadSync(sim_);
    }
    threadSync_->setMaxPeriod(interval);
    return interval;
}

void
SyncManager::getSimShutdownFlags(bool& enter_shutdown, Simulation_impl::ShutdownMode_t& shutdown_mode) {
            
    // Get sim flags to exchange in threadSync
    enter_shutdown = sim_->enter_shutdown_;
    shutdown_mode = sim_->shutdown_mode_;
}

void
SyncManager::getSimFlags(bool& enter_interactive, bool& enter_shutdown, Simulation_impl::ShutdownMode_t& shutdown_mode, bool& generate_ckpt) {
            
    // Get sim flags to exchange in threadSync
    enter_interactive = sim_->enter_interactive_;
    getSimShutdownFlags(enter_shutdown, shutdown_mode);
    generate_ckpt = checkpoint_->getCheckpoint();
}


void
SyncManager::execute()
{
#if 0 // SKK
    std::string type = "RANK";
    if (next_sync_type_ == THREAD)
        type = "THREAD";
    std::cout << "SyncManager::execute: Rank " << rank_.rank 
    << ": Thread " << rank_.thread 
    << ": Type " << type << std::endl;
#endif // SKK  

    if ( profile_tools_ ) profile_tools_->syncManagerStart(next_sync_type_ == RANK);

    bool signals_received = false;
    int  sig_end = 0;
    int  sig_usr = 0;
    int  sig_alrm = 0;
    bool interactive_enabled = false;
    bool enter_interactive = false;
    bool enter_shutdown = false;
    Simulation_impl::ShutdownMode_t shutdown_mode = Simulation_impl::ShutdownMode_t::SHUTDOWN_CLEAN;
    bool generate_ckpt = false;

    if (sim_->interactive_) {
        interactive_enabled = true;
    }


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

        // Get interactive, shutdown, and checkpoint flags
        if (interactive_enabled) {
            getSimFlags(enter_interactive, enter_shutdown, shutdown_mode, generate_ckpt);
            rankSync_->setFlags(enter_interactive, enter_shutdown, shutdown_mode);
        }
        rankSync_->setCkptFlag(generate_ckpt);

        // Now call the actual RankSync.  No barrier needed here
        // because all threads will wait on thread 0 before doing
        // anything
        rankSync_->execute(rank_.thread);

        // Once out of rankSync, signals have been exchanged
        RankExecBarrier_[2].wait();

        // Now call the threadSync after() call
        threadSync_->after();
        
        // Get signals
        signals_received = rankSync_->getSignals(sig_end, sig_usr, sig_alrm);
       
        // Handle any signals
        if ( sig_end )
            real_time_->performSignal(sig_end);
        else if ( signals_received ) {
            if ( sig_usr ) real_time_->performSignal(sig_usr);
            if ( sig_alrm ) real_time_->performSignal(sig_alrm);
        }

        // Generate checkpoint if needed
        rankSync_->getCkptFlag(generate_ckpt);
        if ( generate_ckpt ) {
            checkpoint_->setCheckpoint();
        }
        next_checkpoint_time = checkpoint_->check(getDeliveryTime());

        if (interactive_enabled) {
            rankSync_->getFlags(enter_interactive, enter_shutdown, shutdown_mode);

            // Handle shutdown (all threads/ranks)
            if (enter_shutdown) {
                sim_->setEndSim();
                ic_barrier_.wait();
                if (rank_.thread == 0)
                    rankSync_->clearFlags();
                RankExecBarrier_[5].wait(); 
            }
            // Handle interactive console
            else { 
                if (enter_interactive == true) {
                    sim_->interactive_->execute(sim_->interactive_msg_);
                    sim_->enter_interactive_ = false; // IC may schedule IC again     
                    if (rank_.thread == 0)
                        rankSync_->clearFlags();
                    RankExecBarrier_[5].wait(); 
                }
                
            }
        }

        // No barrier needed. Either the check failed and no
        // checkpoint happened, so no global activity, or the
        // checkpoint happened and the last thing that happens in the
        // checkpoint code is a barrier.
        if ( exit_ != nullptr && rank_.thread == 0 ) exit_->check();

        RankExecBarrier_[3].wait();

        if ( exit_->getGlobalCount() == 0 ) {
            endSimulation(exit_->getEndTime());
        }

        break;
    case THREAD:
        if ( num_ranks_.rank == 1 && rank_.thread == 0 ) {
            real_time_->getSignals(sig_end, sig_usr, sig_alrm);
            threadSync_->setSignals(sig_end, sig_usr, sig_alrm);
        }

        // Move exchange of enter_interactive, shutdown, and checkpoint flags here, similar to getSignals
        // Note that only thread 0 receives signals so it is the only one to execute above
        // However, any thread can trigger interactive or shutdown, so need to have all threads store
        // That is also why the setFlags must be atomic
       
        if (num_ranks_.rank == 1 && interactive_enabled) { 
            // Get local sim flags
            getSimFlags(enter_interactive, enter_shutdown, shutdown_mode, generate_ckpt);
            // Each thread atomically sets shared flags in threadSync
            threadSync_->setFlags(enter_interactive, enter_shutdown, shutdown_mode);
        }

        threadSync_->execute(); // exchange event queues, includes barrier

        // Handle signals for multi-threaded runs/no MPI
        if ( num_ranks_.rank == 1 ) {  
            signals_received = threadSync_->getSignals(sig_end, sig_usr, sig_alrm);
#if 0
            Output& out = sim_->getSimulationOutput();
            out.output("skk:syncmgr:execute: T%d: sig_end=%d, sig_usr=%d, sig_alrm=%d, received=%d\n", rank_.thread,
                sig_end, sig_usr, sig_alrm, signals_received);
#endif
            if ( sig_end )
                real_time_->performSignal(sig_end);
            else if ( signals_received ) {
                if ( sig_usr ) real_time_->performSignal(sig_usr);
                if ( sig_alrm ) real_time_->performSignal(sig_alrm);
            }

            // Check local checkpoint generate flag and set shared generate if needed.
            if ( checkpoint_->getCheckpoint() == true ) {
                ckpt_generate_.store(1);
            }
            // Ensure everyone has written the mask before updating local generate_
            ic_barrier_.wait();
            if ( ckpt_generate_.load() ) {
                checkpoint_->setCheckpoint();
            }
            next_checkpoint_time = checkpoint_->check(getDeliveryTime());
            ckpt_generate_.store(0);

            if (interactive_enabled) {
                threadSync_->getFlags(enter_interactive, enter_shutdown, shutdown_mode);
                if (enter_shutdown) {
                    sim_->setEndSim();
                    ic_barrier_.wait();
                    threadSync_->clearFlags();
                }
                else if (enter_interactive) {
                    sim_->interactive_->execute(sim_->interactive_msg_);
                    sim_->enter_interactive_ = false; // IC may schedule IC again
                    ic_barrier_.wait();
                    threadSync_->clearFlags();
                }
            }
        } // if num_ranks_.rank == 1 i.e. only multithreading

        if ( /*num_ranks_+.rank == 1*/ min_part_ == MAX_SIMTIME_T ) {
            if ( exit_->getRefCount() == 0 ) {
                endSimulation(exit_->getEndTime());
            }
        }
        break;
    default:
        break;
    } // end switch
    computeNextInsert(next_checkpoint_time);
    RankExecBarrier_[4].wait();

    if ( profile_tools_ ) profile_tools_->syncManagerEnd();
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
        if ( num_ranks_.rank == 1 ) {
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
    out.output("%s SyncManager to be delivered at %" PRIu64 " with priority %d\n", header.c_str(), getDeliveryTime(),
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
    if ( !profile_tools_ ) {
        profile_tools_ = new Profile::SyncProfileToolList();
        if ( rank_.thread == 0 ) {
            rankSync_->setProfileToolList(profile_tools_);
        }
    }
    profile_tools_->addProfileTool(tool);
}

std::atomic<unsigned>     SyncManager::ckpt_generate_ { 0 };
Core::ThreadSafe::Barrier SyncManager::ic_barrier_;


} // namespace SST
