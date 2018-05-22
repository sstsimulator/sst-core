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


#ifndef SST_CORE_ACTION_H
#define SST_CORE_ACTION_H

#include <sst/core/serialization/serializer.h>

#include <cinttypes>

#include <sst/core/activity.h>
#include <sst/core/output.h>



namespace SST {

/**
 * An Action is a schedulable Activity which is not an Event.
 */
class Action : public Activity {
public:
    Action() {
#ifdef SST_ENFORCE_EVENT_ORDERING
        enforce_link_order = 0;
#endif
    }
    ~Action() {}

    void print(const std::string& header, Output &out) const override {
        out.output("%s Generic Action to be delivered at %" PRIu64 " with priority %d\n",
                header.c_str(), getDeliveryTime(), getPriority());
    }


protected:
    /** Called to signal to the Simulation object to end the simulation */
    void endSimulation();
    void endSimulation(SimTime_t end);

private:
    NotSerializable(Action)
};

} //namespace SST

#endif // SST_CORE_ACTION_H
