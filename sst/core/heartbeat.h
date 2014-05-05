// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_HEARTBEAT_H
#define SST_CORE_CORE_HEARTBEAT_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>
#include <sst/core/output.h>
#include <sst/core/config.h>
#include <sst/core/cputimer.h>

#ifdef HAVE_MPI
#include <boost/mpi.hpp>
#endif

#include <set>

#include <sst/core/action.h>

namespace SST {

class Simulation;
class TimeConverter;

/**
  \class SimulatorHeartbeat
	An optional heartbeat to show progress in a simulation
*/
class SimulatorHeartbeat : public Action {
public:
    /**
	Create a new heartbeat object for the simulation core to show progress
    */
    SimulatorHeartbeat( Config* cfg, int this_rank, Simulation* sim, TimeConverter* period);
    ~SimulatorHeartbeat();

private:
    SimulatorHeartbeat() { };
    SimulatorHeartbeat(const SimulatorHeartbeat&);
    void operator=(SimulatorHeartbeat const&);
    void execute(void);
    TimeConverter*  m_period;
    double lastTime;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::SimulatorHeartbeat)

#endif // SST_CORE_HEARTBEAT_H
