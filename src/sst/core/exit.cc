// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/exit.h"

#include <sst/core/warnmacros.h>
#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include "sst/core/component.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"
#include "sst/core/stopAction.h"

using SST::Core::ThreadSafe::Spinlock;

namespace SST {

Exit::Exit( int num_threads, TimeConverter* period, bool single_rank ) :
    Action(),
//     m_functor( new EventHandler<Exit,bool,Event*> (this,&Exit::handler ) ),
    num_threads(num_threads),
    m_refCount( 0 ),
    m_period( period ),
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
    
bool Exit::refInc( ComponentId_t id, uint32_t thread )
{
    std::lock_guard<Spinlock> lock(slock);
    if ( m_idSet.find( id ) != m_idSet.end() ) {
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

    m_idSet.insert( id );

    ++m_refCount;
    ++m_thread_counts[thread];
    
    return false;
}

bool Exit::refDec( ComponentId_t id, uint32_t thread )
{
    // TraceFunction trace(CALL_INFO_LONG);
    std::lock_guard<Spinlock> lock(slock);
    if ( m_idSet.find( id ) == m_idSet.end() ) {
        Simulation::getSimulation()->getSimulationOutput().verbose(CALL_INFO, 1, 1, "component (%s) multiple decrement\n",
                Simulation::getSimulation()->getComponent(id)->getName().c_str() );
        return true;
    } 


    if ( m_refCount == 0 ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "refCount is already 0\n" );
        return true;
    }

    m_idSet.erase( id );

    --m_refCount;
    --m_thread_counts[thread];

    if ( single_rank && num_threads == 1 && m_refCount == 0 ) {
        //std::cout << "Exiting..." << std::endl;
        end_time = Simulation::getSimulation()->getCurrentSimCycle();
        Simulation* sim = Simulation::getSimulation();
        // sim->insertActivity( sim->getCurrentSimCycle() + m_period->getFactor(), this );
        sim->insertActivity( sim->getCurrentSimCycle() + 1, this );
    }
    else if ( m_thread_counts[thread] == 0 ) {
        SimTime_t end_time_new = Simulation::getSimulation()->getCurrentSimCycle();
        // trace.getOutput().output(CALL_INFO,"end_time_new = %llu\n",end_time_new);
        // trace.getOutput().output(CALL_INFO,"end_time = %llu\n",end_time);
        if ( end_time_new > end_time ) end_time = end_time_new;
        if ( Simulation::getSimulation()->isIndependentThread() ) {
            // Need to exit just this thread, so we'll need to use a
            // StopAction
            Simulation* sim = Simulation::getSimulation();
            sim->insertActivity( sim->getCurrentSimCycle(), new StopAction() );            
        }
    }    
        
    return false;
}

unsigned int Exit::getRefCount() {
    return m_refCount;
}


void
Exit::execute()
{
    check();

    SimTime_t next = Simulation::getSimulation()->getCurrentSimCycle() + m_period->getFactor();
    Simulation::getSimulation()->insertActivity( next, this );
    
}
    
// bool Exit::handler( Event* e )
void Exit::check()
{
    // TraceFunction trace(CALL_INFO_LONG);
    int value = ( m_refCount > 0 );
    int out;
    
#ifdef SST_CONFIG_HAVE_MPI
    if ( !single_rank ) {
        MPI_Allreduce( &value, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    }
    else {
        out = value;
    }
#else
    out = value;
#endif
    global_count = out;
    // If out is 0, then it's time to end
    if ( !out ) {
#ifdef SST_CONFIG_HAVE_MPI
        // Do an all_reduce to get the end_time
        SimTime_t end_value;
        if ( !single_rank ) {
            MPI_Allreduce( &end_time, &end_value, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
            end_time = end_value;
        }
#endif
        if (single_rank) {
            endSimulation(end_time);
        }
    }
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
