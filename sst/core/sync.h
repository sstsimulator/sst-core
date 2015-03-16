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

#ifndef SST_CORE_SYNC_H
#define SST_CORE_SYNC_H

#include "sst/core/sst_types.h"
#include <sst/core/serialization.h>

#include <map>

#include "sst/core/action.h"

namespace SST {

#define _SYNC_DBG( fmt, args...) __DBG( DBG_SYNC, Sync, fmt, ## args )

class ActivityQueue;
class SyncQueue;
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
class SyncBase : public Action {
public:
    /** Create a new Sync object which fires with a specified period */
    SyncBase() {}
    ~SyncBase() {}

    /** Register a Link which this Sync Object is responsible for */
    virtual ActivityQueue* registerLink(int rank, LinkId_t link_id, Link* link) = 0;

    // void execute(void);

    /** Cause an exchange of Initialization Data to occur */
    virtual int exchangeLinkInitData(int msg_count) = 0;
    /** Finish link configuration */
    virtual void finalizeLinkConfigurations() = 0;

    virtual void setExit(Exit* ex) { exit = ex; }
    virtual void setMaxPeriod(TimeConverter* period);

    void print(const std::string& header, Output &out) const;
    
protected:
    Exit* exit;
    TimeConverter* max_period;

    void sendInitData_sync(Link* link, Event* init_data);
    void finalizeConfiguration(Link* link);
    

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

/**
 * \class Sync
 * Sync objects are used to synchronize between MPI ranks in a simulation.
 * This is an internal class, and not a public-facing API.
 */
class Sync : public SyncBase {
public:
    /** Create a new Sync object which fires with a specified period */
    // Sync(TimeConverter* period);
    Sync();
    ~Sync();

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(int rank, LinkId_t link_id, Link* link);
    void execute(void);

    /** Cause an exchange of Initialization Data to occur */
    int exchangeLinkInitData(int msg_count);
    /** Finish link configuration */
    void finalizeLinkConfigurations();
    
private:
    typedef std::map<int, std::pair<SyncQueue*, std::vector<Activity*>* > > comm_map_t;
    typedef std::map<LinkId_t, Link*> link_map_t;

    // TimeConverter* period;
    comm_map_t comm_map;
    link_map_t link_map;
#ifdef HAVE_MPI
    boost::mpi::communicator comm;
#endif

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

/**
 * \class Sync
 * Sync objects are used to synchronize between MPI ranks in a simulation.
 * This is an internal class, and not a public-facing API.
 */
class SyncB : public SyncBase {
public:
    /** Create a new Sync object which fires with a specified period */
    // Sync(TimeConverter* period);
    SyncB();
    ~SyncB();

    /** Register a Link which this Sync Object is responsible for */
    ActivityQueue* registerLink(int rank, LinkId_t link_id, Link* link);
    void execute(void);

    /** Cause an exchange of Initialization Data to occur */
    int exchangeLinkInitData(int msg_count);
    /** Finish link configuration */
    void finalizeLinkConfigurations();
    
private:
    typedef std::map<int, std::pair<SyncQueue*, std::vector<Activity*>* > > comm_map_t;
    typedef std::map<LinkId_t, Link*> link_map_t;

    // TimeConverter* period;
    comm_map_t comm_map;
    link_map_t link_map;
#ifdef HAVE_MPI
    boost::mpi::communicator comm;
#endif

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Sync)

#endif // SST_CORE_SYNC_H
