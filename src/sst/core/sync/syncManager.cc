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

#include <atomic>
#include <cinttypes>
#include <sys/time.h>
#include <unistd.h>

#define SKK 0

namespace SST {

class InteractiveConsole;

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
    explicit EmptyRankSync(const RankInfo& num_ranks) :
        RankSync(num_ranks)
    {
        nextSyncTime = MAX_SIMTIME_T;
    }
    EmptyRankSync()  = default; // For serialization
    ~EmptyRankSync() = default;

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(const RankInfo& UNUSED(to_rank), const RankInfo& UNUSED(from_rank),
        const std::string& UNUSED(name), Link* UNUSED(link)) override
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

    TimeConverter getMaxPeriod() { return max_period; }

    uint64_t getDataSize() const override { return 0; }

    // Don't want to reset time for Empty Sync
    void setRestartTime(SimTime_t UNUSED(time)) override {}

    void testManager() override {}
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
    SyncProfileToolList() = default;

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
            if ( num_ranks_.thread == 1 ) {
                rankSync_ = new RankSyncSerialSkip(num_ranks_);
            }
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

    // sim_->getSimulationOutput().output("skk:syncmgr:constructor: T%d\n", rank_.thread);
}

SyncManager::SyncManager()
{
    sim_ = Simulation_impl::getSimulation();
}

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
 SyncManager::getSimShutdownFlags(bool& enter_shutdown, Simulation_impl::ShutdownMode_t& shutdown_mode) {
            
            // Get sim flags to exchange in threadSync
            enter_shutdown = sim_->enter_shutdown_;
            shutdown_mode = sim_->shutdown_mode_;
 }

 // sim_->getSimFlags(enter_interactive, enter_shutdown, shutdown_mode, checkpoint)
 void
 SyncManager::getSimFlags(bool& enter_interactive, bool& enter_shutdown, Simulation_impl::ShutdownMode_t& shutdown_mode, bool& generate_ckpt) {
            
            // Get sim flags to exchange in threadSync
            enter_interactive = sim_->enter_interactive_;
            getSimShutdownFlags(enter_shutdown, shutdown_mode);
            generate_ckpt = checkpoint_->getCheckpoint();
 }

 void
SyncManager::rankHandleShutdown()
{
    // If console is handled only by thread 0, then only thread 0 needs to check for cmd_shutdown here
    // The watchpoint action shutdown can be triggered by any thread
#if 0
    // Check for shutdown
    Output& out = sim_->getSimulationOutput();
    out.output("skk:syncmgr:handleShutdown:Before T%d:endSim=%d, shutdown_mode=%d, enter_shutdown=%d, \n", rank_.thread,
        sim_->endSim, sim_->shutdown_mode_, sim_->enter_shutdown_);
#endif

    bool enter_shutdown;
    Simulation_impl::ShutdownMode_t shutdown_mode;

    getSimShutdownFlags(enter_shutdown, shutdown_mode); // get flags from sim_
    //printf("rHS0: Rank%d, Thread%d: rankHandleShutdown: sim->: enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
        //rank_.rank, rank_.thread, sim_->enter_interactive_, sim_->enter_shutdown_, sim_->shutdown_mode_);

    rankSync_->setShutdownFlags(enter_shutdown, shutdown_mode);  // Atomically update shared values in threadSync
    rankSync_->shutdownExchange();
    RankExecBarrier_[0].wait(); // ensure the exchange is done before getting value
    rankSync_->getShutdownFlags(enter_shutdown, shutdown_mode);  // Get shared (cumulative) flags from threadSync
   
    //printf("rHS1: Rank%d, Thread%d: Enter handleShutdown: enter_shutdown %d, shutdown_mode %d\n", 
        //rank_.rank, rank_.thread, enter_shutdown, shutdown_mode);

    if ( enter_shutdown ) {
        sim_->setEndSim();
        //printf("Rank%d, Thread%d: handleShutdown setEndSim: \n", rank_.rank, rank_.thread);
        // Clear sim flags
        // SKK May be duplicative for now because this function called directly in sim for --interactive-start=0
        sim_->enter_interactive_ = false;
        sim_->enter_shutdown_ = false;
        // SKK No need to threadSync-> clear because we're just going to shutdown?
    }
    
    //printf("After HandleShutdown: sim: sim_->enter_interactive %d, sim_->enter_shutdown %d, sim_->shutdown_mode %d\n", 
    //    enter_interactive, enter_shutdown, shutdown_mode);
    //return false;
}

