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

#ifndef SST_CORE_SYNCBASE_H
#define SST_CORE_SYNCBASE_H

#include "sst/core/sst_types.h"

#include <sst/core/rankInfo.h>

namespace SST {

class Action;
class ActivityQueue;
class Link;
class TimeConverter;
class Exit;
class Event;
    
/**
 * \class SyncBase
 * SyncBase defines the API for Sync objects, which
 * are used to synchronize between MPI ranks in a simulation.  This is
 * an internal class, and not a public-facing API.
 */
class SyncBase {
public:
    /** Create a new Sync object which fires with a specified period */
    SyncBase() {}
    virtual ~SyncBase() {}

    /** Register a Link which this Sync Object is responsible for */
    virtual ActivityQueue* registerLink(const RankInfo& to_rank, const RankInfo& from_rank, LinkId_t link_id, Link* link) = 0;

    /** Cause an exchange of Untimed Data to occur */
    virtual int exchangeLinkUntimedData(int msg_count) = 0;
    /** Finish link configuration */
    virtual void finalizeLinkConfigurations() = 0;

    virtual void setExit(Exit* ex) { exit = ex; }
    virtual void setMaxPeriod(TimeConverter* period);

    virtual uint64_t getDataSize() const = 0;

    virtual Action* getSlaveAction() = 0;
    virtual Action* getMasterAction() = 0;
    
protected:
    Exit* exit;
    TimeConverter* max_period;

    void sendUntimedData_sync(Link* link, Event* data);
    void finalizeConfiguration(Link* link);
    

};


} // namespace SST

#endif // SST_CORE_SYNCBASE_H
