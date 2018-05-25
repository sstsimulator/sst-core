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


#include "sst_config.h"
#include <sst/core/serialization/serializer.h>
#include <sst/core/syncQueue.h>

#include <sst/core/event.h>

#include <sst/core/simulation.h>


namespace SST {

using namespace Core::ThreadSafe;
using namespace Core::Serialization;

SyncQueue::SyncQueue() :
    ActivityQueue(), buffer(NULL), buf_size(0)
{
}

SyncQueue::~SyncQueue()
{
}
    
bool
SyncQueue::empty()
{
    std::lock_guard<Spinlock> lock(slock);
	return activities.empty();
}

int
SyncQueue::size()
{
    std::lock_guard<Spinlock> lock(slock);
    return activities.size();
}
    
void
SyncQueue::insert(Activity* activity)
{
    std::lock_guard<Spinlock> lock(slock);
    activities.push_back(activity);
}

Activity*
SyncQueue::pop()
{
    // NEED TO FATAL
	// if ( data.size() == 0 ) return NULL;
	// std::vector<Activity*>::iterator it = data.begin();
	// Activity* ret_val = (*it);
	// data.erase(it);
	// return ret_val;
    return NULL;
}

Activity*
SyncQueue::front()
{
    // NEED TO FATAL
	return NULL;
}

void
SyncQueue::clear()
{
    std::lock_guard<Spinlock> lock(slock);
    activities.clear();
}

char*
SyncQueue::getData()
{
    std::lock_guard<Spinlock> lock(slock);

    serializer ser;

    ser.start_sizing();

    ser & activities;

    size_t size = ser.size();

    if ( buf_size < ( size + sizeof(SyncQueue::Header) ) ) {
        if ( buffer != NULL ) {
            delete[] buffer;
        }
        
        buf_size = size + sizeof(SyncQueue::Header);
        buffer = new char[buf_size];
    }
        
    ser.start_packing(buffer + sizeof(SyncQueue::Header), size);

    ser & activities;

    // Delete all the events
    for ( unsigned int i = 0; i < activities.size(); i++ ) {
        delete activities[i];
    }
    activities.clear();

    // Set the size field in the header
    static_cast<SyncQueue::Header*>(static_cast<void*>(buffer))->buffer_size = size + sizeof(SyncQueue::Header);
    
    return buffer;
}

} // namespace SST
