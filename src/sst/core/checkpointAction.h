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

#ifndef SST_CORE_CHECKPOINT_ACTION_H
#define SST_CORE_CHECKPOINT_ACTION_H

#include "sst/core/action.h"
#include "sst/core/config.h"
#include "sst/core/cputimer.h"
#include "sst/core/output.h"
#include "sst/core/sst_types.h"

#include <set>

namespace SST {

class Simulation_impl;
class TimeConverter;

/**
  \class CheckpointAction
    A recurring event to trigger checkpoint generation
*/
class CheckpointAction : public Action
{
public:
    /**
    Create a new checkpoint object for the simulation core to initiate checkpoints
    */
    CheckpointAction(Config* cfg, int this_rank, Simulation_impl* sim, TimeConverter* period);
    ~CheckpointAction();

    NotSerializable(SST::CheckpointAction) // Going to have to fix this

        private : CheckpointAction() {};
    CheckpointAction(const CheckpointAction&);

    void           operator=(CheckpointAction const&);
    void           execute(void) override;
    int            rank;
    TimeConverter* m_period;
    double         lastTime;
};

} // namespace SST

#endif // SST_CORE_CHECKPOINT_ACTION_H