void 
SyncManager::handleShutdown()
{
    // If console is handled only by thread 0, then only thread 0 needs to check for cmd_shutdown here
    // The watchpoint action shutdown can be triggered by any thread
#if 0
    // Check for shutdown
    Output& out = sim_->getSimulationOutput();
    out.output("skk:syncmgr:handleShutdown:Before T%d:endSim=%d, shutdown_mode=%d, enter_shutdown=%d, \n", rank_.thread,
        sim_->endSim, sim_->shutdown_mode_, sim_->enter_shutdown_);
#endif

    printf("Rank%d, Thread%d: Enter handleShutdown: sim->: enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
        rank_.rank, rank_.thread, sim_->enter_interactive_, sim_->enter_shutdown_, sim_->shutdown_mode_);

    bool enter_shutdown;
    Simulation_impl::ShutdownMode_t shutdown_mode;

    getSimShutdownFlags(enter_shutdown, shutdown_mode); // get flags from sim_
    threadSync_->setShutdownFlags(enter_shutdown, shutdown_mode);  // Atomically update shared values in threadSync
    ic_barrier_.wait(); // barrier
    threadSync_->getShutdownFlags(enter_shutdown, shutdown_mode);  // Get shared (cumulative) flags from threadSync
   
    if ( enter_shutdown ) {
        sim_->setEndSim();
        printf("Rank%d, Thread%d: handleShutdown setEndSim: \n", rank_.rank, rank_.thread);
        // Clear sim flags
        // SKK May be duplicative for now because this function called directly in sim for --interactive-start=0
        sim_->enter_interactive_ = false;
        sim_->enter_shutdown_ = false;
        // SKK No need to threadSync-> clear because we're just going to shutdown?
    }
    
    //printf("After HandleShutdown: sim: sim_->enter_interactive %d, sim_->enter_shutdown %d, sim_->shutdown_mode %d\n", 
    //    enter_interactive, enter_shutdown, shutdown_mode);
    //return false;
}

#if 1
void
SyncManager::testRankFlagExchange() {
    if (rank_.rank == 0) { // R0 sets shutdown for exchange, assumes single threaded
        sim_-> enter_shutdown_ = true;
        sim_-> shutdown_mode_ = Simulation_impl::ShutdownMode_t::SHUTDOWN_CLEAN;
    }
    //RankExecBarrier_[0].wait();  // SKK works with and w/o barrier
    printf("5: Rank%d, Thread%d: testRankFlagExchange: sim: enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
                    rank_.rank, rank_.thread, sim_->enter_interactive_, sim_->enter_shutdown_, sim_->shutdown_mode_);

    rankHandleShutdown();  // works because the shutdown gets exchanges btwn ranks
    //handleShutdown();  // SKK hangs because the shutdown doesn't get exchanged
}
#endif

