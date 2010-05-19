// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_DIRECTLINKQUEUE_H
#define SST_DIRECTLINKQUEUE_H

#include <set>

#include <sst/core/activityQueue.h>

namespace SST {

    class DirectLinkQueue : public ActivityQueue {
public:
	DirectLinkQueue() : ActivityQueue() {}
    virtual ~DirectLinkQueue();

    bool empty() {return data.empty();}
    int size() {return data.size();}
    void insert(Activity* activity) {data.insert(activity);}
    Activity* pop() {
	if ( data.size() == 0 ) return NULL;
	std::multiset<Activity*,Activity::less_time>::iterator it = data.begin();
	Activity* ret_val = (*it);
	data.erase(it);
	return ret_val;
    }
    
private:

    std::multiset<Activity*,Activity::less_time> data;
    
//     friend class boost::serialization::access;
//     template<class Archive>
//     void
//     serialize(Archive & ar, const unsigned int version )
//     {
//     }
};

}

#endif // SST_DIRECTLINKQUEUE_H
