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
//

#include "sst/core/timingOutput.h"

#include "sst/core/simulation_impl.h"
#include "sst/core/stringize.h"
#include "sst/core/util/perfReporter.h"

#include "nlohmann/json.hpp"

#include <cinttypes>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <sys/ioctl.h>

namespace json = ::nlohmann;


namespace SST::Core {

TimingOutput::TimingOutput(const SST::Output& output, int print_verbosity) :
    output_(output),
    print_verbosity_(print_verbosity)
{}

void
TimingOutput::set(Key key, const uint64_t v)
{
    u64map_[key] = v;
}

void
TimingOutput::set(Key key, UnitAlgebra v)
{
    uamap_[key] = v;
}

void
TimingOutput::set(Key key, double v)
{
    dmap_[key] = v;
}

TimingOutput::~TimingOutput() {}


void
TimingOutput::generate(SST::Util::PerfReporter* reporter)
{
    Simulation_impl::basicPerf.outputRegionData(print_verbosity_, reporter);
}

} // namespace SST::Core
