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

#include "sst/core/exit.h"

#include "sst/core/warnmacros.h"
#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include "sst/core/component.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/stopAction.h"
#include "sst/core/timeConverter.h"

using SST::Core::ThreadSafe::Spinlock;

namespace SST {

Exit::Exit(int num_threads, TimeConverter* period, bool single_rank) :
    Action(),
    //     m_functor( new EventHandler<Exit,bool,Event*> (this,&Exit::handler ) ),
    num_threads(num_threads),
    m_refCount(0),
    m_period(period),
    end_time(0),
    single_rank(single_rank)
{
    setPriority(EXITPRIORITY);
    m_thread_counts = new unsigned int[num_threads];
    for ( int i = 0; i < num_threads; i++ ) {
        m_thread_counts[i] = 0;
    }
    // if (!single_rank) sim->insertActivity( period->getFactor(), this );
}

Exit::~Exit()
{
    m_idSet.clear();
}

bool
Exit::refInc(ComponentId_t id, uint32_t thread)
{
    std::lock_guard<Spinlock> lock(slock);
    if ( m_idSet.find(id) != m_idSet.end() ) {
        // CompMap_t comp_map = Simulation::getSimulation()->getComponentMap();
        // bool found_in_map = false;

        // for(CompMap_t::iterator comp_map_itr = comp_map.begin();
        //     comp_map_itr != comp_map.end();
        //     ++comp_map_itr) {

        //     if(comp_map_itr->second->getId() == id) {
        //         found_in_map = true;
        //         break;
        //     }
        // }

        // if(found_in_map) {
        //     _DBG( Exit, "component (%s) multiple increment\n",
        //           Simulation::getSimulation()->getComponent(id)->getName().c_str() );
        // } else {
        //     _DBG( Exit, "component in construction increments exit multiple times.\n" );
        // }
        return true;
    }

    m_idSet.insert(id);

    ++m_refCount;
    ++m_thread_counts[thread];

    return false;
}

bool
Exit::refDec(ComponentId_t id, uint32_t thread)
{
    std::lock_guard<Spinlock> lock(slock);
    if ( m_idSet.find(id) == m_idSet.end() ) {
        Simulation::getSimulation()->getSimulationOutput().verbose(
            CALL_INFO, 1, 1, "component (%s) multiple decrement\n",
            Simulation_impl::getSimulation()->getComponent(id)->getName().c_str());
        return true;
    }

    if ( m_refCount == 0 ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "refCount is already 0\n");
        return true;
    }

    m_idSet.erase(id);

    --m_refCount;
    --m_thread_counts[thread];

    if ( single_rank && num_threads == 1 && m_refCount == 0 ) {
        end_time             = Simulation::getSimulation()->getCurrentSimCycle();
        Simulation_impl* sim = Simulation_impl::getSimulation();
        // sim->insertActivity( sim->getCurrentSimCycle() + m_period->getFactor(), this );
        sim->insertActivity(sim->getCurrentSimCycle() + 1, this);
    }
    else if ( m_thread_counts[thread] == 0 ) {
        SimTime_t end_time_new = Simulation::getSimulation()->getCurrentSimCycle();
        if ( end_time_new > end_time ) end_time = end_time_new;
        if ( Simulation_impl::getSimulation()->isIndependentThread() ) {
            // Need to exit just this thread, so we'll need to use a
            // StopAction
            Simulation_impl* sim = Simulation_impl::getSimulation();
            sim->insertActivity(sim->getCurrentSimCycle(), new StopAction());
        }
    }

    return false;
}

unsigned int
Exit::getRefCount()
{
    return m_refCount;
}

void
Exit::execute()
{
    check();

    SimTime_t next = Simulation::getSimulation()->getCurrentSimCycle() + m_period->getFactor();
    Simulation_impl::getSimulation()->insertActivity(next, this);
}

SimTime_t
Exit::computeEndTime()
{
#ifdef SST_CONFIG_HAVE_MPI
    // Do an all_reduce to get the end_time
    SimTime_t end_value;
    if ( !single_rank ) {
        MPI_Allreduce(&end_time, &end_value, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
        end_time = end_value;
    }
#endif
    if ( single_rank ) { endSimulation(end_time); }
    return end_time;
}

// bool Exit::handler( Event* e )
void
Exit::check()
{
    // TraceFunction trace(CALL_INFO_LONG);
    int value = (m_refCount > 0);
    int out;

#ifdef SST_CONFIG_HAVE_MPI
    if ( !single_rank ) { MPI_Allreduce(&value, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); }
    else {
        out = value;
    }
#else
    out = value;
#endif
    global_count = out;
    // If out is 0, then it's time to end
    if ( !out ) { computeEndTime(); }
    // else {
    //     // Reinsert into TimeVortex.  We do this even when ending so that
    //     // it will get deleted with the TimeVortex on termination.  We do
    //     // this in case we exit with a StopEvent instead.  In that case,
    //     // there is no way to know if the Exit object is deleted in the
    //     // TimeVortex or not, so we just make sure it is always deleted
    //     // there.
    //     Simulation *sim = Simulation::getSimulation();
    //     SimTime_t next = sim->getCurrentSimCycle() +
    //         m_period->getFactor();
    //     sim->insertActivity( next, this );
    // }
}

} // namespace SST
