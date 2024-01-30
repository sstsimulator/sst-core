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

#include "sst_config.h"

#include "sst/core/checkpointAction.h"

#include "sst/core/component.h"
#include "sst/core/mempoolAccessor.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/stringize.h"
#include "sst/core/timeConverter.h"
#include "sst/core/warnmacros.h"

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

namespace SST {

CheckpointAction::CheckpointAction(Config* UNUSED(cfg), int this_rank, Simulation_impl* sim, TimeConverter* period) :
    Action(),
    rank(this_rank),
    m_period(period)
{
    sim->insertActivity(period->getFactor(), this);
    if ( (0 == this_rank) ) { lastTime = sst_get_cpu_time(); }
    // if( (0 == this_rank) ) {
    //     sim->insertActivity( period->getFactor(), this );
    //     lastTime = sst_get_cpu_time();
    // }
}

CheckpointAction::~CheckpointAction() {}

void
CheckpointAction::execute(void)
{
    Simulation_impl* sim = Simulation_impl::getSimulation();
    const double     now = sst_get_cpu_time();

    if ( 0 == rank ) {
        sim->getSimulationOutput().output(
            "# Simulation Checkpoint: Simulated Time %s (Real CPU time since last checkpoint %.5f seconds)\n",
            sim->getElapsedSimTime().toStringBestSI().c_str(), (now - lastTime));

        lastTime = now;
    }

    sim->checkpoint();

    SimTime_t next = sim->getCurrentSimCycle() + m_period->getFactor();
    sim->insertActivity(next, this);

    // Print some resource usage
}

} // namespace SST
