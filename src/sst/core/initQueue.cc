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

#include <sst/core/initQueue.h>

namespace SST {

    InitQueue::InitQueue() : ActivityQueue() {}
    InitQueue::~InitQueue() {
	// Need to delete any events left in the queue
	int size = data.size();
	for ( int i = 0; i < size; ++i ) {
	    delete data.front();
	    data.pop_front();
	}
    }

    bool InitQueue::empty()
    {
	return data.empty();
    }
    
    int InitQueue::size()
    {
	return data.size();
    }
    
    void InitQueue::insert(Activity* activity)
    {
	data.push_back(activity);
    }
    
    Activity* InitQueue::pop()
    {
	if ( data.size() == 0 ) return NULL;
	Activity* ret_val = data.front();
	data.pop_front();
	return ret_val;
    }

    Activity* InitQueue::front()
    {
	if ( data.size() == 0 ) return NULL;
	return data.front();
    }


} // namespace SST

