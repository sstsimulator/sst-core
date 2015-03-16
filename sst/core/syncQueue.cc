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

#include <sst/core/event.h>

namespace SST {

    SyncQueue::SyncQueue() : ActivityQueue() {}

    SyncQueue::~SyncQueue() {}
    
    bool SyncQueue::empty()
    {
	return data.empty();
    }
    
    int SyncQueue::size()
    {
	return data.size();
    }
    
    void SyncQueue::insert(Activity* activity)
    {
	data.push_back(activity);
	// static_cast<Event*>(activity)->setRemoteEvent();
    }
    
    Activity* SyncQueue::pop()
    {
	if ( data.size() == 0 ) return NULL;
	std::vector<Activity*>::iterator it = data.begin();
	Activity* ret_val = (*it);
	data.erase(it);
	return ret_val;
    }

    Activity* SyncQueue::front()
    {
	return data.front();
    }

    void SyncQueue::clear()
    {
	// Need to delete the event
	for ( unsigned int i = 0; i < data.size(); i++ ) {
	    delete data[i];
	}
	data.clear();
    }

    std::vector<Activity*>* SyncQueue::getVector()
    {
	return &data;
    }

} // namespace SST

BOOST_CLASS_EXPORT_IMPLEMENT(SST::SyncQueue)
