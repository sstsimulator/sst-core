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

#ifndef SST_CORE_INTERACTIVE_CONSOLE_H
#define SST_CORE_INTERACTIVE_CONSOLE_H

#include "sst/core/action.h"
#include "sst/core/config.h"
#include "sst/core/cputimer.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/output.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/threadsafe.h"
#include "sst/core/unitAlgebra.h"

#include <set>

namespace SST {

class Params;
class Simulation_impl;
class TimeConverter;

namespace Interactive {
/* Utility functions needed to manage directories */


} // namespace Interactive

/**
  \class CheckpointAction
    A recurring event to trigger checkpoint generation
*/
class InteractiveConsole
{
public:
    // Make this an ELI base
    SST_ELI_DECLARE_BASE(InteractiveConsole)
    SST_ELI_DECLARE_CTOR_EXTERN(SST::Params&)
    SST_ELI_DECLARE_INFO_EXTERN(
        ELI::ProvidesParams
    )

    /**
    Create a new checkpoint object for the simulation core to initiate checkpoints
    */
    InteractiveConsole();
    virtual ~InteractiveConsole();

    /** Called by TimeVortex to trigger checkpoint on simulation clock interval - not used in parallel simulation */
    virtual void execute(const std::string& msg) = 0;

protected:
    // Functions that can be called by child class

    // Informational functions
    /** Get the core timebase */
    UnitAlgebra getCoreTimeBase() const;

    /** Return the current simulation time as a cycle count*/
    SimTime_t getCurrentSimCycle() const;

    /** Return the elapsed simulation time as a time */
    UnitAlgebra getElapsedSimTime() const;

    /** Return the end simulation time as a cycle count*/
    SimTime_t getEndSimCycle() const;

    /** Return the end simulation time as a time */
    UnitAlgebra getEndSimTime() const;

    /** Get this instance's parallel rank */
    RankInfo getRank() const;

    /** Get the number of parallel ranks in the simulation */
    RankInfo getNumRanks() const;

    /** Return the base simulation Output class instance */
    Output& getSimulationOutput() const;

    /** Return the max depth of the TimeVortex */
    uint64_t getTimeVortexMaxDepth() const;

    /** Return the size of the SyncQueue  - per-rank */
    uint64_t getSyncQueueDataSize() const;

    /** Return MemPool usage information - per-rank */
    void getMemPoolUsage(int64_t& bytes, int64_t& active_entries);

    /** Get a TimeConverter */
    TimeConverter* getTimeConverter(const std::string& time);

    /** Get the list of Components */
    void getComponentList(std::vector<std::pair<std::string, SST::Component*>>& vec);

    // Actions that an InteractiveAction can take

    /**
       Runs the simulation the specified number of core time base
       units

       @param time Time in units of core time base
     */
    void simulationRun(SimTime_t time);

    /**
       Schedules the action for execution at the current simulation
       time plus time_offset

       @param time_offset Time in units of core time base
     */
    void schedule_interactive(SimTime_t time_offset, const std::string& msg);

    SST::Core::Serialization::ObjectMap* getComponentObjectMap();

private:
    InteractiveConsole(const InteractiveConsole&) = delete;
    InteractiveConsole& operator=(const InteractiveConsole&) = delete;
};

} // namespace SST

#ifndef SST_ELI_REGISTER_INTERACTIVE_CONSOLE
#define SST_ELI_REGISTER_INTERACTIVE_CONSOLE(cls, lib, name, version, desc) \
    SST_ELI_REGISTER_DERIVED(SST::InteractiveConsole,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)
#endif


#endif // SST_CORE_INTERACTIVE_CONSOLE_H
