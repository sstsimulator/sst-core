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

#ifndef SST_CORE_THREADSYNC_H
#define SST_CORE_THREADSYNC_H

#include "sst/core/sst_types.h"
#include <sst/core/serialization.h>

#include <unordered_map>

#include "sst/core/action.h"
#include "sst/core/threadSyncQueue.h"

namespace SST {

class SyncQueue;
    

class ActivityQueue;
class Link;
class TimeConverter;
class Exit;
class Event;
class Simulation;
class ThreadSyncQueue;

class ThreadSync : public Action {
public:
    /** Create a new ThreadSync object */
    ThreadSync(int num_threads, Simulation* sim);
    ~ThreadSync();

    void setMaxPeriod(TimeConverter* period);
    
    /** Register a Link which this Sync Object is responsible for */
    void registerLink(LinkId_t link_id, Link* link);
    ActivityQueue* getQueueForThread(int tid);

    void execute(void);

    /** Cause an exchange of Initialization Data to occur */
    void processLinkInitData();
    /** Finish link configuration */
    void finalizeLinkConfigurations();

    uint64_t getDataSize() const;

    void print(const std::string& header, Output &out) const;

    static void disable() { disabled = true; barrier.disable(); }

private:
    std::vector<ThreadSyncQueue*> queues;
    std::unordered_map<LinkId_t, Link*> link_map;
    TimeConverter* max_period;
    int num_threads;
    Simulation* sim;
    static bool disabled;
    static Core::ThreadSafe::Barrier barrier;
    double totalWaitTime;
    bool single_rank;
};


} // namespace SST

#endif // SST_CORE_THREADSYNC_H
