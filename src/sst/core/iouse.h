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

#ifndef SST_CORE_IOUSE_H
#define SST_CORE_IOUSE_H

#include <inttypes.h>

namespace SST::Core {

uint64_t maxInputOperations();
uint64_t maxOutputOperations();

} // namespace SST::Core

#endif // SST_CORE_IOUSE_H
