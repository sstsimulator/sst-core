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

} // namespace SST
