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

#ifndef SST_CORE_SYNC_RANKSYNCPARALLELSKIP_H
#define SST_CORE_SYNC_RANKSYNCPARALLELSKIP_H

#include "sst/core/sst_mpi.h"
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

class RankSyncParallelSkip : public RankSync
{
public:
    /** Create a new Sync object which fires with a specified period */
    explicit RankSyncParallelSkip(RankInfo num_ranks);
    RankSyncParallelSkip() {} // For serialization
    virtual ~RankSyncParallelSkip();

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(
        const RankInfo& to_rank, const RankInfo& from_rank, const std::string& name, Link* link) override;
    void execute(int thread) override;

    /** Cause an exchange of Untimed Data to occur */
    void exchangeLinkUntimedData(int thread, std::atomic<int>& msg_count) override;
    /** Finish link configuration */
    void finalizeLinkConfigurations() override;
    /** Prepare for complete() stage */
    void prepareForComplete() override;

    /** Set signals to exchange during sync */
    void setSignals(int end, int usr, int alrm) override;
    /** Return exchanged signals after sync */
    bool getSignals(int& end, int& usr, int& alrm) override;

    SimTime_t getNextSyncTime() override { return myNextSyncTime; }

    void setRestartTime(SimTime_t time) override;

    uint64_t getDataSize() const override;

private:
    static SimTime_t myNextSyncTime;

    // Function that actually does the exchange during run
    void exchange_master(int thread);
    void exchange_slave(int thread);

    struct comm_send_pair : public SST::Core::Serialization::serializable
    {
        RankInfo       to_rank;
        RankSyncQueue* squeue; // RankSyncQueue
        char*          sbuf;
        uint32_t       remote_size;

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            SST_SER(to_rank);
            // squeue - empty so recreate on restart
            // sbuf - empty so recreate on restart
            // remote_size - don't need
        }
        ImplementSerializable(comm_send_pair)
    };

    struct comm_recv_pair : public SST::Core::Serialization::serializable
    {
        uint32_t               remote_rank;
        uint32_t               local_thread;
        char*                  rbuf; // receive buffer
        std::vector<Activity*> activity_vec;
        uint32_t               local_size;
        bool                   recv_done;
#ifdef SST_CONFIG_HAVE_MPI
        MPI_Request req;
#endif
        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            SST_SER(remote_rank);
            SST_SER(local_thread);
            // activity_vec - empty so recreate on restart
            // rbuf - empty so recreate on restart
            // recv_done - don't need
            // req - don't need
        }
        ImplementSerializable(comm_recv_pair)
    };

    using comm_send_map_t = std::map<RankInfo, comm_send_pair>;
    using comm_recv_map_t = std::map<RankInfo, comm_recv_pair>;
    // using link_map_t = std::map<LinkId_t, Link*>;
    using link_map_t      = std::map<std::string, uintptr_t>;

    // TimeConverter* period;
    comm_send_map_t comm_send_map;
    comm_recv_map_t comm_recv_map;
    link_map_t      link_map;

    double mpiWaitTime;
    double deserializeTime;

    int* recv_count;
    int  send_count;

    std::atomic<int32_t>                                    remaining_deser;
    SST::Core::ThreadSafe::BoundedQueue<comm_recv_pair*>    deserialize_queue;
    SST::Core::ThreadSafe::UnboundedQueue<comm_recv_pair*>* link_send_queue;
    SST::Core::ThreadSafe::BoundedQueue<comm_send_pair*>    serialize_queue;
    SST::Core::ThreadSafe::BoundedQueue<comm_send_pair*>    send_queue;

    void deserializeMessage(comm_recv_pair* msg);

    Core::ThreadSafe::Barrier serializeReadyBarrier;
    Core::ThreadSafe::Barrier slaveExchangeDoneBarrier;
    Core::ThreadSafe::Barrier allDoneBarrier;

    Core::ThreadSafe::Spinlock lock;
    static int                 sig_end_;
    static int                 sig_usr_;
    static int                 sig_alrm_;
};

} // namespace SST

#endif // SST_CORE_SYNC_RANKSYNCPARALLELSKIP_H
