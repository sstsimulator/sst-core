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

void
Clock::Group::registerHandler(HandlerBase* handler)
{
    bool currently_inactive = (inactive_start_ == 0);
    handler->order_         = handler_count_++;
    handler->group_         = this;

    // Put handler at end of list, then swap to active region if necessary
    int32_t end_index = (int32_t)handlers_.size();
    handler->index_   = end_index;
    handlers_.push_back(handler);

    if ( inactive_start_ != end_index ) {
        // Need to swap this to the active region (i.e. to inactive_start_)
        swapActiveWithInactive(handlers_[end_index], handlers_[inactive_start_]);
    }

    // Increment inactive_start_
    ++inactive_start_;

    if ( currently_inactive ) clock_->activateGroup(this);
}

void
Clock::Group::activateHandler(HandlerBase* handler)
{
    // Check to see if it's already active
    if ( handler->index_ >= 0 ) return;

    bool currently_inactive = (inactive_start_ == 0);

    // Get the actual index of the inactive handler
    int32_t index = -1 - handler->index_;

    // We will just swap the handler at inactive_start_ with this handler, then we don't need to move as much stuff
    // around
    if ( index != inactive_start_ ) {
        swapSameActiveState(handlers_[index], handlers_[inactive_start_]);
    }

    // Mark this as active
    handler->toggleActiveState();

    // Now we need to move down all the active handlers until we find where we belong based on order
    index = inactive_start_;

    // Increment inactive_start_ since we have another active handler
    ++inactive_start_;

    while ( index > 0 ) {
        if ( handlers_[index - 1]->order_ < handler->order_ ) {
            // Found where this goes
            break;
        }
        handlers_[index]         = handlers_[index - 1];
        handlers_[index]->index_ = index;
        --index;
    }

    handlers_[index] = handler;
    handler->index_  = index;

    if ( currently_inactive ) clock_->activateGroup(this);
}

void
Clock::Group::deactivateHandler(HandlerBase* handler)
{
    // Check to see if this is currently inactive
    if ( handler->index_ < 0 ) return;

    // We need to move the handler below inactive_start_ and move everything up to that point up one space
    --inactive_start_;
    for ( int i = handler->index_; i < inactive_start_; ++i ) {
        handlers_[i]         = handlers_[i + 1];
        handlers_[i]->index_ = i;
    }
    handler->index_ = inactive_start_;
    handler->toggleActiveState();
    handlers_[inactive_start_] = handler;

    // No need to check to deactivate group as it will do it automatically on the next call to execute(). The only
    // oddity would be if a handler gets reactivated before the next call to execute(), in which case it will activate
    // the group even though it is already active.  This will end up being a no-op
}

bool
Clock::Group::execute(Cycle_t current_cycle)
{
    int32_t call_index = 0;
    for ( ; call_index < inactive_start_; ++call_index ) {
        HandlerBase* handler = handlers_[call_index];
        if ( (*handler)(current_cycle) ) {
            handler->toggleActiveState();
            // Just break here. This will break us into the compacting version of the iteration in the loop below
            break;
        }
    }

    // Check to see if it finished all the handlers without any of them removing themselves.  As soon as the first
    // handler removes itself, we drop out of the original loop and enter this one that will compact the rest of the
    // entries then, then reset inactive_start_ to the appropriate place. This gets rid of the need to constantly delete
    // individual elements, doing multiple memcpy's of all entries below. This also allows us to update the index for
    // each handler.

    int32_t copy_index = call_index;

    // Need to increment call_index for the case where we broke out of the upper loop before completing.  If it did
    // complete, then incrementing it won't hurt anything and the next loop will just be skipped.  It will also be
    // skipped if the break was on the last entry in the vector
    for ( ++call_index; call_index < inactive_start_; ++call_index ) {
        HandlerBase* handler = handlers_[call_index];
        if ( (*handler)(current_cycle) ) {
            // Just mark it as inactive and continue on
            handler->toggleActiveState();
        }
        else {
            // Need to swap this with the handler at copy_index and increment copy_index.  For the swap, the current
            // handler is active and the one at copy_index is inactive, by construction
            swapActiveWithInactive(handlers_[call_index], handlers_[copy_index]);
            ++copy_index;
        }
    }

    // inactive_start_ is now at copy_index
    inactive_start_ = copy_index;

    if ( inactive_start_ == 0 ) {
        return true; // No active handlers
    }
    return false;
}

void
Clock::Group::activateOnRestart()
{
    // Reset all the pointers to the group
    for ( size_t x = 0; x < handlers_.size(); ++x ) {
        handlers_[x]->group_ = this;
    }
    if ( inactive_start_ == 0 ) return;
    clock_->activateGroup(this);
}

void
Clock::Group::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(handlers_);
    SST_SER(inactive_start_);
    SST_SER(handler_count_);
    SST_SER(index_);
    // clock_ gets set after deserialzation when reregistered with the clock
}

Clock::Clock() :
    sim_(Simulation::getSimulation())
{}

Clock::Clock(TimeConverter period, int priority) :
    Action(),
    current_cycle_(0),
    period_(period),
    scheduled_(false),
    sim_(Simulation::getSimulation())
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
    handler->markAsActive();
    handler->index_ = handlers_.size();
    handlers_.push_back(handler);

    if ( !scheduled_ ) {
        schedule();
    }
}

