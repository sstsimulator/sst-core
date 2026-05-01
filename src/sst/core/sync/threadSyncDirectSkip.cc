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

#include "sst/core/sync/threadSyncDirectSkip.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"
#include "sst/core/warnmacros.h"

#include <atomic>

namespace SST {

SimTime_t ThreadSyncDirectSkip::localMinimumNextActivityTime = 0;

/** Create a new ThreadSyncDirectSkip object */
ThreadSyncDirectSkip::ThreadSyncDirectSkip(int num_threads, int thread, Simulation* sim) :
    ThreadSync(),
    num_threads(num_threads),
    thread(thread),
    sim(sim),
    totalWaitTime(0.0)
{
    if ( sim->getRank().thread == 0 ) {
        barrier[0].resize(num_threads);
        barrier[1].resize(num_threads);
        barrier[2].resize(num_threads);
    }

    if ( sim->getNumRanks().rank > 1 )
        single_rank = false;
    else
        single_rank = true;

    my_max_period = sim->getInterThreadMinLatency();
    nextSyncTime  = my_max_period;
}

ThreadSyncDirectSkip::~ThreadSyncDirectSkip()
{
    if ( totalWaitTime > 0.0 )
        Output::getDefaultObject().verbose(
            CALL_INFO, 1, 0, "ThreadSyncDirectSkip total wait time: %lg seconds.\n", totalWaitTime);
}

void
ThreadSyncDirectSkip::after()
{
    // Use this nextSyncTime computation for no skip
    nextSyncTime = sim->getCurrentSimCycle() + my_max_period;

    // Use this nextSyncTime computation for skipping

    // Not sure yet that this works. Need to revisit.  Probably also
    // need to only do it periodically instead of every sync.

    // auto nextmin = sim->getLocalMinimumNextActivityTime();
    // auto nextminPlus = nextmin + my_max_period;
    // nextSyncTime = nextmin > nextminPlus ? nextmin : nextminPlus;
}

void
ThreadSyncDirectSkip::execute()
{
    after();
    totalWaitTime += barrier[2].wait();
}

uint64_t
ThreadSyncDirectSkip::getDataSize() const
{
    size_t count = 0;
    return count;
}

void
ThreadSyncDirectSkip::setSignals(int end, int usr, int alrm)
{
    sig_end_  = end;
    sig_usr_  = usr;
    sig_alrm_ = alrm;
}

bool
ThreadSyncDirectSkip::getSignals(int& end, int& usr, int& alrm)
{
    end  = sig_end_;
    usr  = sig_usr_;
    alrm = sig_alrm_;
    return sig_end_ || sig_usr_ || sig_alrm_;
}

void
ThreadSyncDirectSkip::setShutdownFlags(bool enter_shutdown, Simulation::ShutdownMode_t shutdown_mode)
{
    if ( enter_shutdown ) {
        enter_shutdown_.store(enter_shutdown);
        shutdown_mode_.store(static_cast<unsigned>(shutdown_mode));
    }
}

void
ThreadSyncDirectSkip::setFlags(bool enter_interactive, bool enter_shutdown, Simulation::ShutdownMode_t shutdown_mode)
{
    // This must be atomic because it can be set from any thread
    if ( enter_interactive ) enter_interactive_.store(enter_interactive);
    setShutdownFlags(enter_shutdown, shutdown_mode);
}

void
ThreadSyncDirectSkip::getShutdownFlags(bool& enter_shutdown, Simulation::ShutdownMode_t& shutdown_mode)
{
    enter_shutdown = enter_shutdown_.load();
    switch ( shutdown_mode_ ) {
    case 0:
        shutdown_mode = Simulation::ShutdownMode_t::SHUTDOWN_CLEAN;
        break;
    case 1:
        shutdown_mode = Simulation::ShutdownMode_t::SHUTDOWN_SIGNAL;
        break;
    case 2:
        shutdown_mode = Simulation::ShutdownMode_t::SHUTDOWN_EMERGENCY;
        break;
    }
}

void
ThreadSyncDirectSkip::getFlags(bool& enter_interactive, bool& enter_shutdown, Simulation::ShutdownMode_t& shutdown_mode)
{
    enter_interactive = enter_interactive_.load();
    getShutdownFlags(enter_shutdown, shutdown_mode);
}

void
ThreadSyncDirectSkip::clearFlags()
{
    enter_interactive_.store(false);
    enter_shutdown_.store(false);
    shutdown_mode_.store(0);
}

Core::ThreadSafe::Barrier ThreadSyncDirectSkip::barrier[3];
int                       ThreadSyncDirectSkip::sig_end_(0);
int                       ThreadSyncDirectSkip::sig_usr_(0);
int                       ThreadSyncDirectSkip::sig_alrm_(0);
std::atomic<bool>         ThreadSyncDirectSkip::enter_interactive_(false);
std::atomic<bool>         ThreadSyncDirectSkip::enter_shutdown_(false);
std::atomic<unsigned>     ThreadSyncDirectSkip::shutdown_mode_(0);


} // namespace SST
