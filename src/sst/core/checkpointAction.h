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

#ifndef SST_CORE_CHECKPOINT_ACTION_H
#define SST_CORE_CHECKPOINT_ACTION_H

#include "sst/core/action.h"
#include "sst/core/config.h"
#include "sst/core/cputimer.h"
#include "sst/core/output.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/threadsafe.h"

#include <set>

namespace SST {

class Simulation_impl;
class TimeConverter;

namespace Checkpointing {
/* Utility functions needed to manage directories */

/**
   Creates a directory of the specified basename. If a directory named
   basename already exists, it will append an _N to the end,
   incrementing N from 1 until it finds an unused name.
 */
std::string createUniqueDirectory(const std::string basename);

/**
   Removes a directory.  For safety, this will recurse and remove each
   file individually instead of issuing an rm -r.  It will not follow
   links, but will simply remove the link.
 */
void removeDirectory(const std::string name);

/**
   Initializes the infrastructure needed for checkpointing.  Uses the
   createUniqueDirectory() function to create the directory, then
   broadcasts the name to all ranks.
 */
std::string initializeCheckpointInfrastructure(Config* cfg, bool rt_can_ckpt, int myRank);

} // namespace Checkpointing

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
    CheckpointAction(Config* cfg, RankInfo this_rank, Simulation_impl* sim, TimeConverter* period);
    ~CheckpointAction();

    /** Generate a checkpoint next time check() is called */
    void setCheckpoint();

    /** Called by TimeVortex to trigger checkpoint on simulation clock interval - not used in parallel simulation */
    void execute() override;

    /** Called by SyncManager to check whether a checkpoint should be generated */
    SimTime_t check(SimTime_t current_time);

    /** Return next checkpoint time */
    SimTime_t getNextCheckpointSimTime();

    static Core::ThreadSafe::Barrier barrier;
    static uint32_t                  checkpoint_id;

    NotSerializable(SST::CheckpointAction);

private:
    CheckpointAction(const CheckpointAction&) = delete;
    CheckpointAction& operator=(const CheckpointAction&) = delete;

    void createCheckpoint(Simulation_impl* sim); // The actual checkpoint operation

    RankInfo       rank_;          // RankInfo for this thread/rank
    TimeConverter* period_;        // Simulation time interval for scheduling or nullptr if not set
    double         last_cpu_time_; // Last time a checkpoint was triggered
    bool           generate_;      // Whether a checkpoint should be done next time check() is called
    SimTime_t      next_sim_time_; // Next simulationt ime a checkpoint should trigger at or 0 if not applicable
    std::string    dir_format_;    // Format string for checkpoint directory names
    std::string    file_format_;   // Format string for checkpoint file names
};

} // namespace SST

#endif // SST_CORE_CHECKPOINT_ACTION_H
