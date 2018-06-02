// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_CORE_MEMUSE
#define _H_SST_CORE_MEMUSE

#include <inttypes.h>

namespace SST {
namespace Core {

uint64_t maxLocalMemSize();
uint64_t maxGlobalMemSize();
uint64_t maxLocalPageFaults();
uint64_t globalPageFaults();

}
}


#endif
