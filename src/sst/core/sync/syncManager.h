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

#ifndef SST_CORE_SYNC_SYNCMANAGER_H
#define SST_CORE_SYNC_SYNCMANAGER_H

#include "sst/core/action.h"
#include "sst/core/link.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/threadsafe.h"

#include <unordered_map>
#include <vector>

namespace SST {

class CheckpointAction;
class Exit;
class RealTimeManager;
class Simulation_impl;
// class SyncBase;
class ThreadSyncQueue;
class TimeConverter;

class SyncProfileToolList;
namespace Profile {
class SyncProfileTool;
}

class RankSync
{
public:
    RankSync(RankInfo num_ranks) : num_ranks_(num_ranks) { link_maps.resize(num_ranks_.rank); }
    RankSync() : max_period(nullptr) {}
    virtual ~RankSync() {}

    /** Register a Link which this Sync Object is responsible for */
    virtual ActivityQueue*
         registerLink(const RankInfo& to_rank, const RankInfo& from_rank, const std::string& name, Link* link) = 0;
    void exchangeLinkInfo(uint32_t my_rank);

    virtual void execute(int thread)                                              = 0;
    virtual void exchangeLinkUntimedData(int thread, std::atomic<int>& msg_count) = 0;
    virtual void finalizeLinkConfigurations()                                     = 0;
    virtual void prepareForComplete()                                             = 0;

    /** Set signals to exchange during sync */
    virtual void setSignals(int end, int usr, int alrm)    = 0;
    /** Return exchanged signals after sync */
    virtual bool getSignals(int& end, int& usr, int& alrm) = 0;

    virtual SimTime_t getNextSyncTime() { return nextSyncTime; }

    virtual void setRestartTime(SimTime_t time) { nextSyncTime = time; }

    TimeConverter* getMaxPeriod() { return max_period; }

    virtual uint64_t getDataSize() const = 0;

protected:
    SimTime_t      nextSyncTime;
    TimeConverter* max_period;
    const RankInfo num_ranks_;

    std::vector<std::map<std::string, uintptr_t>> link_maps;

    void finalizeConfiguration(Link* link) { link->finalizeConfiguration(); }

    void prepareForCompleteInt(Link* link) { link->prepareForComplete(); }

    void sendUntimedData_sync(Link* link, Event* data) { link->sendUntimedData_sync(data); }

    inline void setLinkDeliveryInfo(Link* link, uintptr_t info) { link->pair_link->setDeliveryInfo(info); }

    inline Link* getDeliveryLink(Event* ev) { return ev->getDeliveryLink(); }
};

class ThreadSync
{
public:
    ThreadSync() : max_period(nullptr) {}
    virtual ~ThreadSync() {}

    virtual void before()                     = 0;
    virtual void after()                      = 0;
    virtual void execute()                    = 0;
    virtual void processLinkUntimedData()     = 0;
    virtual void finalizeLinkConfigurations() = 0;
    virtual void prepareForComplete()         = 0;

    /** Set signals to exchange during sync */
    virtual void setSignals(int end, int usr, int alrm)    = 0;
    /** Return exchanged signals after sync */
    virtual bool getSignals(int& end, int& usr, int& alrm) = 0;

    virtual SimTime_t getNextSyncTime() { return nextSyncTime; }
    virtual void      setRestartTime(SimTime_t time) { nextSyncTime = time; }

    void           setMaxPeriod(TimeConverter* period) { max_period = period; }
    TimeConverter* getMaxPeriod() { return max_period; }

    /** Register a Link which this Sync Object is responsible for */
    virtual void           registerLink(const std::string& name, Link* link)                = 0;
    virtual ActivityQueue* registerRemoteLink(int tid, const std::string& name, Link* link) = 0;

protected:
    SimTime_t      nextSyncTime;
    TimeConverter* max_period;

    void finalizeConfiguration(Link* link) { link->finalizeConfiguration(); }

    void prepareForCompleteInt(Link* link) { link->prepareForComplete(); }

    void sendUntimedData_sync(Link* link, Event* data) { link->sendUntimedData_sync(data); }

    inline void setLinkDeliveryInfo(Link* link, uintptr_t info) { link->pair_link->setDeliveryInfo(info); }

    inline Link* getDeliveryLink(Event* ev) { return ev->getDeliveryLink(); }
};

class SyncManager : public Action
{
public:
    SyncManager(
        const RankInfo& rank, const RankInfo& num_ranks, SimTime_t min_part,
        const std::vector<SimTime_t>& interThreadLatencies, RealTimeManager* real_time);
    SyncManager(); // For serialization only
    virtual ~SyncManager();

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue*
         registerLink(const RankInfo& to_rank, const RankInfo& from_rank, const std::string& name, Link* link);
    void exchangeLinkInfo();
    void execute() override;

    /** Cause an exchange of Initialization Data to occur */
    void exchangeLinkUntimedData(std::atomic<int>& msg_count);
    /** Finish link configuration */
    void finalizeLinkConfigurations();
    void prepareForComplete();

    void print(const std::string& header, Output& out) const override;

    uint64_t getDataSize() const;

    void setRestartTime(SimTime_t time)
    {
        rankSync_->setRestartTime(time);
        threadSync_->setRestartTime(time);
    }

    void addProfileTool(Profile::SyncProfileTool* tool);

    NotSerializable(SST::SyncManager)

private:
    // Enum to track the next sync type
    enum sync_type_t { RANK, THREAD };

    RankInfo                         rank_;
    RankInfo                         num_ranks_;
    static Core::ThreadSafe::Barrier RankExecBarrier_[5];
    static Core::ThreadSafe::Barrier LinkUntimedBarrier_[3];

    static RankSync* rankSync_;
    static SimTime_t next_rankSync_;
    ThreadSync*      threadSync_;
    Exit*            exit_;
    Simulation_impl* sim_;

    sync_type_t next_sync_type_;
    SimTime_t   min_part_;

    RealTimeManager*  real_time_;
    CheckpointAction* checkpoint_;

    SyncProfileToolList* profile_tools_ = nullptr;

    void computeNextInsert(SimTime_t next_checkpoint_time = MAX_SIMTIME_T);
    void setupSyncObjects();
};

} // namespace SST

#endif // SST_CORE_SYNC_SYNCMANAGER_H
