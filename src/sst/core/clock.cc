// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/clock.h"

#include "sst/core/factory.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"

#include <sys/time.h>

namespace SST {

Clock::Clock(TimeConverter period, int priority) :
    Action(),
    current_cycle_(0),
    period_(period),
    scheduled_(false)
{
    setPriority(priority);
}

void
Clock::registerHandler(Clock::HandlerBase* handler)
{
    if ( handler->clock_ != this ) {
        if ( handler->clock_ == nullptr ) {
            handler->clock_ = this;
        }
        else if ( handler->isActive() ) {
            Simulation::getSimulationOutput().fatal(CALL_INFO, 1,
                "ERROR: Attempting to register a handler with two different clocks. Please use different handlers for "
                "each clock.\n");
        }
        else {
            Simulation::getSimulationOutput().output(
                "WARNING: Registering Clock::Handler that was already registered with another clock.  Clock handlers "
                "should only be registered with a single clock frequency.  If using multiple clocks, each Clock needs "
                "a separate handler, but multiple handlers can point to the same function to be called each clock "
                "cycle. The clock handler will now be associated with the new clock, but as of SST 17, this will "
                "become a fatal error.\n");

            // For now, we will just associate this handler with the new clock, but at SST 17, this will become a fatal
            // error.
            handler->clock_ = this;
            handler->markAsInactive();
        }
    }

    if ( handler->isActive() ) {
        Simulation::getSimulationOutput().output(
            "WARNING: Register Clock::Handler that is already active.  Handler will not be registered again.\n");
        return;
    }
    handler->markAsActive(static_handler_map_.size());
    static_handler_map_.push_back(handler);

    if ( !scheduled_ ) {
        schedule();
    }
}

bool
Clock::unregisterHandler(Clock::HandlerBase* handler, bool& empty)
{
    if ( handler->active_ ) {
        // Need to get index before marking it as inactive, which sets index to -1
        int  index = handler->index_;
        auto iter  = static_handler_map_.begin() + index;
        iter       = static_handler_map_.erase(iter);
        handler->markAsInactive();

        // Need to update the index for every handler below this one
        for ( ; iter != static_handler_map_.end(); ++iter ) {
            (*iter)->index_ = index++;
        }
    }

    empty = static_handler_map_.empty();

    return 0;
}

void
Clock::registerHandler_restart(Clock::HandlerBase* handler)
{
    handler->clock_ = this;
    if ( !handler->active_ ) return;

    handler->index_ = static_handler_map_.size();
    static_handler_map_.push_back(handler);

    if ( !scheduled_ ) {
        schedule();
    }
}

bool
Clock::isHandlerRegistered(Clock::HandlerBase* handler)
{
    for ( auto* h : static_handler_map_ ) {
        if ( h == handler ) return true;
    }

    return false;
}


Cycle_t
Clock::getNextCycle()
{
    if ( !scheduled_ ) updateCurrentCycle();

    return current_cycle_ + 1;
}

void
Clock::execute()
{
    Simulation* sim = Simulation::getSimulation();

    if ( static_handler_map_.empty() ) {
        scheduled_ = false;
        return;
    }

    // Derive the current cycle from the core time
    current_cycle_++;

    auto sop_iter = static_handler_map_.begin();
    for ( ; sop_iter != static_handler_map_.end(); ++sop_iter ) {
        Clock::HandlerBase* handler = *sop_iter;

        if ( (*handler)(current_cycle_) ) {
            handler->markAsInactive();
            // Just break here.  The handler has been marked inactive and the pointer will be overwritten or removed at
            // the end as an empty slot.  This will break us into the compacting version of the iteration in the loop
            // below
            break;
        }
    }

    // Check to see if it finished all the handlers without any of them removing themselves.  As soon as the first
    // handler removes itself, we drop out of the original loop and enter this one that will compact the rest of the
    // entries then delete any empty spaces at the end. This gets rid of the need to constantly delete individual
    // elements, doing multiple memcpy's of all entries below. This also allows us to update the index for each handler.
    if ( sop_iter != static_handler_map_.end() ) {
        // sop_iter becomes the location to copy to and we will start one past this calling the handler.  Then, we will
        // copy the handler to the copy_iter and update its index.
        auto   copy_iter = sop_iter; // Rename for clarity
        size_t index     = copy_iter - static_handler_map_.begin();

        for ( auto call_iter = sop_iter + 1; call_iter != static_handler_map_.end(); ++call_iter ) {
            Clock::HandlerBase* handler = *call_iter;

            if ( (*handler)(current_cycle_) ) {
                // just mark as inactive.  The pointer will either get overwritten, or removed at the end as an empty
                // slot
                handler->markAsInactive();
            }
            else {
                // Still active.  Need to copy up to copy_iter location and update the index
                handler->index_ = index++;
                *copy_iter      = *call_iter;

                // Advance the copy location
                ++copy_iter;
            }
        }

        // Need to remove an empty locations in the vector
        static_handler_map_.erase(copy_iter, static_handler_map_.end());
    }

    // Compute the next time to fire
    next_ = sim->getCurrentSimCycle() + period_.getFactor();
    sim->insertActivity(next_, this);

    return;
}

void
Clock::schedule()
{
    Simulation* sim = Simulation::getSimulation();
    current_cycle_  = sim->getCurrentSimCycle() / period_.getFactor();
    SimTime_t next  = (current_cycle_ * period_.getFactor()) + period_.getFactor();

    // Check to see if we need to insert clock into queue at current
    // simtime.  This happens if the clock would have fired at this
    // tick and if the current priority is less than my priority.
    // However, if we are at time = 0, then we always go out to the
    // next cycle. Also, if the call happens during complete() or
    // finish(), then we don't adjust either.
    if ( sim->getCurrentPriority() < getPriority() && sim->getCurrentSimCycle() != 0 && !sim->endSim ) {
        if ( sim->getCurrentSimCycle() % period_.getFactor() == 0 ) {
            next = sim->getCurrentSimCycle();
            current_cycle_--; // First thing execute does in increment current_cycle_
        }
    }

    sim->insertActivity(next, this);
    scheduled_ = true;
}

void
Clock::updateCurrentCycle()
{
    Simulation* sim = Simulation::getSimulation();
    current_cycle_  = sim->getCurrentSimCycle() / period_.getFactor();
    return;
}

std::string
Clock::toString() const
{
    std::stringstream buf;
    buf << "Clock Activity with period " << period_.getFactor() << " to be delivered at " << getDeliveryTime()
        << " with priority " << getPriority() << " with " << static_handler_map_.size() << " items on clock list";
    return buf.str();
}

void
Clock::serialize_order(SST::Core::Serialization::serializer& ser)
{
    Action::serialize_order(ser);

    // Won't serialize the handlers; they'll be re-registered at
    // restart
    SST_SER(current_cycle_);
    SST_SER(period_);
    SST_SER(next_);
    SST_SER(scheduled_);
}


} // namespace SST
