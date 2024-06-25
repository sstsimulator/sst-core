// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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

class Exit;
class Simulation_impl;
// class SyncBase;
class ThreadSyncQueue;
class TimeConverter;

class SyncProfileToolList;
namespace Profile {
class SyncProfileTool;
}

class RankSync : public SST::Core::Serialization::serializable
{
public:
    RankSync(RankInfo num_ranks) : num_ranks(num_ranks) { link_maps.resize(num_ranks.rank); }
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

    virtual SimTime_t getNextSyncTime() { return nextSyncTime; }

    // void setMaxPeriod(TimeConverter* period) {max_period = period;}
    TimeConverter* getMaxPeriod() { return max_period; }

    virtual uint64_t getDataSize() const = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& nextSyncTime;
        ser& max_period; // Unused
        // ser& num_ranks; // const so a pain to serialize but don't need it
        ser& link_maps;
    }
    ImplementVirtualSerializable(SST::RankSync) protected : SimTime_t nextSyncTime;
    TimeConverter* max_period;
    const RankInfo num_ranks;

    std::vector<std::map<std::string, uintptr_t>> link_maps;

    void finalizeConfiguration(Link* link) { link->finalizeConfiguration(); }

    void prepareForCompleteInt(Link* link) { link->prepareForComplete(); }

    void sendUntimedData_sync(Link* link, Event* data) { link->sendUntimedData_sync(data); }

    inline void setLinkDeliveryInfo(Link* link, uintptr_t info) { link->pair_link->setDeliveryInfo(info); }

    inline Link* getDeliveryLink(Event* ev) { return ev->getDeliveryLink(); }

private:
};

class ThreadSync : public SST::Core::Serialization::serializable
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

    virtual SimTime_t getNextSyncTime() { return nextSyncTime; }

    void           setMaxPeriod(TimeConverter* period) { max_period = period; }
    TimeConverter* getMaxPeriod() { return max_period; }

    /** Register a Link which this Sync Object is responsible for */
    virtual void           registerLink(const std::string& name, Link* link)                = 0;
    virtual ActivityQueue* registerRemoteLink(int tid, const std::string& name, Link* link) = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& nextSyncTime;
        ser& max_period; // Unused
    }
    ImplementVirtualSerializable(SST::ThreadSync)

        protected :

        SimTime_t nextSyncTime;
    TimeConverter* max_period;

    void finalizeConfiguration(Link* link) { link->finalizeConfiguration(); }

    void prepareForCompleteInt(Link* link) { link->prepareForComplete(); }

    void sendUntimedData_sync(Link* link, Event* data) { link->sendUntimedData_sync(data); }

    inline void setLinkDeliveryInfo(Link* link, uintptr_t info) { link->pair_link->setDeliveryInfo(info); }

    inline Link* getDeliveryLink(Event* ev) { return ev->getDeliveryLink(); }

private:
};

class SyncManager : public Action
{
public:
    SyncManager(
        const RankInfo& rank, const RankInfo& num_ranks, TimeConverter* minPartTC, SimTime_t min_part,
        const std::vector<SimTime_t>& interThreadLatencies);
    SyncManager(); // For serialization only
    virtual ~SyncManager();

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue*
         registerLink(const RankInfo& to_rank, const RankInfo& from_rank, const std::string& name, Link* link);
    void exchangeLinkInfo();
    void execute(void) override;

    /** Cause an exchange of Initialization Data to occur */
    void exchangeLinkUntimedData(std::atomic<int>& msg_count);
    /** Finish link configuration */
    void finalizeLinkConfigurations();
    void prepareForComplete();

    void print(const std::string& header, Output& out) const override;

    uint64_t getDataSize() const;

    void addProfileTool(Profile::SyncProfileTool* tool);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::SyncManager)
private:
    enum sync_type_t { RANK, THREAD };

    RankInfo                         rank;
    RankInfo                         num_ranks;
    static Core::ThreadSafe::Barrier RankExecBarrier[6];
    static Core::ThreadSafe::Barrier LinkUntimedBarrier[3];
    // static SimTime_t min_next_time;
    // static int min_count;

    static RankSync* rankSync;
    static SimTime_t next_rankSync;
    ThreadSync*      threadSync;
    Exit*            exit;
    Simulation_impl* sim;

    sync_type_t next_sync_type;
    SimTime_t   min_part;

    SyncProfileToolList* profile_tools = nullptr;

    void computeNextInsert();
};

} // namespace SST

#endif // SST_CORE_SYNC_SYNCMANAGER_H