#if 0
void
SyncManager::rankHandleInteractiveConsole()
{
    ic_barrier_.wait(); // SKK This one may not be necessary...

    // Handle interactive console for multithreaded runs
    // Serial execution handles this in simulation run so it happens right away
    //assert(num_ranks_.thread != 1); // SKK I should never have a threadSync with a single thread, correct?, I might in multi-rank


#if 0
    Output& out = sim_->getSimulationOutput();
    out.output("skk:syncmgr:execute: T%d: check enter_interactive_=%d\n", rank_.thread, sim_->enter_interactive_);
    out.output("skk:syncmgr:execute: T%d: enter_interactive_mask_=0x%x\n", rank_.thread, ic_mask);
#endif
    // 2) Print list of threads (in order) and whether triggered
#if 1
    for ( uint32_t tindex = 0; tindex < num_ranks_.thread; tindex++ ) {
        if ( rank_.thread == tindex ) {
            if ( rank_.thread == 0 ) std::cout << "\nINTERACTIVE CONSOLE\n";
            std::cout << "  Rank:" << rank_.rank << "/" << num_ranks_.rank << " Thread:" << rank_.thread << "/"
                        << num_ranks_.thread;
            if ( sim_->enter_interactive_ ) {
                std::cout << " (Triggered)\n";
            }
            else {
                std::cout << " (Not Triggered)\n";
            }
#endif
#if 1 // Print component summary? - will be at whatever level was last (maybe print PWD instead?)
            if (sim_->interactive_ != nullptr) {
                sim_->interactive_->summary();
            }
#endif
        }
        ic_barrier_.wait();
    }

    // 3) T0: Invoke IC for T0 (or Query which thread to inspect?)
#if 1
    if ( rank_.thread == 0 ) {
        current_ic_thread_.store(0);
    }
    ic_barrier_.wait(); // Ensure store is complete before everyone checks
#else
    if ( rank_.thread == 0 ) {
        current_ic_thread_ = num_ranks_.thread;
        std::cout << "\n---- Enter thread ID (0 to " << num_ranks_.thread - 1
                    << ") to inspect in interactive console or " << num_ranks_.thread
                    << " to continue simulation\n";
        std::cin >> current_ic_thread_;
        std::cout << "skk: set tid to " << current_ic_thread_ << std::endl;
    }
    ic_barrier_.wait();
#endif
    // 4) Tj: Invoke IC for current thread, with ability to change to new thread
    
    unsigned int tid      = current_ic_thread_.load();
    int          ic_state = 0;
    while ( ic_state != InteractiveConsole::ICretcode::DONE ) {
        if ( (rank_.thread == tid) && (sim_->interactive_ != nullptr) ) {
        
            // Invoke IC for the thread and capture return state
            int result = sim_->interactive_->execute(sim_->interactive_msg_);

            if ( result >= 0 ) { // change thread to threadID <result>
                current_ic_thread_.store(result);
                current_ic_state_.store(0);
            }
            else { // DONE (-1) or Print SUMMARY (-2)
                current_ic_state_.store(result);
            }
        }
        ic_barrier_.wait();
        // If console is handled only by thread 0, then only thread 0 needs to check for cmd_shutdown here
        // Could we handle this directly? If cmd_shutdown, then return SHUTDOWN (-3) and share it across threads?

        //RankExecBarrier_[1].wait();
        rankHandleShutdown(); // Check if console issued shutdown command  SKK have this return true/false for shutdown to skip next step
        
        
        tid      = current_ic_thread_.load();
        ic_state = current_ic_state_.load();
        //out.output("T%d: tid %d, ic_state %d\n", rank_.thread, tid, ic_state);
        if ( ic_state == InteractiveConsole::ICretcode::SUMMARY ) { // Print thread info summary
            for ( uint32_t tindex = 0; tindex < num_ranks_.thread; tindex++ ) {
                if ( rank_.thread == tindex ) {
                    std::cout << "Rank:" << rank_.rank << " Thread:" << rank_.thread
                                << " (Process:" << getppid() << ") ";
                    // Print component summary
                    if ( sim_->interactive_ != nullptr ) {
                        sim_->interactive_->summary();
                    }
                }
                ic_barrier_.wait();
            }
            current_ic_state_.store(0);
        } // if state == SUMMARY, print thread info summary
    }

    // SKK Right now this is called directly for --interactive-start=0, so we need to clear here
    sim_->enter_interactive_ = false;
    //printf("After while loop: sim: sim_->enter_interactive %d\n", sim_->enter_interactive_);
    // SKK Do I need to clear outside flags? I should not need shared enter_ic or shutdown inside here
    // Ensure everyone has latest flags before clearing
    ic_barrier_.wait();
    rankSync_->clearFlags();

}
#endif

