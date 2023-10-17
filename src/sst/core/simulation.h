// -*- c++ -*-

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

#ifndef SST_CORE_SIMULATION_H
#define SST_CORE_SIMULATION_H

#if !SST_BUILDING_CORE
#warning "The Simulation object is being removed as part of the public API and simulation.h will be removed in SST 14"
#endif

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
    virtual ~Simulation() {}


    /********* Public API ************/
    /** Get the run mode of the simulation (e.g. init, run, both etc) */
    virtual SimulationRunMode getSimulationMode() const = 0;

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

    /** Get this instance's parallel rank */
    virtual RankInfo getRank() const = 0;

    /** Get the number of parallel ranks in the simulation */
    virtual RankInfo getNumRanks() const = 0;

    /** Returns the output directory of the simulation
     *  @return Directory in which simulation outputs are placed
     */
    virtual std::string& getOutputDirectory() = 0;

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
