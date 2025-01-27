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

#ifndef SST_CORE_REAL_TIME_ALARM_MANAGER_H
#define SST_CORE_REAL_TIME_ALARM_MANAGER_H

#include "sst/core/realtimeAction.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/threadsafe.h"
#include "sst/core/warnmacros.h"

#include <map>
#include <set>
#include <signal.h>
#include <time.h>
#include <vector>

namespace SST {

class Output;
class RankInfo;
class UnitAlgebra;

/* Action cleanly exit simulation */
class ExitCleanRealTimeAction : public RealTimeAction
{
public:
    SST_ELI_REGISTER_REALTIMEACTION(
        ExitCleanRealTimeAction, "sst", "rt.exit.clean", SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Signal handler that causes an immediate, but non-emergency shutdown. This is the default action for the "
        "'--exit-after' option.")

    ExitCleanRealTimeAction();
    virtual void execute() override;
    virtual void begin(time_t scheduled_time) override;
};

/* Action to immediately exit simulation */
class ExitEmergencyRealTimeAction : public RealTimeAction
{
public:
    SST_ELI_REGISTER_REALTIMEACTION(
        ExitEmergencyRealTimeAction, "sst", "rt.exit.emergency", SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Signal handler that causes an emergency shutdown. This is the default action for SIGTERM and SIGINT.")

    ExitEmergencyRealTimeAction();
    virtual void execute() override;
};

/* Action to output core status */
class CoreStatusRealTimeAction : public RealTimeAction
{
public:
    SST_ELI_REGISTER_REALTIMEACTION(
        CoreStatusRealTimeAction, "sst", "rt.status.core", SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Signal handler that causes SST-Core to print its status. This is the default action for SIGUSR1.")

    CoreStatusRealTimeAction();
    void execute() override;
};

/* Action to output component status */
class ComponentStatusRealTimeAction : public RealTimeAction
{
public:
    SST_ELI_REGISTER_REALTIMEACTION(
        ComponentStatusRealTimeAction, "sst", "rt.status.all", SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Signal handler that causes SST-Core to print its status along with component status. This is the default "
        "action for SIGUSR2.")

    ComponentStatusRealTimeAction();
    void execute() override;
};

/* Action to trigger a checkpoint on a time interval */
class CheckpointRealTimeAction : public RealTimeAction
{
public:
    SST_ELI_REGISTER_REALTIMEACTION(
        CheckpointRealTimeAction, "sst", "rt.checkpoint", SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Signal handler that causes SST to generate a checkpoint. This is the default action for the "
        "'--checkpoint-wall-period' option.")

    CheckpointRealTimeAction();
    virtual void execute() override;
    virtual void begin(time_t scheduled_time) override;

    bool canInitiateCheckpoint() override { return true; }
};

/* Action to generate a heartbeat message */
class HeartbeatRealTimeAction : public RealTimeAction
{
public:
    SST_ELI_REGISTER_REALTIMEACTION(
        HeartbeatRealTimeAction, "sst", "rt.heartbeat", SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Signal handler that causes SST to generate a heartbeat message (status and some resource usage information). "
        "This is the default action for the '--heartbeat-wall-period' option.")

    HeartbeatRealTimeAction();
    virtual void execute() override;
    virtual void begin(time_t scheduled_time) override;

private:
    double                           last_time_;
    static std::atomic<uint64_t>     thr_max_tv_depth_;
    static Core::ThreadSafe::Barrier exchange_barrier_;
};

/* Wrapper for RealTimeActions that occur on a time interval */
class RealTimeIntervalAction
{
public:
    RealTimeIntervalAction(uint32_t interval, RealTimeAction* action);

    void begin(time_t begin_time);

    void     execute(uint32_t elapsed);
    uint32_t getNextAlarmTime() const;

private:
    uint32_t        alarm_interval_;  /* Interval to trigger alarm at (seconds) */
    uint32_t        next_alarm_time_; /* Next time an alarm should be triggered for this alarm */
    RealTimeAction* action_;          /* Action to take on when alarm triggers */
};

/* This class manages alarms but does not do any actions itself
 * All times are stored in seconds
 */
class AlrmSignalAction : public RealTimeAction
{
public:
    AlrmSignalAction();
    void         execute() override;
    void         addIntervalAction(uint32_t interval, RealTimeAction* action);
    virtual void begin(time_t scheduled_time) override; // Start alarms
private:
    std::vector<RealTimeIntervalAction> interval_actions_;
    bool                                alarm_manager_; /* The instance on thread 0/rank 0 is the manager */
    bool            rank_leader_; /* The instance on thread 0 of each rank participates in MPI exchanges */
    time_t          last_time_;   /* Last time a SIGALRM was received */
    static uint32_t elapsed_;     /* A static so that each threads' instance of this class share the same one */
    static Core::ThreadSafe::Barrier exchange_barrier_;
};

/** Class to manage real-time events (signals and alarms) */
class RealTimeManager : public SST::Core::Serialization::serializable
{
public:
    RealTimeManager(RankInfo num_ranks);
    RealTimeManager();

    /** Register actions */
    void registerSignal(RealTimeAction* action, int signum);
    void registerInterval(uint32_t interval, RealTimeAction* action);

    /** Begin monitoring signals */
    void begin();

    /** Simulation run loop calls this when a signal has been received
     * from the OS. One or more of the sig_X_from_os_ vars will be non-zero.
     *
     * Serial - this executes the relevant signal handler(s)
     * Parallel - this saves the signals until the next global sync
     */
    void notifySignal();

    /** This is a request to execute the handler in response to a particular signal */
    void performSignal(int signum);

    /* OS signal handling */
    static void installSignalHandlers();
    static void SimulationSigEndHandler(int sig);
    static void SimulationSigUsrHandler(int sig);
    static void SimulationSigAlrmHandler(int sig);

    /* SyncManager request to get signals. Also clears local signals */
    bool getSignals(int& sig_end, int& sig_usr, int& sig_alrm);

    /**
       Check whether or not any of the Actions registered with the
       manager can initiate a checkpoint.
     */
    bool canInitiateCheckpoint() { return can_checkpoint_; }

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::RealTimeManager)

private:
    bool serial_exec_;            // Whether execution is serial or parallel
    bool can_checkpoint_ = false; // Set to true if any Actions can trigger checkpoint

    /* The set of signal handlers for all signals */
    std::map<int, RealTimeAction*> signal_actions_;

    static sig_atomic_t sig_alrm_from_os_;
    static sig_atomic_t sig_usr_from_os_;
    static sig_atomic_t sig_end_from_os_;

    int sig_alrm_;
    int sig_usr_;
    int sig_end_;
};

} // namespace SST
#endif /* SST_CORE_REAL_TIME_ALARM_MANAGER_H */