#if 0
void
SyncManager::handleInteractiveConsole()
{
    ic_barrier_.wait(); // SKK This one may not be necessary...

    // Handle interactive console for multithreaded runs
    // Serial execution handles this in simulation run so it happens right away
    //assert(num_ranks_.thread != 1); // SKK I should never have a threadSync with a single thread, correct?, I might in multi-rank

#if 0
    Output& out = sim_->getSimulationOutput();
    out.output("skk:syncmgr:execute: T%d: check enter_interactive_=%d\n", rank_.thread, sim_->enter_interactive_);
    out.output("skk:syncmgr:execute: T%d: enter_interactive_mask_=0x%x\n", rank_.thread, ic_mask);
#endif
    // 2) Print list of threads (in order) and whether triggered
    for ( uint32_t tindex = 0; tindex < num_ranks_.thread; tindex++ ) {
        if ( rank_.thread == tindex ) {
            if ( rank_.thread == 0 ) std::cout << "\nINTERACTIVE CONSOLE\n";
            std::cout << "  Rank:" << rank_.rank << "/" << num_ranks_.rank << " Thread:" << rank_.thread << "/"
                        << num_ranks_.thread;
            if ( sim_->enter_interactive_ ) {
                std::cout << " (Triggered)\n";
            }
            else {
                std::cout << " (Not Triggered)\n";
            }
#if 1       // Print component summary? - will be at whatever level was last (maybe print PWD instead?)
            if (sim_->interactive_ != nullptr) {
                sim_->interactive_->summary();
            }
#endif
        }
        ic_barrier_.wait();
    }

    // 3) T0: Invoke IC for T0 (or Query which thread to inspect?)
#if 1
    if ( rank_.thread == 0 ) {
        current_ic_thread_.store(0);
    }
    ic_barrier_.wait(); // Ensure store is complete before everyone checks
#else
    if ( rank_.thread == 0 ) {
        current_ic_thread_ = num_ranks_.thread;
        std::cout << "\n---- Enter thread ID (0 to " << num_ranks_.thread - 1
                    << ") to inspect in interactive console or " << num_ranks_.thread
                    << " to continue simulation\n";
        std::cin >> current_ic_thread_;
        std::cout << "skk: set tid to " << current_ic_thread_ << std::endl;
    }
    ic_barrier_.wait();
#endif
    // 4) Tj: Invoke IC for current thread, with ability to change to new thread
    unsigned int tid      = current_ic_thread_.load();
    int          ic_state = 0;
    while ( ic_state != InteractiveConsole::ICretcode::DONE ) {
        if ( (rank_.thread == tid) && (sim_->interactive_ != nullptr) ) {
            // Invoke IC for the thread and capture return state
            int result = sim_->interactive_->execute(sim_->interactive_msg_);

            if ( result >= 0 ) { // change thread to threadID <result>
                current_ic_thread_.store(result);
                current_ic_state_.store(0);
            }
            else { // DONE (-1) or Print SUMMARY (-2)
                current_ic_state_.store(result);
            }
        }
        ic_barrier_.wait();
        // If console is handled only by thread 0, then only thread 0 needs to check for cmd_shutdown here
        // Could we handle this directly? If cmd_shutdown, then return SHUTDOWN (-3) and share it across threads?
        
        handleShutdown(); // Check if console issued shutdown command  SKK have this return true/false for shutdown to skip next step

        tid      = current_ic_thread_.load();
        ic_state = current_ic_state_.load();
        //out.output("T%d: tid %d, ic_state %d\n", rank_.thread, tid, ic_state);
        if ( ic_state == InteractiveConsole::ICretcode::SUMMARY ) { // Print thread info summary
            for ( uint32_t tindex = 0; tindex < num_ranks_.thread; tindex++ ) {
                if ( rank_.thread == tindex ) {
                    std::cout << "Rank:" << rank_.rank << " Thread:" << rank_.thread
                                << " (Process:" << getppid() << ") ";
                    // Print component summary
                    if ( sim_->interactive_ != nullptr ) {
                        sim_->interactive_->summary();
                    }
                }
                ic_barrier_.wait();
            }
            current_ic_state_.store(0);
        } // if state == SUMMARY, print thread info summary
    }

    // SKK Right now this is called directly for --interactive-start=0, so we need to clear here
    sim_->enter_interactive_ = false;
    //printf("After while loop: sim: sim_->enter_interactive %d\n", sim_->enter_interactive_);
    // SKK Do I need to clear outside flags? I should not need shared enter_ic or shutdown inside here
    // Ensure everyone has latest flags before clearing
    ic_barrier_.wait();
    threadSync_->clearFlags();     
}
#endif

void
SyncManager::partitionInfo() {
    Output& out = sim_->getSimulationOutput();
    if (rank_.rank == 0 && rank_.thread == 0) {
        out.output("INTERACTIVE CONSOLE\n");
    }
    for (uint32_t rindex = 0; rindex < num_ranks_.rank; rindex++ ) {
        for ( uint32_t tindex = 0; tindex < num_ranks_.thread; tindex++ ) {
            if ( rank_.rank == rindex && rank_.thread == tindex ) {
                #if 0
                if (rank_.rank == 0 && rank_.thread == 0) {
                    out.output("INTERACTIVE CONSOLE\n");
                }
                #endif
                //out.output("Rank:%d, Thread:%d (Process %d)", rank_.rank, rank_.thread, getppid());
                // Print component summary
                if ( sim_->interactive_ != nullptr ) {
                    sim_->interactive_->summary();
                }
            }
            ic_barrier_.wait();
        } 
        RankExecBarrier_[0].wait();
    }
}

