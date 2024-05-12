// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_EXIT_H
#define SST_CORE_CORE_EXIT_H

#include "sst/core/action.h"
#include "sst/core/sst_types.h"
#include "sst/core/threadsafe.h"

#include <cinttypes>
#include <unordered_set>

namespace SST {

#define _EXIT_DBG(fmt, args...) __DBG(DBG_EXIT, Exit, fmt, ##args)

class Simulation;
class TimeConverter;

/**
 * Exit Event Action
 *
 * Causes the simulation to halt
 */
class Exit : public Action
{
public:
    /**
     * Create a new ExitEvent
     * @param sim Simulation object
     * @param single_rank True if there are no parallel ranks
     *
     *  Exit needs to register a handler during constructor time, which
     * requires a simulation object.  But the simulation class creates
     * an Exit object during it's construction, meaning that
     * Simulation::getSimulation() won't work yet.  So Exit is the one
     * exception to the "constructors shouldn't take simulation
     * pointers" rule.  However, it still needs to follow the "classes
     * shouldn't contain pointers back to Simulation" rule.
     */
    Exit(int num_threads, bool single_rank);
    ~Exit();

    /** Increment Reference Count for a given Component ID */
    bool refInc(ComponentId_t, uint32_t thread);
    /** Decrement Reference Count for a given Component ID */
    bool refDec(ComponentId_t, uint32_t thread);

    unsigned int getRefCount();

    /** Gets the end time of the simulation
     * @return Time when simulation ends
     */
    SimTime_t getEndTime() { return end_time; }

    /** Stores the time the simulation has ended
     * @param time Current simulation time
     * @return void
     */
    void setEndTime(SimTime_t time) { end_time = time; }

    /** Computes the end time of the simulation
     * @return End time of the simulation
     */
    SimTime_t computeEndTime();
    void      execute(void) override;
    void      check();

    /**
     * @param header String to preface the exit action log
     * @param out SST Output logger object
     * @return void
     */
    void print(const std::string& header, Output& out) const override
    {
        out.output(
            "%s Exit Action to be delivered at %" PRIu64 " with priority %d\n", header.c_str(), getDeliveryTime(),
            getPriority());
    }


    unsigned int getGlobalCount() { return global_count; }

    /**
     *
     * TODO to enable different partitioning on restart, will need to associate m_thread_counts and
     * m_idSet back to components so that a new Exit event can be generated on restart
     */
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Exit)
private:
    Exit() {}                    // for serialization only
    Exit(const Exit&);           // Don't implement
    void operator=(Exit const&); // Don't implement

    //     bool handler( Event* );

    //     EventHandler< Exit, bool, Event* >* m_functor;
    int                               num_threads;
    unsigned int                      m_refCount;
    unsigned int*                     m_thread_counts;
    unsigned int                      global_count;
    std::unordered_set<ComponentId_t> m_idSet;
    SimTime_t                         end_time;

    Core::ThreadSafe::Spinlock slock;

    bool single_rank;
};

} // namespace SST

#endif // SST_CORE_EXIT_H
