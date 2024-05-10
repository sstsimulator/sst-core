// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_ACTION_H
#define SST_CORE_ACTION_H

#include "sst/core/activity.h"
#include "sst/core/output.h"
#include "sst/core/serialization/serializer.h"

#include <cinttypes>

namespace SST {

/**
 * An Action is a schedulable Activity which is not an Event.
 */
class Action : public Activity
{
public:
    Action() {}
    ~Action() {}

protected:
    /** Called to signal to the Simulation object to end the simulation */
    void endSimulation();

    /** Called to signal to the Simulation object to end the simulation
     * @param end Simulation cycle when the simulation finishes
     */
    void endSimulation(SimTime_t end);

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementVirtualSerializable(SST::Action)
};

} // namespace SST

#endif // SST_CORE_ACTION_H
