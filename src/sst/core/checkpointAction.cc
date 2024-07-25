// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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

CheckpointAction::CheckpointAction(
    Config* UNUSED(cfg), RankInfo this_rank, Simulation_impl* sim, TimeConverter* period) :
    Action(),
    rank_(this_rank),
    period_(period),
    generate_(false)
{
    next_sim_time_ = 0;
    last_cpu_time_ = 0;

    if ( period_ ) {
        next_sim_time_ =
            (period_->getFactor() * (sim->getCurrentSimCycle() / period_->getFactor())) + period_->getFactor();
        sim->insertActivity(next_sim_time_, this);
    }

    if ( (0 == this_rank.rank) ) { last_cpu_time_ = sst_get_cpu_time(); }
}

CheckpointAction::~CheckpointAction() {}

void
CheckpointAction::execute(void)
{
    Simulation_impl* sim = Simulation_impl::getSimulation();
    createCheckpoint(sim);

    next_sim_time_ += period_->getFactor();
    sim->insertActivity(next_sim_time_, this);
}

void
CheckpointAction::createCheckpoint(Simulation_impl* sim)
{

    if ( 0 == rank_.rank ) {
        const double now = sst_get_cpu_time();
        sim->getSimulationOutput().output(
            "# Simulation Checkpoint: Simulated Time %s (Real CPU time since last checkpoint %.5f seconds)\n",
            sim->getElapsedSimTime().toStringBestSI().c_str(), (now - last_cpu_time_));

        last_cpu_time_ = now;
    }

    sim->checkpoint();
}

void
CheckpointAction::check()
{
    // TODO add logic for simulation-interval checkpoints in parallel
    Simulation_impl* sim = Simulation_impl::getSimulation();
    if ( generate_ ) { createCheckpoint(sim); }
    generate_ = false;
}

void
CheckpointAction::setCheckpoint()
{
    generate_ = true;
}

} // namespace SST
