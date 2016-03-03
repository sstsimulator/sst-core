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
#include "sst/core/action.h"

#include "sst/core/simulation.h"

namespace SST {

void Action::endSimulation() {
    Simulation::getSimulation()->endSimulation();
}

void Action::endSimulation(SimTime_t end) {
    Simulation::getSimulation()->endSimulation(end);
}

const char*
Action::cls_name() const
{
    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO,-1,"Trying to serialize an Action.  This is not supported.\n");
    return NULL;
}

void
Action::serialize_order(SST::Core::Serialization::serializer& ser)
{
    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO,-1,"Trying to serialize an Action.  This is not supported.\n");
}

uint32_t
Action::cls_id() const
{
    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO,-1,"Trying to serialize an Action.  This is not supported.\n");
    return 0;
}

} // namespace SST

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Action);
