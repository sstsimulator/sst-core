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
#include "sst/core/heartbeat.h"

#include "sst/core/component.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"

#include <sst/core/warnmacros.h>
#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

namespace SST {

SimulatorHeartbeat::SimulatorHeartbeat( Config* UNUSED(cfg), int this_rank, Simulation* sim, TimeConverter* period) :
    Action(),
    rank(this_rank),
    m_period( period )
{
    sim->insertActivity( period->getFactor(), this );
    if( (0 == this_rank) ) {
        lastTime = sst_get_cpu_time();
    }
    // if( (0 == this_rank) ) {
    // 	sim->insertActivity( period->getFactor(), this );
    //     lastTime = sst_get_cpu_time();
    // }
}

SimulatorHeartbeat::~SimulatorHeartbeat() {

}

void SimulatorHeartbeat::execute( void )
{
    Simulation *sim = Simulation::getSimulation();
    const double now = sst_get_cpu_time();

    Output& sim_output = sim->getSimulationOutput();
    if ( 0 == rank ) {
        sim->getSimulationOutput().output("# Simulation Heartbeat: Simulated Time %s (Real CPU time since last period %.5f seconds)\n",
                                          sim->getElapsedSimTime().toStringBestSI().c_str(), (now - lastTime) );

        lastTime = now;
    }

    SimTime_t next = sim->getCurrentSimCycle() + m_period->getFactor();
    sim->insertActivity( next, this );

    // Print some resource usage
    uint64_t local_max_tv_depth = Simulation::getSimulation()->getTimeVortexMaxDepth();
    uint64_t global_max_tv_depth = 0;
    
    uint64_t global_max_sync_data_size = 0, global_sync_data_size = 0;

    uint64_t mempool_size = 0;
    uint64_t active_activities = 0;
#ifdef USE_MEMPOOL
    Activity::getMemPoolUsage(mempool_size, active_activities);
#endif
    uint64_t max_mempool_size, global_mempool_size, global_active_activities;
    
#ifdef SST_CONFIG_HAVE_MPI
    uint64_t local_sync_data_size = Simulation::getSimulation()->getSyncQueueDataSize();

    MPI_Allreduce(&local_max_tv_depth, &global_max_tv_depth, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
    MPI_Allreduce(&local_sync_data_size, &global_max_sync_data_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
    MPI_Allreduce(&local_sync_data_size, &global_sync_data_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
    MPI_Allreduce(&mempool_size, &max_mempool_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
    MPI_Allreduce(&mempool_size, &global_mempool_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
    MPI_Allreduce(&active_activities, &global_active_activities, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
#else
    global_max_tv_depth = local_max_tv_depth;
    global_max_sync_data_size = 0;
    global_max_sync_data_size = 0;
    max_mempool_size = mempool_size;
    global_mempool_size = mempool_size;
    global_active_activities = active_activities;
#endif

        
    if ( rank == 0 ) {
	    char ua_buffer[256];
        
        sprintf(ua_buffer, "%" PRIu64 "B", global_max_sync_data_size);
        UnitAlgebra global_max_sync_data_size_ua(ua_buffer);
        
        sprintf(ua_buffer, "%" PRIu64 "B", global_sync_data_size);
        UnitAlgebra global_sync_data_size_ua(ua_buffer);
        
        sprintf(ua_buffer, "%" PRIu64 "B", max_mempool_size);
        UnitAlgebra max_mempool_size_ua(ua_buffer);
        
        sprintf(ua_buffer, "%" PRIu64 "B", global_mempool_size);
        UnitAlgebra global_mempool_size_ua(ua_buffer);
        
        sim_output.output("\tMax mempool usage:               %s\n",
                          max_mempool_size_ua.toStringBestSI().c_str());
        sim_output.output("\tGlobal mempool usage:            %s\n",
                          global_mempool_size_ua.toStringBestSI().c_str());
        sim_output.output("\tGlobal active activities         %" PRIu64 " activities\n",
                          global_active_activities);
        sim_output.output("\tMax TimeVortex depth:            %" PRIu64 " entries\n",
                          global_max_tv_depth);
        sim_output.output("\tMax Sync data size:              %s\n",
                          global_max_sync_data_size_ua.toStringBestSI().c_str());
        sim_output.output("\tGlobal Sync data size:           %s\n",
                          global_sync_data_size_ua.toStringBestSI().c_str());
        
    }

}

} // namespace SST

