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

#ifndef SST_CORE_REAL_TIME_ACTION_H
#define SST_CORE_REAL_TIME_ACTION_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/threadsafe.h"
#include "sst/core/warnmacros.h"

namespace SST {

class Output;
class RankInfo;
class UnitAlgebra;

/** An event to trigger at a real-time interval */
class RealTimeAction
{
public:
    SST_ELI_DECLARE_BASE(RealTimeAction)
    SST_ELI_DECLARE_DEFAULT_INFO_EXTERN()
    SST_ELI_DECLARE_DEFAULT_CTOR_EXTERN()


    RealTimeAction();

    /* Optional function called just before run loop starts. Passes in
     * the next scheduled time of the event or 0 if the event is not
     * scheduled */
    virtual void begin(time_t UNUSED(scheduled_time)) {}
    virtual void execute() = 0;

    /* Attribute functions that let the core know when certain actions
     * need to be planned for */

    /**
       Let's the core know if this action may trigger a checkpoint so
       that all the checkpoint infrastructure can be initialized.
     */
    virtual bool canInitiateCheckpoint() { return false; }

    /* Accessors for core state that signal handlers may need
     * These accessors return per-thread information unless noted in a comment
     */

    /** Get the core timebase */
    UnitAlgebra getCoreTimeBase() const;
    /** Return the current simulation time as a cycle count*/
    SimTime_t   getCurrentSimCycle() const;
    /** Return the elapsed simulation time as a time */
    UnitAlgebra getElapsedSimTime() const;
    /** Return the end simulation time as a cycle count*/
    SimTime_t   getEndSimCycle() const;
    /** Return the end simulation time as a time */
    UnitAlgebra getEndSimTime() const;
    /** Get this instance's parallel rank */
    RankInfo    getRank() const;
    /** Get the number of parallel ranks in the simulation */
    RankInfo    getNumRanks() const;
    /** Return the base simulation Output class instance */
    Output&     getSimulationOutput() const;
    /** Return the max depth of the TimeVortex */
    uint64_t    getTimeVortexMaxDepth() const;
    /** Return the size of the SyncQueue  - per-rank */
    uint64_t    getSyncQueueDataSize() const;
    /** Return MemPool usage information - per-rank */
    void        getMemPoolUsage(int64_t& bytes, int64_t& active_entries);


    /* Actions that signal handlers can take */

    /** Invokes printStatus on the simulation instance
     * component_status indicates whether printStatus should
     * also be called on all components
     */
    void simulationPrintStatus(bool component_status);

    /** Inform the simulation that a signal requires a shutdown
     * abnormal indicates whether emergencyShutdown() should be called
     */
    void simulationSignalShutdown(bool abnormal);

    /** Generate a checkpoint */
    void simulationCheckpoint();

    void initiateInteractive(const std::string& msg);
};

} // namespace SST

#ifndef SST_ELI_REGISTER_REALTIMEACTION
#define SST_ELI_REGISTER_REALTIMEACTION(cls, lib, name, version, desc) \
    SST_ELI_REGISTER_DERIVED(SST::RealTimeAction,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)
#endif

#endif /* SST_CORE_REAL_TIME_ACTION_H */
