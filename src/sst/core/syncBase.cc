// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/syncBase.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"

namespace SST {


void
SyncBase::setMaxPeriod(TimeConverter* period)
{
    max_period = period;
    // Simulation *sim = Simulation::getSimulation();
    // SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
    // setPriority(SYNCPRIORITY);
    // sim->insertActivity( next, this );        
}
    
void
SyncBase::sendInitData_sync(Link* link, Event* init_data)
{
    link->sendInitData_sync(init_data);
}
    
void
SyncBase::finalizeConfiguration(Link* link)
{
    link->finalizeConfiguration();
}


} // namespace SST


