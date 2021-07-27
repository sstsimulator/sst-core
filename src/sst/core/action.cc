// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/action.h"

#include "sst/core/simulation_impl.h"

namespace SST {

void
Action::endSimulation()
{
    Simulation_impl::getSimulation()->endSimulation();
}

void
Action::endSimulation(SimTime_t end)
{
    Simulation_impl::getSimulation()->endSimulation(end);
}

} // namespace SST
