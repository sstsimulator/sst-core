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

#include "sst/core/timeVortex.h"

#include "sst/core/event.h"
#include "sst/core/simulation_impl.h"

#include <string>

namespace SST {
namespace TV::pvt {

void
pack_timevortex(TimeVortex*& s, SST::Core::Serialization::serializer& ser)
{
    std::string& type = Simulation_impl::getSimulation()->timeVortexType;
    SST_SER(type);
    s->serialize_order(ser);
}

void
unpack_timevortex(TimeVortex*& s, SST::Core::Serialization::serializer& ser)
{
    std::string tv_type;
    SST_SER(tv_type);

    Params p;
    s = Factory::getFactory()->Create<TimeVortex>(tv_type, p);
    s->serialize_order(ser);
}

} // namespace TV::pvt

TimeVortex::TimeVortex()
{
    max_depth = MAX_SIMTIME_T;
    // sim_ = Simulation_impl::getSimulation();
}

void
TimeVortex::print(Output& out) const
{
    // Default implementation for print.  Derived classes can override
    // for a more efficient implementation

    // Get the contents of the timevortex and make a copy of it.  This
    // may not be sorted.
    std::vector<Activity*> contents;
    getContents(contents);

    std::sort(contents.begin(), contents.end(), Activity::less<true, true, true>());

    for ( auto it = contents.begin(); it != contents.end(); it++ ) {
        (*it)->print("  ", out);
    }
}


SST_ELI_DEFINE_CTOR_EXTERN(TimeVortex)
SST_ELI_DEFINE_INFO_EXTERN(TimeVortex)

} // namespace SST
