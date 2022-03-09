// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "sst/core/simulation_impl.h"
#include "sst/core/timeConverter.h"
#include "sst/core/warnmacros.h"

namespace SST {

SimTime_t ThreadSyncDirectSkip::localMinimumNextActivityTime = 0;

/** Create a new ThreadSyncDirectSkip object */
ThreadSyncDirectSkip::ThreadSyncDirectSkip(int num_threads, int thread, Simulation_impl* sim) :
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

// ActivityQueue*
// ThreadSyncDirectSkip::getQueueForThread(int UNUSED(tid))
// {
//     return nullptr;
// }

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


Core::ThreadSafe::Barrier ThreadSyncDirectSkip::barrier[3];

} // namespace SST
