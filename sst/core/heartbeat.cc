// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/heartbeat.h"

#ifdef HAVE_MPI
#include <boost/mpi.hpp>
#endif

#include <sst/core/debug.h>
#include "sst/core/component.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"

namespace SST {

SimulatorHeartbeat::SimulatorHeartbeat( Config* cfg, int this_rank, Simulation* sim, TimeConverter* period) :
    Action(),
    m_period( period )
{
    if(cfg->verbose && (0 == this_rank) ) {
    	sim->insertActivity( period->getFactor(), this );
    }
}

SimulatorHeartbeat::~SimulatorHeartbeat() {

}

void SimulatorHeartbeat::execute( void )
{
    Simulation *sim = Simulation::getSimulation();
    sim->getSimulationOutput().output("# Heartbeat: %" PRIu64 " cycles\n",
	sim->getCurrentSimCycle() );

    SimTime_t next = sim->getCurrentSimCycle() +
	m_period->getFactor();
    sim->insertActivity( next, this );
}


template<class Archive>
void
SimulatorHeartbeat::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
    ar & BOOST_SERIALIZATION_NVP(m_period);
}

} // namespace SST

SST_BOOST_SERIALIZATION_INSTANTIATE(SST::SimulatorHeartbeat::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::SimulatorHeartbeat);
