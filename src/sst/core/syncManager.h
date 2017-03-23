// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SYNCMANAGER_H
#define SST_CORE_SYNCMANAGER_H

#include "sst/core/sst_types.h"

#include "sst/core/action.h"
#include "sst/core/link.h"
#include "sst/core/rankInfo.h"
#include "sst/core/threadsafe.h"

#include <vector>
#include <unordered_map>

namespace SST {

class Exit;
class Simulation;
class SyncBase;
class ThreadSyncQueue;
class TimeConverter;

class NewRankSync {
public:
    NewRankSync() {}
    virtual ~NewRankSync() {}

    /** Register a Link which this Sync Object is responsible for */
    virtual ActivityQueue* registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link) = 0;

    virtual void execute(int thread) = 0;
    virtual void exchangeLinkInitData(int thread, std::atomic<int>& msg_count) = 0;
    virtual void finalizeLinkConfigurations() = 0;

    virtual SimTime_t getNextSyncTime() { return nextSyncTime; }

    // void setMaxPeriod(TimeConverter* period) {max_period = period;}
    TimeConverter* getMaxPeriod() {return max_period;}

    virtual uint64_t getDataSize() const = 0;
    
protected:
    SimTime_t nextSyncTime;
    TimeConverter* max_period; 

    void finalizeConfiguration(Link* link) {
        link->finalizeConfiguration();
    }
   
    void sendInitData_sync(Link* link, Event* init_data) {
        link->sendInitData_sync(init_data);
    }

private:
    
};

class NewThreadSync {
public:
    NewThreadSync () {}
    virtual ~NewThreadSync() {}

    virtual void before() = 0;
    virtual void after() = 0;
    virtual void execute() = 0;
    virtual void processLinkInitData() = 0;
    virtual void finalizeLinkConfigurations() = 0;

    virtual SimTime_t getNextSyncTime() { return nextSyncTime; }

    void setMaxPeriod(TimeConverter* period) {max_period = period;}
    TimeConverter* getMaxPeriod() {return max_period;}

    /** Register a Link which this Sync Object is responsible for */
    virtual void registerLink(LinkId_t link_id, Link* link) = 0;
    virtual ActivityQueue* getQueueForThread(int tid) = 0;
    
protected:
    SimTime_t nextSyncTime;
    TimeConverter* max_period;
    
    void finalizeConfiguration(Link* link) {
        link->finalizeConfiguration();
    }
   
    void sendInitData_sync(Link* link, Event* init_data) {
        link->sendInitData_sync(init_data);
    }

private:
};

class SyncManager : public Action {
public:
    SyncManager(const RankInfo& rank, const RankInfo& num_ranks, TimeConverter* minPartTC, SimTime_t min_part, const std::vector<SimTime_t>& interThreadLatencies);
    virtual ~SyncManager();

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link);
    void execute(void);

    /** Cause an exchange of Initialization Data to occur */
    void exchangeLinkInitData(std::atomic<int>& msg_count);
    /** Finish link configuration */
    void finalizeLinkConfigurations();

    void print(const std::string& header, Output &out) const;

private:
    enum sync_type_t { RANK, THREAD};

    RankInfo rank;
    RankInfo num_ranks;
    static Core::ThreadSafe::Barrier RankExecBarrier[6];
    static Core::ThreadSafe::Barrier LinkInitBarrier[3];
    // static SimTime_t min_next_time;
    // static int min_count;
    
    static NewRankSync*     rankSync;
    static SimTime_t        next_rankSync;
    NewThreadSync*   threadSync;
    Exit* exit;
    Simulation * sim;
    
    sync_type_t      next_sync_type;
    SimTime_t min_part;
    
    void computeNextInsert();
    
};


} // namespace SST

#endif // SST_CORE_SYNCMANAGER_H
