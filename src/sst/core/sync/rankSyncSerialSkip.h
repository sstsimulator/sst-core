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

#ifndef SST_CORE_SYNC_RANKSYNCSERIALSKIP_H
#define SST_CORE_SYNC_RANKSYNCSERIALSKIP_H

#include "sst/core/sst_types.h"
#include "sst/core/sync/syncManager.h"
#include "sst/core/threadsafe.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <string>

namespace SST {

class RankSyncQueue;
class TimeConverter;

namespace Profile {
class SyncProfileToolList;
};

class RankSyncSerialSkip : public RankSync
{
public:
    /** Create a new Sync object which fires with a specified period */
    explicit RankSyncSerialSkip(RankInfo num_ranks);
    RankSyncSerialSkip() {} // For serialization
    virtual ~RankSyncSerialSkip();

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(const RankInfo& to_rank, const RankInfo& from_rank, Link* link) override;
    void           execute(int thread) override;

    /** Cause an exchange of Untimed Data to occur */
    void exchangeLinkUntimedData(int thread, std::atomic<int>& msg_count) override;
    /** Finish link configuration */
    void finalizeLinkConfigurations() override;
    /** Prepare for the complete() stage */
    void prepareForComplete() override;

    /** Set signals to exchange during sync */
    void setSignals(int end, int usr, int alrm) override;
    /** Return exchanged signals after sync */
    bool getSignals(int& end, int& usr, int& alrm) override;

    /** Set interactive flags to exchange during sync */
    // Separated enter_interactive from from shutdown since they may be needed separately
    void setShutdownFlags(bool enter_shutdown, Simulation_impl::ShutdownMode_t shutdown_mode) override;
    void setCkptFlag(bool generate_ckpt) override;
    void setFlags(bool enter_interactive, bool enter_shutdown, Simulation_impl::ShutdownMode_t shutdown_mode) override;
    /** Return exchanged interactive flags after sync */
    void getShutdownFlags(bool& enter_shutdown, Simulation_impl::ShutdownMode_t& shutdown_mode) override;
    void getCkptFlag(bool& generate_ckpt) override;
    void getFlags(
        bool& enter_interactive, bool& enter_shutdown, Simulation_impl::ShutdownMode_t& shutdown_mode) override;
    /** Clear interactive flags before next run */
    void clearFlags() override;

    SimTime_t getNextSyncTime() override { return myNextSyncTime; }

    void setRestartTime(SimTime_t time) override;

    uint64_t getDataSize() const override;

    void setProfileToolList(Profile::SyncProfileToolList* profile_tools) override;

private:
    static SimTime_t myNextSyncTime;

    // Function that actually does the exchange during run
    void exchange();

    struct comm_pair : public SST::Core::Serialization::serializable
    {
        RankSyncQueue* squeue; // RankSyncQueue
        char*          rbuf;   // receive buffer
        uint32_t       local_size;
        uint32_t       remote_size;

        void serialize_order(SST::Core::Serialization::serializer& UNUSED(ser)) override {}
        ImplementSerializable(comm_pair)
    };

    using comm_map_t = std::map<int, comm_pair>;
    using link_map_t = std::map<std::string, uintptr_t>;

    // TimeConverter* period;
    comm_map_t comm_map;
    link_map_t link_map;

    double mpiWaitTime;
    double deserializeTime;

    Profile::SyncProfileToolList* profile_tools_ = nullptr;

    Core::ThreadSafe::Spinlock   lock;
    static int                   sig_end_;
    static int                   sig_usr_;
    static int                   sig_alrm_;
    static std::atomic<bool>     enter_interactive_;
    static std::atomic<bool>     enter_shutdown_;
    static std::atomic<unsigned> shutdown_mode_;
    static std::atomic<bool>     generate_ckpt_;
};

} // namespace SST

#endif // SST_CORE_SYNC_RANKSYNCSERIALSKIP_H
