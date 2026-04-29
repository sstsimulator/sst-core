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

#ifndef SST_CORE_SYNC_THREADSYNCDIRECTSKIP_H
#define SST_CORE_SYNC_THREADSYNCDIRECTSKIP_H

#include "sst/core/action.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_types.h"
#include "sst/core/sync/syncManager.h"
#include "sst/core/sync/syncQueue.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace SST {

class ActivityQueue;
class Link;
class TimeConverter;
class Exit;
class Event;
class Simulation_impl;
class ThreadSyncQueue;

class ThreadSyncDirectSkip : public ThreadSync
{
public:
    /** Create a new ThreadSync object */
    ThreadSyncDirectSkip(int num_threads, int thread, Simulation_impl* sim);
    ThreadSyncDirectSkip() {} // For serialization only
    ~ThreadSyncDirectSkip();

    void setMaxPeriod(TimeConverter* period);

    void before() override {}
    void after() override;
    void execute() override;

    /** Cause an exchange of Untimed Data to occur */
    void processLinkUntimedData() override {}
    /** Finish link configuration */
    void finalizeLinkConfigurations() override {}
    void prepareForComplete() override {}

    /** Set signals to exchange during sync */
    void setSignals(int end, int usr, int alrm) override;
    /** Return exchanged signals after sync */
    bool getSignals(int& end, int& usr, int& alrm) override;

    /** Set interactive flags to exchange during sync */
    void setShutdownFlags(bool enter_shutdown, Simulation_impl::ShutdownMode_t shutdown_mode) override;
    void setFlags(bool enter_interactive, bool enter_shutdown, Simulation_impl::ShutdownMode_t shutdown_mode) override;
    /** Return exchanged interactive flags after sync */
    void getShutdownFlags(bool& enter_shutdown, Simulation_impl::ShutdownMode_t& shutdown_mode) override;
    void getFlags(
        bool& enter_interactive, bool& enter_shutdown, Simulation_impl::ShutdownMode_t& shutdown_mode) override;
    /** Clear interactive flags before next run */
    void clearFlags() override;

    SimTime_t getNextSyncTime() override { return nextSyncTime - 1; }

    /** Register a Link which this Sync Object is responsible for */
    void           registerLink(Link* UNUSED(link)) override {}
    ActivityQueue* registerRemoteLink(int UNUSED(id), Link* UNUSED(link)) override { return nullptr; }

    uint64_t getDataSize() const;

private:
    SimTime_t                        my_max_period;
    int                              num_threads;
    int                              thread;
    static SimTime_t                 localMinimumNextActivityTime;
    Simulation_impl*                 sim;
    static Core::ThreadSafe::Barrier barrier[3];
    double                           totalWaitTime;
    bool                             single_rank;
    static int                       sig_end_;
    static int                       sig_usr_;
    static int                       sig_alrm_;
    static std::atomic<bool>         enter_interactive_;
    static std::atomic<bool>         enter_shutdown_;
    static std::atomic<unsigned>     shutdown_mode_;
};


} // namespace SST

#endif // SST_CORE_SYNC_THREADSYNCDIRECTSKIP_H