void
SyncManager::testProducerConsumer() 
{
#if 1
    rankSync_->testManager();
#else
    if (rank_.thread == 0)
        rankSync_->testManager();
    else
        rankSync_->testWorker();
#endif
#if 1
    Output& out = sim_->getSimulationOutput();
    out.output("**SyncManager::testProducerConsumer: before barrier R%d: T%d\n", 
        rank_.rank, rank_.thread);
#endif 

    RankExecBarrier_[0].wait();
    sim_->setEndSim();

}

void
SyncManager::execute()
{
#if 1 // SKK
    std::string type = "RANK";
    if (next_sync_type_ == THREAD)
        type = "THREAD";
    Output& out = sim_->getSimulationOutput();
    //out.output("SyncManager::execute: Rank %d: Thread %d: Type %s\n", 
    //    rank_.rank, rank_.thread, type.c_str());
#else 
    std::string type = "RANK";
    if (next_sync_type_ == THREAD)
        type = "THREAD";
    std::cout << "SyncManager::execute: Rank " << rank_.rank 
    << ": Thread " << rank_.thread 
    << ": Type " << type << std::endl;
#endif // SKK  

    SST_SYNC_PROFILE_START

    if ( profile_tools_ ) profile_tools_->syncManagerStart();

    bool signals_received;
    int  sig_end;
    int  sig_usr;
    int  sig_alrm;
    bool enter_interactive;
    bool enter_shutdown;
    Simulation_impl::ShutdownMode_t shutdown_mode;
    bool generate_ckpt;


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
#if 1
        // Get interactive, shutdown, and checkpoint flags
        //printf("0: Rank%d, Thread%d: sim_- Flags: enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
        //            rank_.rank, rank_.thread, sim_->enter_interactive_, sim_->enter_shutdown_, sim_->shutdown_mode_);
        getSimFlags(enter_interactive, enter_shutdown, shutdown_mode, generate_ckpt);
        #if 1
        rankSync_->setFlags(enter_interactive, enter_shutdown, shutdown_mode);
        rankSync_->setCkptFlag(generate_ckpt);
        //printf("1: Rank%d, Thread%d: Flags: enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
        //            rank_.rank, rank_.thread, enter_interactive, enter_shutdown, shutdown_mode);
     
        #else
        threadSync_->setFlags(enter_interactive, enter_shutdown, shutdown_mode);
        ic_barrier_.wait();
        threadSync_->getFlags(enter_interactive, enter_shutdown, shutdown_mode);
        #endif
        //printf("After threadSync_->getFlags: enter_interactive %d, enter_shutdown %d, shutdown_mode %d \n",
        //    enter_interactive, enter_shutdown, shutdown_mode);
#endif        

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
#if 1
        rankSync_->getCkptFlag(generate_ckpt);
        if ( generate_ckpt ) {
            checkpoint_->setCheckpoint();
        }
        next_checkpoint_time = checkpoint_->check(getDeliveryTime());
#else
        // Check local checkpoint generate flag and set shared generate if needed.
            if ( checkpoint_->getCheckpoint() == true ) {
                ckpt_generate_.store(1);
            }
            // Ensure everyone has written the mask before updating local generate_
            ic_barrier_.wait();
            printf("2.5: Rank%d, Thread%d: ckpt_generate_ %d\n", 
                rank_.rank, rank_.thread, ckpt_generate_.load());
            if ( ckpt_generate_.load() ) {
                checkpoint_->setCheckpoint();
            }
            next_checkpoint_time = checkpoint_->check(getDeliveryTime());
            ckpt_generate_.store(0);

        //next_checkpoint_time = checkpoint_->check(getDeliveryTime());
#endif

#if 1
        rankSync_->getFlags(enter_interactive, enter_shutdown, shutdown_mode);
        //printf("2: Rank%d, Thread%d: Flags: enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
        //            rank_.rank, rank_.thread, enter_interactive, enter_shutdown, shutdown_mode);

        // Handle shutdown (all threads/ranks)
         if (enter_shutdown) {
            sim_->setEndSim();
            #if 1
            ic_barrier_.wait();
            rankSync_->clearFlags();
            #endif
        }
        // Handle interactive console (only thread 0 on each rank)
        else { //if ( rank_.thread == 0 ) {

            // std::cout << "skk: syncmgr rank: t0: interactive execute\n";
            if (enter_interactive == true) {
                //if ( sim_->interactive_ != nullptr ) sim_->interactive_->execute(sim_->interactive_msg_);
                //printf("3: Rank%d, Thread%d: Flags: enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
                //    rank_.rank, rank_.thread, enter_interactive, enter_shutdown, shutdown_mode);
                partitionInfo();

                #if 1
                // SKK Test Producer/Consumer
                //testProducerConsumer();
                //sim_->interactive_->rankParallelExecute();
                //Output::getDefaultObject().output("R%d, T%d: Before execute \n", rank_.rank, rank_.thread);
                sim_->interactive_->execute(sim_->interactive_msg_);
                
                //RankExecBarrier_[0].wait();
                //Output::getDefaultObject().output("R%d, T%d: After execute \n", rank_.rank, rank_.thread);
                //sim_->setEndSim();
                #else
                //if (rank_.rank == 0) {  // SKK Temporary to test passing of watchpoint triggers
                if (rank_.rank == 0 && rank_.thread == 0) { 
                    //sim_->interactive_->execute();
                    handleInteractiveConsole();  // SKK Hack to work until manager/worker implemented
                    //rankHandleInteractiveConsole(); // won't work with MPIALLREDUCE bc only R0 calls
                }
                RankExecBarrier_[0].wait();  // SKK Sync result of IC
                #endif
                sim_->enter_interactive_ = false; // IC may schedule IC again
                
            }
            //printf("4: Rank%d, Thread%d: Flags: enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
            //        rank_.rank, rank_.thread, enter_interactive, enter_shutdown, shutdown_mode);
            rankSync_->clearFlags();
            RankExecBarrier_[0].wait();  // SKK Sync clear 
            #if 0
            // Test Rank Handle Shutdown Communication
            testRankFlagExchange();
            #endif
        }

#endif

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

#if 1  // Move exchange of enter_interactive, shutdown, and checkpoint flags here, similar to getSignals
        // Note that only thread 0 receives signals so it is the only one to execute above
        // However, any thread can trigger interactive or shutdown, so need to have all threads store
        // That is also why the setFlags must be atomic
       
        if (num_ranks_.rank == 1) {  
            // Get local sim flags
            getSimFlags(enter_interactive, enter_shutdown, shutdown_mode, generate_ckpt);
            // Each thread atomically sets shared flags in threadSync
            threadSync_->setFlags(enter_interactive, enter_shutdown, shutdown_mode);
        }

#endif
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

            threadSync_->getFlags(enter_interactive, enter_shutdown, shutdown_mode);
            //printf("After threadSync_->getFlags: enter_interactive %d, enter_shutdown %d, shutdown_mode %d \n",
            //    enter_interactive, enter_shutdown, shutdown_mode);
            if (enter_shutdown) {
                sim_->setEndSim();
                ic_barrier_.wait();
                threadSync_->clearFlags();
            }
            else if (enter_interactive) {
                //handleInteractiveConsole(); // Check of any thread set interactive console
                sim_->interactive_->summary();
                sim_->interactive_->execute(sim_->interactive_msg_);
                //Output::getDefaultObject().output(" R%d, T%d: after interactive enter_interactive %d, enter_shutdown %d, shutdown_mode %d\n", 
                    //rank_.rank, rank_.thread, enter_interactive, enter_shutdown, shutdown_mode);
                //ic_barrier_.wait();
                //handleShutdown();
                //ic_barrier_.wait();
                //threadSync_->clearFlags();
            }
            //threadSync_->clearFlags(); // SKK May currently be duplicated at end of handleIC for ic-start=0 case 
            //printf("After threadSync_->clearFlags: enter_interactive %d, enter_shutdown %d, shutdown_mode %d \n",
            //    enter_interactive, enter_shutdown, shutdown_mode);


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
    if ( !profile_tools_ ) profile_tools_ = new SyncProfileToolList();
    profile_tools_->addProfileTool(tool);
}

std::atomic<unsigned>     SyncManager::ckpt_generate_ { 0 };
std::atomic<int>          SyncManager::current_ic_thread_ { 0 };
std::atomic<int>          SyncManager::current_ic_state_ { 0 };
std::atomic<unsigned>     SyncManager::endSim_ { false };
Core::ThreadSafe::Barrier SyncManager::ic_barrier_;


} // namespace SST
