// -*- c++ -*-

// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SIMULATION_H
#define SST_CORE_SIMULATION_H

#include "sst/core/output.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/unitAlgebra.h"

namespace SST {

// #define _SIM_DBG( fmt, args...) __DBG( DBG_SIM, Sim, fmt, ## args )
#define STATALLFLAG "--ALLSTATS--"

class Output;
class TimeLord;

/**
 * Main control class for a SST Simulation.
 * Provides base features for managing the simulation
 */
class Simulation
{
public:
    /** Type of Run Modes */
    typedef enum {
        UNKNOWN, /*!< Unknown mode - Invalid for running */
        INIT,    /*!< Initialize-only.  Useful for debugging initialization and graph generation */
        RUN,     /*!< Run-only.  Useful when restoring from a checkpoint (not currently supported) */
        BOTH     /*!< Default.  Both initialize and Run the simulation */
    } Mode_t;

    virtual ~Simulation();

    /********* Public Static API ************/

    /** Return a pointer to the singleton instance of the Simulation */
    static Simulation* getSimulation()
        __attribute__((deprecated("Element facing Simulation class APIs are deprecated and have moved to the various "
                                  "element base classes.  The APIs will be removed in SST 13.")));

    /** Return the TimeLord associated with this Simulation */
    static TimeLord* getTimeLord(void)
        __attribute__((deprecated("Element facing Simulation class APIs are deprecated and have moved to the various "
                                  "element base classes.  The APIs will be removed in SST 13.")));

    /** Return the base simulation Output class instance */
    static Output& getSimulationOutput()
        __attribute__((deprecated("Element facing Simulation class APIs are deprecated and have moved to the various "
                                  "element base classes.  The APIs will be removed in SST 13.")));

    /********* Public API ************/
    /** Get the run mode of the simulation (e.g. init, run, both etc) */
    virtual Mode_t getSimulationMode() const = 0;

    /** Return the current simulation time as a cycle count*/
    virtual SimTime_t getCurrentSimCycle() const = 0;

    /** Return the end simulation time as a cycle count*/
    virtual SimTime_t getEndSimCycle() const = 0;

    /** Return the current priority */
    virtual int getCurrentPriority() const = 0;

    /** Return the elapsed simulation time as a time */
    virtual UnitAlgebra getElapsedSimTime() const = 0;

    /** Return the end simulation time as a time */
    virtual UnitAlgebra getEndSimTime() const = 0;

    /** Return the end simulation time as a time */
    virtual UnitAlgebra getFinalSimTime() const
        __attribute__((deprecated("getFinalSimTime() has been deprecated and will be removed in SST 13.  It has been "
                                  "replaced by getEndSimTime()."))) = 0;

    /** Get this instance's parallel rank */
    virtual RankInfo getRank() const = 0;

    /** Get the number of parallel ranks in the simulation */
    virtual RankInfo getNumRanks() const = 0;

    /** Returns the output directory of the simulation
     *  @return Directory in which simulation outputs are placed
     */
    virtual std::string& getOutputDirectory() = 0;

    /** Signifies that an event type is required for this simulation
     *  Causes the Factory to verify that the required event type can be found.
     *  @param name fully qualified libraryName.EventName
     */
    virtual void requireEvent(const std::string& name) __attribute__((deprecated(
        "requireEvent() has been deprecated in favor or requireLibrary() and will be removed in SST 13."))) = 0;

    /** Signifies that a library is required for this simulation.
     *  @param name Name of the library
     */
    virtual void requireLibrary(const std::string& name) = 0;

    /** Causes the current status of the simulation to be printed to stderr.
     * @param fullStatus - if true, call printStatus() on all components as well
     *        as print the base Simulation's status
     */
    virtual void printStatus(bool fullStatus) = 0;

    /** Get the amount of real-time spent executing the run phase of
     * the simulation.
     *
     * @return real-time in seconds spent executing the run phase
     */
    virtual double getRunPhaseElapsedRealTime() const = 0;

    /** Get the amount of real-time spent executing the init phase of
     * the simulation.
     *
     * @return real-time in seconds spent executing the init phase
     */
    virtual double getInitPhaseElapsedRealTime() const = 0;

    /** Get the amount of real-time spent executing the complete phase of
     * the simulation.
     *
     * @return real-time in seconds spent executing the complete phase
     */
    virtual double getCompletePhaseElapsedRealTime() const = 0;

protected:
    Simulation() {}
    // Simulation(Config* config, RankInfo my_rank, RankInfo num_ranks);
    Simulation(Simulation const&);     // Don't Implement
    void operator=(Simulation const&); // Don't implement
};

} // namespace SST

#endif // SST_CORE_SIMULATION_H
