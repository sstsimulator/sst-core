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

#include "sst/core/realtime.h"

#include "sst/core/cputimer.h"
#include "sst/core/mempoolAccessor.h"
#include "sst/core/output.h"
#include "sst/core/realtimeAction.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/stringize.h"
#include "sst/core/timeLord.h"
#include "sst/core/unitAlgebra.h"

#include <unistd.h>

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#define UNUSED_WO_MPI(x) x
#else
#define UNUSED_WO_MPI(x) UNUSED(x)
#endif


namespace SST {

/************ Static RealTimeManager Functions ***********/
/************ Executed by thread 0 ONLY ******************/
void
RealTimeManager::SimulationSigEndHandler(int sig)
{
    sig_end_from_os_ = sig;
    Simulation_impl::notifySignal(); // Notify monitor

    signal(sig, SIG_DFL); // Restore default handler - double sigint/term will kill the process
}

void
RealTimeManager::SimulationSigUsrHandler(int sig)
{
    sig_usr_from_os_ = sig;
    Simulation_impl::notifySignal(); // Notify monitor
}

void
RealTimeManager::SimulationSigAlrmHandler(int sig)
{
    sig_alrm_from_os_ = sig;
    Simulation_impl::notifySignal(); // Notify monitor
}

void
RealTimeManager::installSignalHandlers()
{
    Output out = Simulation_impl::getSimulationOutput();
    if ( SIG_ERR == signal(SIGUSR1, RealTimeManager::SimulationSigUsrHandler) ) {
        out.fatal(CALL_INFO, 1, "Installation of SIGUSR1 signal handler failed.\n");
    }
    if ( SIG_ERR == signal(SIGUSR2, RealTimeManager::SimulationSigUsrHandler) ) {
        out.fatal(CALL_INFO, 1, "Installation of SIGUSR2 signal handler failed.\n");
    }
    if ( SIG_ERR == signal(SIGINT, RealTimeManager::SimulationSigEndHandler) ) {
        out.fatal(CALL_INFO, 1, "Installation of SIGINT signal handler failed.\n");
    }
    if ( SIG_ERR == signal(SIGTERM, RealTimeManager::SimulationSigEndHandler) ) {
        out.fatal(CALL_INFO, 1, "Installation of SIGTERM signal handler failed.\n");
    }
    if ( SIG_ERR == signal(SIGALRM, RealTimeManager::SimulationSigAlrmHandler) ) {
        out.fatal(CALL_INFO, 1, "Installation of SIGALRM signal handler failed.\n");
    }

    out.verbose(CALL_INFO, 1, 0, "Signal handler registration is completed\n");
}


/************ RealTimeAction ***********/

SST_ELI_DEFINE_INFO_EXTERN(RealTimeAction)
SST_ELI_DEFINE_CTOR_EXTERN(RealTimeAction)


RealTimeAction::RealTimeAction() {}

UnitAlgebra
RealTimeAction::getCoreTimeBase() const
{
    return Simulation_impl::getSimulation()->getTimeLord()->getTimeBase();
}

SimTime_t
RealTimeAction::getCurrentSimCycle() const
{
    return Simulation_impl::getSimulation()->getCurrentSimCycle();
}

UnitAlgebra
RealTimeAction::getElapsedSimTime() const
{
    return Simulation_impl::getSimulation()->getElapsedSimTime();
}

SimTime_t
RealTimeAction::getEndSimCycle() const
{
    return Simulation_impl::getSimulation()->getEndSimCycle();
}

UnitAlgebra
RealTimeAction::getEndSimTime() const
{
    return Simulation_impl::getSimulation()->getEndSimTime();
}

RankInfo
RealTimeAction::getRank() const
{
    return Simulation_impl::getSimulation()->getRank();
}

RankInfo
RealTimeAction::getNumRanks() const
{
    return Simulation_impl::getSimulation()->getNumRanks();
}

Output&
RealTimeAction::getSimulationOutput() const
{
    return Simulation_impl::getSimulation()->getSimulationOutput();
}

uint64_t
RealTimeAction::getTimeVortexMaxDepth() const
{
    return Simulation_impl::getSimulation()->getTimeVortexMaxDepth();
}

void
RealTimeAction::getMemPoolUsage(int64_t& bytes, int64_t& active_entries)
{
    Core::MemPoolAccessor::getMemPoolUsage(bytes, active_entries);
}

uint64_t
RealTimeAction::getSyncQueueDataSize() const
{
    return Simulation_impl::getSimulation()->getSyncQueueDataSize();
}

void
RealTimeAction::simulationPrintStatus(bool component_status)
{
    Simulation_impl::getSimulation()->printStatus(component_status);
}

void
RealTimeAction::simulationSignalShutdown(bool abnormal)
{
    Simulation_impl::getSimulation()->signalShutdown(abnormal);
}

void
RealTimeAction::simulationCheckpoint()
{
    Simulation_impl::getSimulation()->scheduleCheckpoint();
}

void
RealTimeAction::initiateInteractive(const std::string& msg)
{
    Simulation_impl* sim    = Simulation_impl::getSimulation();
    sim->enter_interactive_ = true;
    sim->interactive_msg_   = msg;
}


/************ ExitCleanRealTimeAction ***********/

ExitCleanRealTimeAction::ExitCleanRealTimeAction() : RealTimeAction() {}

void
ExitCleanRealTimeAction::execute()
{
    Output   sim_output = getSimulationOutput();
    RankInfo rank       = getRank();

    sim_output.output("EXIT-AFTER TIME REACHED; SHUTDOWN (%u,%u)!\n", rank.rank, rank.thread);
    sim_output.output("# Simulated time:                  %s\n", getElapsedSimTime().toStringBestSI().c_str());

    simulationSignalShutdown(false); /* Not an abnormal shutdown */
}

void
ExitCleanRealTimeAction::begin(time_t scheduled_time)
{
    Output sim_output = getSimulationOutput();
    if ( scheduled_time == 0 ) { return; }

    struct tm* end = localtime(&scheduled_time);
    sim_output.verbose(
        CALL_INFO, 1, 0, "# Will end by: %04u/%02u/%02u at: %02u:%02u:%02u\n", (end->tm_year + 1900), (end->tm_mon + 1),
        end->tm_mday, end->tm_hour, end->tm_min, end->tm_sec);
}


/************ ExitEmergencyRealTimeAction ***********/

ExitEmergencyRealTimeAction::ExitEmergencyRealTimeAction() : RealTimeAction() {}

void
ExitEmergencyRealTimeAction::execute()
{
    Output   sim_output = getSimulationOutput();
    RankInfo rank       = getRank();

    sim_output.output("EMERGENCY SHUTDOWN (%u,%u)!\n", rank.rank, rank.thread);
    sim_output.output("# Simulated time:                  %s\n", getElapsedSimTime().toStringBestSI().c_str());
    simulationSignalShutdown(true);
}


/************ CoreStatusRealTimeAction ***********/

CoreStatusRealTimeAction::CoreStatusRealTimeAction() : RealTimeAction() {}

void
CoreStatusRealTimeAction::execute()
{
    simulationPrintStatus(false);
}


/************ ComponentStatusRealTimeAction ***********/

ComponentStatusRealTimeAction::ComponentStatusRealTimeAction() : RealTimeAction() {}

void
ComponentStatusRealTimeAction::execute()
{
    simulationPrintStatus(true);
}


/************ CheckpointRealTimeAction ***********/

CheckpointRealTimeAction::CheckpointRealTimeAction() : RealTimeAction() {}

void
CheckpointRealTimeAction::execute()
{
    Output   sim_output = getSimulationOutput();
    RankInfo rank       = getRank();

    sim_output.output(
        "Creating checkpoint at simulated time %s (rank=%u,thread=%u).\n", getElapsedSimTime().toStringBestSI().c_str(),
        rank.rank, rank.thread);
    simulationCheckpoint();
}

void
CheckpointRealTimeAction::begin(time_t scheduled_time)
{
    Output sim_output = getSimulationOutput();

    struct tm* end = localtime(&scheduled_time);
    sim_output.verbose(
        CALL_INFO, 1, 0, "# First checkpoint will occur around: %04u/%02u/%02u at %02u:%02u:%02u\n",
        (end->tm_year + 1900), (end->tm_mon + 1), end->tm_mday, end->tm_hour, end->tm_min, end->tm_sec);
}


/************ HeartbeatRealTimeAction ***********/

HeartbeatRealTimeAction::HeartbeatRealTimeAction() : RealTimeAction()
{
    RankInfo rank = getRank();
    last_time_    = 0;
    if ( getRank().thread == 0 ) { exchange_barrier_.resize(getNumRanks().thread); }
}

void
HeartbeatRealTimeAction::execute()
{
    Output&  sim_output = getSimulationOutput();
    RankInfo rank       = getRank();
    RankInfo num_ranks  = getNumRanks();

    int64_t mempool_size      = 0;
    int64_t active_activities = 0;

    if ( 0 == rank.thread ) {
        if ( 0 == rank.rank ) {
            const double now = sst_get_cpu_time();
            sim_output.output(
                "# Simulation Heartbeat: Simulated Time %s (Real CPU time since last period %.5f seconds)\n",
                getElapsedSimTime().toStringBestSI().c_str(), (now - last_time_));

            last_time_ = now;
        }
        thr_max_tv_depth_.store(getTimeVortexMaxDepth());
        getMemPoolUsage(mempool_size, active_activities);
    }

    if ( num_ranks.thread > 1 ) {
        exchange_barrier_.wait(); // ensure thr_max_tv_depth is initialized

        // Compute local max
        if ( 0 != rank.thread ) { Core::ThreadSafe::atomic_fetch_max(thr_max_tv_depth_, getTimeVortexMaxDepth()); }
        exchange_barrier_.wait(); // ensure updates to thr_max_tv_depth are done
    }

    // Print some resource usage
    uint64_t global_max_tv_depth          = 0;
    uint64_t global_max_sync_data_size    = 0; // Only relevant for MPI
    uint64_t global_sum_sync_data_size    = 0; // Only relevant for MPI
    int64_t  global_sum_mempool_size      = 0;
    int64_t  global_max_mempool_size      = 0;
    int64_t  global_sum_active_activities = 0;

    if ( rank.thread == 0 ) {
        if ( num_ranks.rank > 1 ) { /* MPI */
#ifdef SST_CONFIG_HAVE_MPI
            uint64_t local_sync_data_size = getSyncQueueDataSize();
            MPI_Allreduce(&thr_max_tv_depth_, &global_max_tv_depth, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
            MPI_Allreduce(&local_sync_data_size, &global_max_sync_data_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
            MPI_Allreduce(&local_sync_data_size, &global_sum_sync_data_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
            MPI_Allreduce(&mempool_size, &global_max_mempool_size, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);
            MPI_Allreduce(&mempool_size, &global_sum_mempool_size, 1, MPI_INT64_T, MPI_SUM, MPI_COMM_WORLD);
            MPI_Allreduce(&active_activities, &global_sum_active_activities, 1, MPI_INT64_T, MPI_SUM, MPI_COMM_WORLD);
#endif
        }
        else { /* Serial or threaded-only */
            global_max_tv_depth          = thr_max_tv_depth_;
            global_max_sync_data_size    = 0;
            global_sum_sync_data_size    = 0;
            global_max_mempool_size      = mempool_size;
            global_sum_mempool_size      = mempool_size;
            global_sum_active_activities = active_activities;
        }

        if ( rank.rank == 0 ) {
            std::string ua_str;

            ua_str = format_string("%" PRIu64 "B", global_max_sync_data_size);
            UnitAlgebra global_max_sync_data_size_ua(ua_str);

            ua_str = format_string("%" PRIu64 "B", global_sum_sync_data_size);
            UnitAlgebra global_sync_data_size_ua(ua_str);

            ua_str = format_string("%" PRIu64 "B", global_max_mempool_size);
            UnitAlgebra global_max_mempool_size_ua(ua_str);

            ua_str = format_string("%" PRIu64 "B", global_sum_mempool_size);
            UnitAlgebra global_sum_mempool_size_ua(ua_str);

            sim_output.output(
                "\tMax mempool usage:               %s\n", global_max_mempool_size_ua.toStringBestSI().c_str());
            sim_output.output(
                "\tGlobal mempool usage:            %s\n", global_sum_mempool_size_ua.toStringBestSI().c_str());
            sim_output.output(
                "\tGlobal active activities         %" PRIu64 " activities\n", global_sum_active_activities);
            sim_output.output("\tMax TimeVortex depth:            %" PRIu64 " entries\n", global_max_tv_depth);
            if ( num_ranks.rank > 1 ) {
                sim_output.output(
                    "\tMax Sync data size:              %s\n", global_max_sync_data_size_ua.toStringBestSI().c_str());
                sim_output.output(
                    "\tGlobal Sync data size:           %s\n", global_sync_data_size_ua.toStringBestSI().c_str());
            }
        }
    }
}

void
HeartbeatRealTimeAction::begin(time_t UNUSED(scheduled_time))
{
    last_time_ = sst_get_cpu_time();
}

std::atomic<uint64_t>     HeartbeatRealTimeAction::thr_max_tv_depth_(0);
Core::ThreadSafe::Barrier HeartbeatRealTimeAction::exchange_barrier_;


/************ RealTimeIntervalAction ***********/

RealTimeIntervalAction::RealTimeIntervalAction(uint32_t interval, RealTimeAction* action)
{
    alarm_interval_ = next_alarm_time_ = interval;
    action_                            = action;
}

void
RealTimeIntervalAction::begin(time_t begin_time)
{
    action_->begin(begin_time + alarm_interval_);
}

uint32_t
RealTimeIntervalAction::getNextAlarmTime() const
{
    return next_alarm_time_;
}

void
RealTimeIntervalAction::execute(uint32_t elapsed)
{
    /* Update alarm & execute action if needed */
    if ( next_alarm_time_ <= elapsed ) {
        next_alarm_time_ = alarm_interval_;
        action_->execute();
    }
    else {
        next_alarm_time_ = next_alarm_time_ - elapsed;
    }
}


/************ AlrmSignalAction ***********/

AlrmSignalAction::AlrmSignalAction() : RealTimeAction()
{
    RankInfo num_ranks = getNumRanks();
    RankInfo rank      = getRank();
    alarm_manager_     = rank.rank == 0 && rank.thread == 0;
    rank_leader_       = (num_ranks.rank > 1) && (rank.thread == 0);
    if ( rank.thread == 0 ) { exchange_barrier_.resize(num_ranks.thread); }
}

void
AlrmSignalAction::addIntervalAction(uint32_t interval, RealTimeAction* action)
{
    RealTimeIntervalAction interval_action(interval, action);
    interval_actions_.push_back(interval_action);
}

void
AlrmSignalAction::begin(time_t UNUSED(scheduled_time))
{
    RankInfo num_ranks = getNumRanks();

    /* Rank 0/thread 0 manages the alarm */
    if ( alarm_manager_ ) {
        last_time_ = time(nullptr);
        if ( interval_actions_.size() != 0 ) {

            uint32_t next_alarm_time = interval_actions_[0].getNextAlarmTime();
            interval_actions_[0].begin(last_time_);

            for ( size_t i = 1; i < interval_actions_.size(); i++ ) {
                interval_actions_[i].begin(last_time_);
                uint32_t t = interval_actions_[i].getNextAlarmTime();
                if ( t < next_alarm_time ) { next_alarm_time = t; }
            }

            alarm(next_alarm_time);
        }
    }

    /* Exchange last_time_ so that alarm triggers agree */
    if ( rank_leader_ ) {
#ifdef SST_CONFIG_HAVE_MPI
        // Broadcast elapsed time
        MPI_Bcast(&last_time_, sizeof(last_time_), MPI_BYTE, 0, MPI_COMM_WORLD);
#endif
    }
    if ( num_ranks.thread > 1 ) {
        // Share elapsed time - barrier
        exchange_barrier_.wait();
    }
}

void
AlrmSignalAction::execute()
{
    RankInfo num_ranks       = getNumRanks();
    uint32_t next_alarm_time = 0;

    if ( alarm_manager_ ) {
        /* Manage the alarm */
        time_t the_time = time(nullptr);
        elapsed_        = the_time - last_time_;
    }

    if ( rank_leader_ ) {
#ifdef SST_CONFIG_HAVE_MPI
        // Broadcast elapsed time
        MPI_Bcast(&elapsed_, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
#endif
    }
    if ( num_ranks.thread > 1 ) {
        // Share elapsed time - barrier
        exchange_barrier_.wait();
    }
    // All AlrmSignalAction objects have the same elapsed_ value now

    for ( size_t i = 0; i < interval_actions_.size(); i++ ) {
        interval_actions_[i].execute(elapsed_);

        if ( alarm_manager_ ) {
            if ( i == 0 ) { next_alarm_time = interval_actions_[i].getNextAlarmTime(); }
            else {
                uint32_t t = interval_actions_[i].getNextAlarmTime();
                if ( t < next_alarm_time ) { next_alarm_time = t; }
            }
        }
    }
    last_time_ += elapsed_;

    if ( alarm_manager_ && next_alarm_time != 0 ) { alarm(next_alarm_time); }
}

uint32_t                  AlrmSignalAction::elapsed_ = 0;
Core::ThreadSafe::Barrier AlrmSignalAction::exchange_barrier_;


/************ RealTimeManager ***********/

RealTimeManager::RealTimeManager(RankInfo num_ranks)
{
    serial_exec_ = (num_ranks.rank == 1) && (num_ranks.thread == 1);
    sig_end_     = 0;
    sig_usr_     = 0;
    sig_alrm_    = 0;
}

RealTimeManager::RealTimeManager()
{
    sig_end_  = 0;
    sig_usr_  = 0;
    sig_alrm_ = 0;
}

void
RealTimeManager::registerSignal(RealTimeAction* action, int signum)
{
    signal_actions_.insert(std::make_pair(signum, action));
    if ( action->canInitiateCheckpoint() ) can_checkpoint_ = true;
}

void
RealTimeManager::registerInterval(uint32_t interval, RealTimeAction* action)
{
    if ( signal_actions_.find(SIGALRM) == signal_actions_.end() ) {
        AlrmSignalAction* alrm = new AlrmSignalAction();
        signal_actions_.insert(std::make_pair(SIGALRM, alrm));
    }

    static_cast<AlrmSignalAction*>(signal_actions_[SIGALRM])->addIntervalAction(interval, action);
    if ( action->canInitiateCheckpoint() ) can_checkpoint_ = true;
}

void
RealTimeManager::begin()
{
    if ( signal_actions_.find(SIGALRM) != signal_actions_.end() ) { signal_actions_[SIGALRM]->begin(0); }
}

/* Races are possible if signals come in while this function is executing.
 * -> Race with SIGALRM doesn't happen (only one outstanding at a time)
 * -> Race with SIGINT/TERM doesn't matter since we only need one
 * -> Race with SIGUSR1/2 might cause a lost signal but best-effort anyways
 */
void
RealTimeManager::notifySignal()
{
    /* In OpenMPI, ORTE will automatically propagate certain signals received
     * by mpirun to all ranks. Since we wait until the next sync to handle the
     * signal, we might handle the same signal multiple times if different ranks
     * receive the propagated signals between different sync points. Avoiding is hard.
     * Ignoring the signal on all ranks except 0 can instead lead to missing signals
     * sent directly to other ranks.
     */

    // Don't care if we overwrite
    if ( sig_end_from_os_ ) {
        sig_end_         = sig_end_from_os_; // Don't care if we overwrite
        sig_end_from_os_ = 0;
        if ( serial_exec_ ) {
            signal_actions_[sig_end_]->execute();
            sig_end_ = 0; // Reset but doesn't matter since simulation is ending
        }
    }

    if ( sig_usr_from_os_ ) {
        sig_usr_         = sig_usr_from_os_;
        sig_usr_from_os_ = 0;
        if ( serial_exec_ ) {
            signal_actions_[sig_usr_]->execute();
            sig_usr_ = 0; // Reset
        }
    }

    if ( sig_alrm_from_os_ ) {
        sig_alrm_from_os_ = 0;
        if ( serial_exec_ ) { signal_actions_[SIGALRM]->execute(); }
        else {
            sig_alrm_ = SIGALRM;
        }
    }

    // Ignore signals that have no registered handler - should be impossible to receive anyways
    // For parallel runs, we delay handling until the next sync
}

bool
RealTimeManager::getSignals(int& sig_end, int& sig_usr, int& sig_alrm)
{
    sig_end   = sig_end_;
    sig_usr   = sig_usr_;
    sig_alrm  = sig_alrm_;
    sig_end_  = 0;
    sig_usr_  = 0;
    sig_alrm_ = 0;
    return sig_end || sig_usr || sig_alrm;
}

void
RealTimeManager::performSignal(int signum)
{
    /* If we have intercepted a signal then it exists in signal_actions_ */
    signal_actions_[signum]->execute();
}

void
RealTimeManager::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(serial_exec_);
}

sig_atomic_t RealTimeManager::sig_end_from_os_  = 0;
sig_atomic_t RealTimeManager::sig_usr_from_os_  = 0;
sig_atomic_t RealTimeManager::sig_alrm_from_os_ = 0;

} /* End namespace SST */
