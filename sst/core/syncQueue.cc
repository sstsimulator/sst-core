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


#include "sst_config.h"
#include "sst/core/serialization.h"
#include <sst/core/syncQueue.h>

#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <sst/core/event.h>

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

#include "sst/core/simulation.h"

namespace SST {


SyncQueue::SyncQueue() :
    ActivityQueue()
{
}

SyncQueue::~SyncQueue()
{
}
    
bool
SyncQueue::empty()
{
	return activities.empty();
}

int
SyncQueue::size()
{
    return activities.size();
}
    
void
SyncQueue::insert(Activity* activity)
{
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
    activities.clear();
}

char*
SyncQueue::getData()
{
    buffer.clear();

    // Reserve space for the header information
    for ( unsigned int i = 0; i < sizeof(SyncQueue::Header); i++ ) {
        buffer.push_back(0);
    }

    boost::iostreams::back_insert_device<std::vector<char> > inserter(buffer);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::vector<char> > > output_stream(inserter);
    boost::archive::polymorphic_binary_oarchive oa(output_stream, boost::archive::no_header || boost::archive:: no_tracking);

    oa << activities;
    output_stream.flush();
    
    for ( unsigned int i = 0; i < activities.size(); i++ ) {
        delete activities[i];
    }
    activities.clear();

    SyncQueue::Header hdr;
    hdr.buffer_size = buffer.size();

    char* hdr_bytes = reinterpret_cast<char*>(&hdr);
    for ( unsigned int i = 0; i < sizeof(SyncQueue::Header); i++ ) {
        buffer[i] = hdr_bytes[i];
    }
    
    return buffer.data();
 }
} // namespace SST

// BOOST_CLASS_EXPORT_IMPLEMENT(SST::SyncQueue)
