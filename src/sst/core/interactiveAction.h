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

#ifndef SST_CORE_INTERACTIVE_ACTION_H
#define SST_CORE_INTERACTIVE_ACTION_H

#include "sst/core/action.h"
// Can include this because this header is not installed
#include "sst/core/interactiveConsole.h"
#include "sst/core/simulation_impl.h"

#include <string>

namespace SST {

/**
  \class InteractiveAction An event to trigger interactive mode.  This
    is a "one shot" event and will delete itself on execute()
*/
class InteractiveAction : public Action
{
public:
    /**
       Create a new InteractiveAction object for the simulation core to initiate interactive mode
    */
    InteractiveAction(Simulation_impl* sim, const std::string& msg) :
        Action(),
        sim_(sim),
        msg_(msg)
    {
        setPriority(INTERACTIVEPRIOIRTY);
    }

    ~InteractiveAction() {}
#if 1
    /**
       Indicates InteractiveAction should be inserted into the
       TimeVortex. The insertion will only happen for serial runs, as
       InteractiveAction is managed by the SyncManager in parallel
       runs.
     */
    void insertIntoTimeVortex(SimTime_t time)
    {
        // If this is a serial job, insert this into
        // the time TimeVortex.  If it is parallel, then the
        // InteractiveAction is managed by the SyncManager.
        // std::cout << "skk: insertIntoTimeVortex called\n";
        RankInfo num_ranks = sim_->getNumRanks();
        // if (num_ranks.rank == 1 && num_ranks.thread == 1) {
        // std::cout << "  skk: insertIntoTimeVortex insertActivity\n";
        sim_->insertActivity(time, this);
        //}
    }
#endif

    /** Called by TimeVortex to trigger interactive mode. */
    void execute() override
    {
        sim_->enter_interactive_ = true;
        sim_->interactive_msg_   = msg_;
        delete this;
    }


private:
    Simulation_impl* sim_;
    std::string      msg_;
};

} // namespace SST
#endif // SST_CORE_INTERACTIVE_ACTION_H
