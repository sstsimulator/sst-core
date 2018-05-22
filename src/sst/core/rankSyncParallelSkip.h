// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_RANKSYNCPARALLELSKIP_H
#define SST_CORE_RANKSYNCPARALLELSKIP_H

#include "sst/core/sst_types.h"
#include <sst/core/syncManager.h>
#include <sst/core/threadsafe.h>
#include <sst/core/warnmacros.h>

#include <map>

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif   

namespace SST {

class SyncQueue;
class TimeConverter;

class RankSyncParallelSkip : public NewRankSync {
public:
    /** Create a new Sync object which fires with a specified period */
    RankSyncParallelSkip(RankInfo num_ranks, TimeConverter* minPartTC);
    virtual ~RankSyncParallelSkip();
    
    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link) override;
    void execute(int thread) override;

    /** Cause an exchange of Untimed Data to occur */
    void exchangeLinkUntimedData(int thread, std::atomic<int>& msg_count) override;
    /** Finish link configuration */
    void finalizeLinkConfigurations() override;
    /** Prepare for complete() stage */
    void prepareForComplete() override;

    SimTime_t getNextSyncTime() override { return myNextSyncTime; }
    
    uint64_t getDataSize() const override;
    
private:

    static SimTime_t myNextSyncTime;

    // Function that actually does the exchange during run
    void exchange_master(int thread);
    void exchange_slave(int thread);
    
    struct comm_send_pair {
        RankInfo to_rank;
        SyncQueue* squeue; // SyncQueue
        char* sbuf;
        uint32_t remote_size;
    };

    struct comm_recv_pair {
        uint32_t remote_rank;
        uint32_t local_thread;
        char* rbuf; // receive buffer
        std::vector<Activity*> activity_vec;
        uint32_t local_size;
        bool recv_done;
#ifdef SST_CONFIG_HAVE_MPI
        MPI_Request req;
#endif   
    };
    
    typedef std::map<RankInfo, comm_send_pair > comm_send_map_t;
    typedef std::map<RankInfo, comm_recv_pair > comm_recv_map_t;
    typedef std::map<LinkId_t, Link*> link_map_t;

    // TimeConverter* period;
    comm_send_map_t comm_send_map;
    comm_recv_map_t comm_recv_map;
    link_map_t link_map;

    double mpiWaitTime;
    double deserializeTime;

    int* recv_count;
    int send_count;

    std::atomic<int32_t> remaining_deser;
    SST::Core::ThreadSafe::BoundedQueue<comm_recv_pair*> deserialize_queue;
    SST::Core::ThreadSafe::UnboundedQueue<comm_recv_pair*>* link_send_queue;
    SST::Core::ThreadSafe::BoundedQueue<comm_send_pair*> serialize_queue;
    SST::Core::ThreadSafe::BoundedQueue<comm_send_pair*> send_queue;
    
    void deserializeMessage(comm_recv_pair* msg);
    

    Core::ThreadSafe::Barrier serializeReadyBarrier;
    Core::ThreadSafe::Barrier slaveExchangeDoneBarrier;
    Core::ThreadSafe::Barrier allDoneBarrier;
};


} // namespace SST

#endif // SST_CORE_RANKSYNCPARALLELSKIP_H
