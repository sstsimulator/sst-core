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

#include "sst/core/exit.h"

#include "sst/core/component.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_mpi.h"
#include "sst/core/stopAction.h"

#include <cstdint>
#include <mutex>

using SST::Core::ThreadSafe::Spinlock;

namespace SST {

Exit::Exit(int num_threads, bool single_rank) :
    Action(),
    num_threads_(num_threads),
    single_rank_(single_rank)
{
    setPriority(EXITPRIORITY);
    thread_counts_ = new unsigned int[num_threads_];
    for ( int i = 0; i < num_threads_; i++ ) {
        thread_counts_[i] = 0;
    }
}

Exit::~Exit()
{
    delete[] thread_counts_;
}

void
Exit::refInc(uint32_t thread)
{
    std::lock_guard<Spinlock> lock(slock_);
    ++ref_count_;
    ++thread_counts_[thread];
}

void
Exit::refDec(uint32_t thread)
{
    std::lock_guard<Spinlock> lock(slock_);
    if ( ref_count_ == 0 ) {
        Simulation_impl::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "ref_count is already 0\n");
    }

    --ref_count_;
    --thread_counts_[thread];

    if ( single_rank_ && num_threads_ == 1 && ref_count_ == 0 ) {
        end_time_            = Simulation_impl::getSimulation()->getCurrentSimCycle();
        Simulation_impl* sim = Simulation_impl::getSimulation();
        sim->insertActivity(sim->getCurrentSimCycle() + 1, this);
    }
    else if ( thread_counts_[thread] == 0 ) {
        SimTime_t end_time_new = Simulation_impl::getSimulation()->getCurrentSimCycle();
        if ( end_time_new > end_time_ ) end_time_ = end_time_new;
        if ( Simulation_impl::getSimulation()->isIndependentThread() ) {
            // Need to exit just this thread, so we'll need to use a
            // StopAction
            Simulation_impl* sim = Simulation_impl::getSimulation();
            sim->insertActivity(sim->getCurrentSimCycle(), new StopAction());
        }
    }
}

unsigned int
Exit::getRefCount()
{
    return ref_count_;
}

void
Exit::execute()
{
    // Only gets put into queue once, no need to reschedule
    check();
}

SimTime_t
Exit::computeEndTime()
{
#ifdef SST_CONFIG_HAVE_MPI
    // Do an all_reduce to get the end_time
    SimTime_t end_value;
    if ( !single_rank_ ) {
        MPI_Allreduce(&end_time_, &end_value, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
        end_time_ = end_value;
    }
#endif
    if ( single_rank_ ) {
        endSimulation(end_time_);
    }
    return end_time_;
}

void
Exit::check()
{
    int value = (ref_count_ > 0);
    int out;

#ifdef SST_CONFIG_HAVE_MPI
    if ( !single_rank_ ) {
        MPI_Allreduce(&value, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    }
    else {
        out = value;
    }
#else
    out = value;
#endif
    global_count_ = out;
    // If out is 0, then it's time to end
    if ( !out ) {
        computeEndTime();
    }
}

std::string
Exit::toString() const
{
    std::stringstream buf;
    buf << "Exit Action to be delivered at " << getDeliveryTime() << " with priority " << getPriority();
    return buf.str();
}

} // namespace SST
