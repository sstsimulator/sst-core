// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/timeVortex.h"

#include "sst/core/event.h"
#include "sst/core/simulation_impl.h"

namespace SST {

namespace TV {
namespace pvt {

void
pack_timevortex(TimeVortex*& s, SST::Core::Serialization::serializer& ser)
{
    std::string type = Simulation_impl::getSimulation()->timeVortexType;
    ser&        type;
    s->serialize_order(ser);
}

void
unpack_timevortex(TimeVortex*& s, SST::Core::Serialization::serializer& ser)
{
    std::string tv_type;
    ser&        tv_type;

    Params p;
    s = Factory::getFactory()->Create<TimeVortex>(tv_type, p);
    s->serialize_order(ser);
}

} // namespace pvt
} // namespace TV

TimeVortex::TimeVortex()
{
    max_depth = MAX_SIMTIME_T;
    // sim_ = Simulation_impl::getSimulation();
}

void
TimeVortex::fixup(Activity* act)
{
    if ( !sim_ ) sim_ = Simulation_impl::getSimulation();

    Event* ev = dynamic_cast<Event*>(act);
    // Only need to fix up events
    if ( !ev ) return;

    int count = sim_->event_handler_restart_tracking.count(ev->delivery_info);
    if ( !count ) {
        Simulation_impl::getSimulation()->sim_output.fatal(CALL_INFO, 1, "ERROR: Handler tag not found\n");
        // Need to abort here
        return;
    }
    ev->delivery_info = sim_->event_handler_restart_tracking[ev->delivery_info];
}

SST_ELI_DEFINE_CTOR_EXTERN(TimeVortex)
SST_ELI_DEFINE_INFO_EXTERN(TimeVortex)

} // namespace SST
