// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/watchPoint.h"

#include "sst/core/output.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_types.h"

#include <string>


namespace SST {

bool
WatchPoint::getInteractive()
{
    return Simulation_impl::getSimulation()->enter_interactive_;
}

void
WatchPoint::setEnterInteractive()
{
    Simulation_impl::getSimulation()->enter_interactive_ = true;
}

void
WatchPoint::setInteractiveMsg(const std::string& msg)
{
    Simulation_impl::getSimulation()->interactive_msg_ = msg;
}

SimTime_t
WatchPoint::getCurrentSimCycle()
{
    return Simulation_impl::getSimulation()->getCurrentSimCycle();
}

void
WatchPoint::setCheckpoint()
{
    if (Simulation_impl::getSimulation()->checkpoint_directory_ == "") {
        std::cout << "Invalid action: checkpointing not enabled (use --checkpoint-enable cmd line option)\n";
    }
    else {
        Simulation_impl::getSimulation()->scheduleCheckpoint();
    }
}

void
WatchPoint::printStatus()
{
    Simulation_impl::getSimulation()->printStatus(true);
}

void
WatchPoint::heartbeat()
{
    // do nothing for now, need to add all the functions similar to RTAction
    // Could it just use RTAction?
}

void
WatchPoint::simulationShutdown()
{
    Simulation_impl::getSimulation()->endSimulation();
}

} // namespace SST
