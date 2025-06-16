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

#include "sst/core/sst_types.h"

#include <ostream>

namespace SST {

std::ostream&
operator<<(std::ostream& os, const SimulationRunMode& mode)
{
    switch ( mode ) {
    case SimulationRunMode::UNKNOWN:
        os << "UNKNOWN";
        break;
    case SimulationRunMode::INIT:
        os << "INIT";
        break;
    case SimulationRunMode::RUN:
        os << "RUN";
        break;
    case SimulationRunMode::BOTH:
        os << "BOTH";
        break;
    default:
        os << "INVALID"; // In case of an invalid value
        break;
    }
    return os;
}

} // namespace SST
