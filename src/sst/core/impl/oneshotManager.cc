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

#include "sst/core/impl/oneshotManager.h"

#include "sst/core/simulation_impl.h"
#include "sst/core/timeVortex.h"

namespace SST::Core {

void
OneShot::execute()
{
    // Call all the handlers.  Delete each one once after calling.
    // The handlers are guaranteed not to be shared.
    for ( auto* x : handlers ) {
        (*x)();
        delete x;
    }

    manager->oneshotCallback(time, this);
}


OneShotManager::OneShotManager(Simulation_impl* sim) :
    sim_(sim)
{}

OneShotManager::~OneShotManager()
{
    // Need to delete all the Handlers
    for ( auto& list : handler_vector_map_ ) {
        for ( auto* item : list.second.first ) {
            delete item;
        }
    }
}

void
OneShotManager::registerHandlerBase(SimTime_t trigger_time, int priority, bool relative, OneShot::HandlerBase* handler)
{

    // Check to make sure this isn't in the past
    TimeStamp_t curr_time = std::make_pair(sim_->getCurrentSimCycle(), sim_->getCurrentPriority());
    TimeStamp_t trig_time = std::make_pair(trigger_time + (relative ? sim_->getCurrentSimCycle() : 0), priority);

    if ( trig_time <= curr_time ) {
        // Emit warning and ignore request
        sim_->getSimulationOutput().output(
            "WARNING: Trying to register a OneShot for a time in the past, ignoring request\n");
        return;
    }

    // Get the handler list for the specified time stamp.  If it's not
    // already there, it will be created.
    std::pair<OneShot::HandlerList_t, bool>& map_item = handler_vector_map_[trig_time];

    // Push handler onto the list
    map_item.first.push_back(handler);

    // Check to see if anything needs to be scheduled
    scheduleNextOneShot();
}

void
OneShotManager::oneshotCallback(TimeStamp_t time, OneShot* oneshot)
{
    // Remove the data for this time, schedule the next time and
    // delete the OneShot.
    handler_vector_map_.erase(time);
    delete oneshot;

    scheduleNextOneShot();
}

void
OneShotManager::scheduleNextOneShot()
{
    // Get next event to schdule.  Iterator maps as follows:
    // it.first : TimeStamp_t : trigger time
    // it.second.first = HandlerList_t : list of handlers to call
    // it.second.second = bool : scheduled
    auto it = handler_vector_map_.begin();

    // If there are no events, return
    if ( it == handler_vector_map_.end() ) return;

    // If first event is already scheduled, nothing to do
    if ( it->second.second ) return;

    // Not yet scheduled, create a OneShot and insert into TimeVortex
    TimeStamp_t t = it->first;

    OneShot* oneshot = new OneShot(t, this, it->second.first);
    sim_->timeVortex->insert(oneshot);
    it->second.second = true;
}

} // namespace SST::Core
