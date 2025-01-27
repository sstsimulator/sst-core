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

#include "sst/core/interactiveConsole.h"

#include "sst/core/interactiveAction.h"
#include "sst/core/mempoolAccessor.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeLord.h"


namespace SST {


/************ InteractiveConsole ***********/

SST_ELI_DEFINE_INFO_EXTERN(SST::InteractiveConsole)
SST_ELI_DEFINE_CTOR_EXTERN(SST::InteractiveConsole)


InteractiveConsole::InteractiveConsole() {}

InteractiveConsole::~InteractiveConsole() {}

UnitAlgebra
InteractiveConsole::getCoreTimeBase() const
{
    return Simulation_impl::getSimulation()->getTimeLord()->getTimeBase();
}

SimTime_t
InteractiveConsole::getCurrentSimCycle() const
{
    return Simulation_impl::getSimulation()->getCurrentSimCycle();
}

UnitAlgebra
InteractiveConsole::getElapsedSimTime() const
{
    return Simulation_impl::getSimulation()->getElapsedSimTime();
}

SimTime_t
InteractiveConsole::getEndSimCycle() const
{
    return Simulation_impl::getSimulation()->getEndSimCycle();
}

UnitAlgebra
InteractiveConsole::getEndSimTime() const
{
    return Simulation_impl::getSimulation()->getEndSimTime();
}

RankInfo
InteractiveConsole::getRank() const
{
    return Simulation_impl::getSimulation()->getRank();
}

RankInfo
InteractiveConsole::getNumRanks() const
{
    return Simulation_impl::getSimulation()->getNumRanks();
}

Output&
InteractiveConsole::getSimulationOutput() const
{
    return Simulation_impl::getSimulation()->getSimulationOutput();
}

uint64_t
InteractiveConsole::getTimeVortexMaxDepth() const
{
    return Simulation_impl::getSimulation()->getTimeVortexMaxDepth();
}

void
InteractiveConsole::getMemPoolUsage(int64_t& bytes, int64_t& active_entries)
{
    Core::MemPoolAccessor::getMemPoolUsage(bytes, active_entries);
}

uint64_t
InteractiveConsole::getSyncQueueDataSize() const
{
    return Simulation_impl::getSimulation()->getSyncQueueDataSize();
}

TimeConverter*
InteractiveConsole::getTimeConverter(const std::string& time)
{
    return Simulation_impl::getSimulation()->getTimeLord()->getTimeConverter(time);
}

void
InteractiveConsole::schedule_interactive(SimTime_t time_offset, const std::string& msg)
{
    Simulation_impl*   sim = Simulation_impl::getSimulation();
    InteractiveAction* act = new InteractiveAction(sim, msg);
    sim->insertActivity(getCurrentSimCycle() + time_offset, act);
}

SST::Core::Serialization::ObjectMap*
InteractiveConsole::getComponentObjectMap()
{
    return Simulation_impl::getSimulation()->getComponentObjectMap();
}


} /* End namespace SST */
