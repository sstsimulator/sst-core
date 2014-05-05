// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_ACTION_H
#define SST_CORE_ACTION_H

#include <sst/core/serialization.h>

#include <sst/core/activity.h>
#include <sst/core/output.h>

namespace SST {

/**
 * An Action is a schedulable Activity which is not an Event.
 */
class Action : public Activity {
public:
    Action() {}
    ~Action() {}

    void print(const std::string& header, Output &out) const {
        out.output("%s Generic Action to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
    }


protected:
    /** Called to signal to the Simulation object to end the simulation */
    void endSimulation();

private:
     friend class boost::serialization::access;
     template<class Archive>
     void
     serialize(Archive & ar, const unsigned int version )
     {
         ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Activity);
     }
};

} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Action)

#endif // SST_CORE_ACTION_H
