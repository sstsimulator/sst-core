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

#ifndef SST_CORE_RNG_SSTRNG_H
#define SST_CORE_RNG_SSTRNG_H

#warning \
    "sst/core/rng/sstrng.h is deprecated and will be removed in SST 13.  Please use the equivalent functionality in sst/core/rng/rng.h"

#include "sst/core/rng/rng.h"

#include <stdint.h>

namespace SST {
namespace RNG {

using SSTRandom = SST::RNG::Random;

} // namespace RNG
} // namespace SST

#endif // SST_CORE_RNG_SSTRNG_H
