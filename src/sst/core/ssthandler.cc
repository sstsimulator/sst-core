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

#include "sst_config.h"

#include "sst/core/ssthandler.h"

#include "sst/core/sst_types.h"

#include <atomic>
#include <cstdio>

namespace SST {

std::atomic<HandlerId_t> SSTHandlerBaseProfile::HandlerProfileToolList::id_counter(1);

SSTHandlerBaseProfile::HandlerProfileToolList::HandlerProfileToolList()
{
    my_id = id_counter.fetch_add(1);
}

} // namespace SST
