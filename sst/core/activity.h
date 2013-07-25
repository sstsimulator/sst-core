// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_ACTIVITY_H
#define SST_ACTIVITY_H

#include <sst/core/sst_types.h>
#include <sst/core/output.h>

namespace SST {

class Activity {
public:
  Activity() {}
    virtual ~Activity() {}

    virtual void execute(void) = 0;
    
    // Comparator class to use with STL container classes.
    class less_time_priority {
    public:
	inline bool operator()(const Activity* lhs, const Activity* rhs) {
	    if (lhs->delivery_time == rhs->delivery_time ) return lhs->priority < rhs->priority;
	    else return lhs->delivery_time < rhs->delivery_time;
	}

	inline bool operator()(const Activity& lhs, const Activity& rhs) {
	    if (lhs.delivery_time == rhs.delivery_time ) return lhs.priority < rhs.priority;
	    else return lhs.delivery_time < rhs.delivery_time;
	}
    };
    
    // Comparator class to use with STL container classes.
    class less_time {
    public:
	inline bool operator()(const Activity* lhs, const Activity* rhs) {
	    return lhs->delivery_time < rhs->delivery_time;
	}

	inline bool operator()(const Activity& lhs, const Activity& rhs) {
	    return lhs.delivery_time < rhs.delivery_time;
	}
    };
    
    void setDeliveryTime(SimTime_t time) {
	delivery_time = time;
    }

    inline SimTime_t getDeliveryTime() const {
	return delivery_time;
    }

    inline int getPriority() const {
	return priority;
    }

    virtual void print(const std::string& header, Output &out) const {
        out.output("%s Generic Activity to be delivered at %"PRIu64" with priority %d\n",
                header.c_str(), delivery_time, priority);
    }
    
protected:
    void setPriority(int priority) {
	this->priority = priority;
    }

private:
    SimTime_t delivery_time;
    int       priority;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_NVP(delivery_time);
        ar & BOOST_SERIALIZATION_NVP(priority);
    }
};

}

BOOST_CLASS_EXPORT_KEY(SST::Activity)

#endif // SST_ACTIVITY_H
