// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_RANKSYNC_H
#define SST_CORE_RANKSYNC_H

#include "sst/core/sst_types.h"
#include <sst/core/syncBase.h>
#include <sst/core/threadsafe.h>

#include <map>

#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif


namespace SST {

class Activity;
class SyncQueue;

class RankSync : public SyncBase {
public:
    /** Create a new Sync object which fires with a specified period */
    // Sync(TimeConverter* period);
    RankSync(Core::ThreadSafe::Barrier& barrier);
    ~RankSync();
    
    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link);
    void execute(void);

    /** Cause an exchange of Initialization Data to occur */
    int exchangeLinkInitData(int msg_count);
    /** Finish link configuration */
    void finalizeLinkConfigurations();

    uint64_t getDataSize() const;
    
    Action* getSlaveAction();
    Action* getMasterAction();

private:

    struct comm_pair_send {
        SyncQueue* squeue; // SyncQueue
        uint32_t remote_size;
        RankInfo dest;
        char* buffer;
    };
    
    struct comm_pair_recv {
        char* rbuf; // receive buffer
        uint32_t local_size;
        uint32_t remote_rank;
        uint32_t local_thread;
    };

#ifdef SST_CONFIG_HAVE_MPI
    int sendQueuedEvents(comm_pair_send& send_info, MPI_Request* request);
    std::vector<Activity*>* recvEvents(comm_pair_recv& recv_info);
#endif
    
    // typedef std::map<int, std::pair<SyncQueueC*, std::vector<char>* > > comm_map_t;
    typedef std::map<RankInfo, comm_pair_send > comm_send_map_t;
    typedef std::map<RankInfo, comm_pair_recv > comm_recv_map_t;
    typedef std::map<LinkId_t, Link*> link_map_t;

    // TimeConverter* period;
    comm_send_map_t comm_send_map;
    comm_recv_map_t comm_recv_map;
    link_map_t link_map;

    Core::ThreadSafe::Barrier& barrier;
    
    Core::ThreadSafe::Spinlock slock;
    Core::ThreadSafe::BoundedQueue<comm_pair_send*>* serializeQ;
    Core::ThreadSafe::BoundedQueue<comm_pair_send*>* sendQ;

    double mpiWaitTime;
    double deserializeTime;

    friend class RankSyncSlave;
    friend class RankSyncMaster;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};


} // namespace SST

#endif // SST_CORE_RANKSYNC_H
