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

#ifndef SST_CORE_CORE_EXIT_H
#define SST_CORE_CORE_EXIT_H

#include "sst/core/action.h"
#include "sst/core/sst_types.h"
#include "sst/core/threadsafe.h"

#include <cinttypes>
#include <string>
#include <unordered_set>

namespace SST {

class Simulation;

/**
   Exit Action

   Tracks the number of components that have requested DoNotEndSim.
   Once that count reaches 0, the Exit object will end the simulation.
 */
class Exit : public Action
{
public:
    /**
       Create a new ExitEvent

       @param num_threads number of threads on the rank
       @param single_rank True if there are no other ranks
    */
    Exit(int num_threads, bool single_rank);

    ~Exit();

    /**
       Increment reference count on a given thread

       @param thread number of Component making call
    */
    void refInc(uint32_t thread);

    /**
       Decrement reference count on a given thread

       @param thread number of Component making call
    */
    void refDec(uint32_t thread);

    /**
       Get the current local reference count

       @return current local reference count
     */
    unsigned int getRefCount();

    /**
       Gets the end time of the simulation

       @return Time when simulation ends
    */
    SimTime_t getEndTime() { return end_time_; }

    /**
       Stores the time the simulation has ended

       @param time Current simulation time
    */
    void setEndTime(SimTime_t time) { end_time_ = time; }

    /**
       Computes the end time of the simulation.  This call
       synchronizes across ranks.

       @return End time of the simulation
    */
    SimTime_t computeEndTime();

    /**
       Function called when Exit fires in the TimeVortex
    */
    void execute() override;

    /**
       Function called by SyncManager to check to see if it's time to
       exit.  This call synchronizes across ranks.
    */
    void check();

    /**
       Creates a string representation of the Exit object

       @return string representation of Exit object
    */
    std::string toString() const override;

    /**
       Get the global ref_count.  This is only valid after check() is
       called

       @return global ref_count
    */
    unsigned int getGlobalCount() { return global_count_; }

    // Exit should not be serialized. It will be created new on
    // restart and Components store there primary component state and
    // reregister with Exit on restart.
    NotSerializable(SST::Exit)

private:
    Exit()                       = delete;
    Exit(const Exit&)            = delete;
    Exit& operator=(const Exit&) = delete;

    int           num_threads_   = 0;
    unsigned int  ref_count_     = 0;
    unsigned int* thread_counts_ = nullptr;
    unsigned int  global_count_  = 0;
    SimTime_t     end_time_      = 0;
    bool          single_rank_   = true;

    Core::ThreadSafe::Spinlock slock_;
};

} // namespace SST

#endif // SST_CORE_EXIT_H
