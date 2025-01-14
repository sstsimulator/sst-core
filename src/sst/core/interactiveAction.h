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
#include "sst/core/simulation_impl.h"


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
    InteractiveAction(Simulation_impl* sim, const std::string& msg) : Action(), sim_(sim), msg_(msg)
    {
        setPriority(INTERACTIVEPRIOIRTY);
    }

    ~InteractiveAction() {}

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
