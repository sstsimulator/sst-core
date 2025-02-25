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
#include "sst/core/sst_types.h"
#include "sst/core/sync/syncManager.h"
#include "sst/core/sync/syncQueue.h"

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

    SimTime_t getNextSyncTime() override { return nextSyncTime - 1; }

    /** Register a Link which this Sync Object is responsible for */
    void           registerLink(const std::string& UNUSED(name), Link* UNUSED(link)) override {}
    ActivityQueue* registerRemoteLink(int UNUSED(id), const std::string& UNUSED(name), Link* UNUSED(link)) override
    {
        return nullptr;
    }

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
};


} // namespace SST

#endif // SST_CORE_SYNC_THREADSYNCDIRECTSKIP_H