bool
Clock::unregisterHandler(Clock::HandlerBase* handler, bool& empty)
{
    if ( handler->isActive() ) {
        // Need to get index before marking it as inactive, which sets index to -1
        int  index = handler->index_;
        auto iter  = handlers_.begin() + index;
        iter       = handlers_.erase(iter);
        handler->markAsInactive();

        // Need to update the index for every handler below this one
        for ( ; iter != handlers_.end(); ++iter ) {
            (*iter)->index_ = index++;
        }
    }
    empty = handlers_.empty();

    return 0;
}

void
Clock::registerHandler_restart(Clock::HandlerBase* handler)
{
    handler->clock_ = this;
    if ( !handler->isActive() ) return;

    handler->index_ = handlers_.size();
    handlers_.push_back(handler);

    if ( !scheduled_ ) {
        schedule();
    }
}

bool
Clock::isHandlerRegistered(Clock::HandlerBase* handler)
{
    for ( auto* h : handlers_ ) {
        if ( h == handler ) return true;
    }

    return false;
}

void
Clock::registerGroup(Clock::Group* group)
{
    group->clock_ = this;
    group->index_ = groups_.size();

    // Groups start life inactive, so just put at the end of the vector
    groups_.push_back(group);
}

void
Clock::activateGroup(Clock::Group* group)
{
    // Check to see if it's already active
    if ( group->index_ < groups_inactive_start_ ) return;

    // Just swap it with the group at groups_inactive_start_, if necessary, and increment groups_inactive_start_
    if ( group->index_ != groups_inactive_start_ ) {
        // Not in the right place, do the swap
        int32_t index = group->index_;
        std::swap(groups_[index], groups_[groups_inactive_start_]);
        groups_[index]->index_                  = index;
        groups_[groups_inactive_start_]->index_ = groups_inactive_start_;
    }
    ++groups_inactive_start_;
    if ( !scheduled_ ) schedule();
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
    if ( handlers_.empty() && groups_inactive_start_ == 0 ) {
        scheduled_ = false;
        return;
    }

    // Derive the current cycle from the core time
    current_cycle_++;

    auto sop_iter = handlers_.begin();
    for ( ; sop_iter != handlers_.end(); ++sop_iter ) {
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
    if ( sop_iter != handlers_.end() ) {
        // sop_iter becomes the location to copy to and we will start one past this calling the handler.  Then, we will
        // copy the handler to the copy_iter and update its index.
        auto   copy_iter = sop_iter; // Rename for clarity
        size_t index     = copy_iter - handlers_.begin();

        for ( auto call_iter = sop_iter + 1; call_iter != handlers_.end(); ++call_iter ) {
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

        // Need to remove any empty locations in the vector
        handlers_.erase(copy_iter, handlers_.end());
    }


    // Now we need to go through all the active groups.  This loop is a bit weird because each iteration through the
    // loop can do one of too things to the control variables:

    // 1 - increment i if the group does not ask to be removed

    // 2 - decrement groups_inactive_start_ if the group asks to be removed.  This is because the removed group gets
    // moved to groups_inactive_start_ (after decrementing it) and we need to execute the group that just got moved to
    // that index
    for ( int i = 0; i < groups_inactive_start_; /* incremented in loop */ ) {
        if ( groups_[i]->execute(current_cycle_) ) {
            // Need to deactivate this group by swapping it below groups_inactive_start_
            --groups_inactive_start_;
            if ( i != groups_inactive_start_ ) std::swap(groups_[i], groups_[groups_inactive_start_]);
            groups_[i]->index_                      = i;
            groups_[groups_inactive_start_]->index_ = groups_inactive_start_;
            // Don't increment i because we need to execute the one we just swapped into this position
        }
        else {
            ++i;
        }
    }

    // Compute the next time to fire
    next_ = sim_->getCurrentSimCycle() + period_.getFactor();
    sim_->insertActivity(next_, this);

    return;
}

void
Clock::schedule()
{
    current_cycle_ = sim_->getCurrentSimCycle() / period_.getFactor();
    SimTime_t next = (current_cycle_ * period_.getFactor()) + period_.getFactor();

    // Check to see if we need to insert clock into queue at current
    // simtime.  This happens if the clock would have fired at this
    // tick and if the current priority is less than my priority.
    // However, if we are at time = 0, then we always go out to the
    // next cycle. Also, if the call happens during complete() or
    // finish(), then we don't adjust either.
    if ( sim_->getCurrentPriority() < getPriority() && sim_->getCurrentSimCycle() != 0 && !sim_->endSim ) {
        if ( sim_->getCurrentSimCycle() % period_.getFactor() == 0 ) {
            next = sim_->getCurrentSimCycle();
            current_cycle_--; // First thing execute does in increment current_cycle_
        }
    }

    sim_->insertActivity(next, this);
    scheduled_ = true;
}

void
Clock::updateCurrentCycle()
{
    current_cycle_ = sim_->getCurrentSimCycle() / period_.getFactor();
    return;
}

std::string
Clock::toString() const
{
    std::stringstream buf;
    buf << "Clock Activity with period " << period_.getFactor() << " to be delivered at " << getDeliveryTime()
        << " with priority " << getPriority() << " with " << handlers_.size() << " items on clock list";
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
