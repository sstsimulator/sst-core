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

#include "sst_config.h"

#include "sst/core/sync/syncQueue.h"

#include "sst/core/event.h"
#include "sst/core/serialization/serializer.h"
#include "sst/core/simulation_impl.h"

#if SST_EVENT_PROFILING
#define SST_EVENT_PROFILE_SIZE(events, bytes)                    \
    do {                                                         \
        Simulation_impl* sim = Simulation_impl::getSimulation(); \
        sim->incrementExchangeCounters(events, bytes);           \
    } while ( 0 );
#else
#define SST_EVENT_PROFILE_SIZE(events, bytes)
#endif

namespace SST {

using namespace Core::ThreadSafe;
using namespace Core::Serialization;

RankSyncQueue::RankSyncQueue(RankInfo to_rank) :
    SyncQueue(to_rank),
    buffer(nullptr),
    buf_size(0)
{}

bool
RankSyncQueue::empty()
{
    std::lock_guard<Spinlock> lock(slock);
    return activities.empty();
}

int
RankSyncQueue::size()
{
    std::lock_guard<Spinlock> lock(slock);
    return activities.size();
}

void
RankSyncQueue::insert(Activity* activity)
{
    std::lock_guard<Spinlock> lock(slock);
    activities.push_back(activity);
}

Activity*
RankSyncQueue::pop()
{
    // NEED TO FATAL
    // if ( data.size() == 0 ) return nullptr;
    // std::vector<Activity*>::iterator it = data.begin();
    // Activity* ret_val = (*it);
    // data.erase(it);
    // return ret_val;
    return nullptr;
}

Activity*
RankSyncQueue::front()
{
    // NEED TO FATAL
    return nullptr;
}

void
RankSyncQueue::clear()
{
    std::lock_guard<Spinlock> lock(slock);
    activities.clear();
}

char*
RankSyncQueue::getData()
{
    std::lock_guard<Spinlock> lock(slock);

    serializer ser;

    ser.start_sizing();

    SST_SER(activities);

    size_t size = ser.size();

    SST_EVENT_PROFILE_SIZE(activities.size(), size)

    if ( buf_size < (size + sizeof(RankSyncQueue::Header)) ) {
        if ( buffer != nullptr ) {
            delete[] buffer;
        }

        buf_size = size + sizeof(RankSyncQueue::Header);
        buffer   = new char[buf_size];
    }

    ser.start_packing(buffer + sizeof(RankSyncQueue::Header), size);

    SST_SER(activities);

    // Delete all the events
    for ( unsigned int i = 0; i < activities.size(); i++ ) {
        delete activities[i];
    }
    activities.clear();

    // Set the size field in the header
    static_cast<RankSyncQueue::Header*>(static_cast<void*>(buffer))->buffer_size = size + sizeof(RankSyncQueue::Header);

    return buffer;
}

} // namespace SST
