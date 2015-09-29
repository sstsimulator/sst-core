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

#ifndef SST_CORE_RANKSYNCSERIALSKIP_H
#define SST_CORE_RANKSYNCSERIALSKIP_H

#include "sst/core/sst_types.h"
#include <sst/core/syncManager.h>
#include <sst/core/threadsafe.h>

#include <map>

namespace SST {

class SyncQueue;
class TimeConverter;

class RankSyncSerialSkip : public NewRankSync {
public:
    /** Create a new Sync object which fires with a specified period */
    // Sync(TimeConverter* period);
    RankSyncSerialSkip(Core::ThreadSafe::Barrier& barrier, TimeConverter* minPartTC);
    virtual ~RankSyncSerialSkip();
    
    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link);
    void execute(int thread);

    /** Cause an exchange of Initialization Data to occur */
    void exchangeLinkInitData(int thread, std::atomic<int>& msg_count);
    /** Finish link configuration */
    void finalizeLinkConfigurations();

    SimTime_t getNextSyncTime() { return myNextSyncTime; }
    
    uint64_t getDataSize() const;
    
private:

    static SimTime_t myNextSyncTime;
    TimeConverter* minPartTC;
    
    // Function that actually does the exchange during run
    void exchange();
    
    struct comm_pair {
        SyncQueue* squeue; // SyncQueue
        char* rbuf; // receive buffer
        uint32_t local_size;
        uint32_t remote_size;
    };
    
    // typedef std::map<int, std::pair<SyncQueueC*, std::vector<char>* > > comm_map_t;
    typedef std::map<int, comm_pair > comm_map_t;
    typedef std::map<LinkId_t, Link*> link_map_t;

    // TimeConverter* period;
    comm_map_t comm_map;
    link_map_t link_map;

    double mpiWaitTime;
    double deserializeTime;

    Core::ThreadSafe::Barrier& barrier;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};


} // namespace SST

#endif // SST_CORE_SYNCMANAGER_H
