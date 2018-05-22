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

#include "sst_config.h"
#include "sst/core/syncBase.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"

namespace SST {


void
SyncBase::setMaxPeriod(TimeConverter* period)
{
    max_period = period;
}
    
void
SyncBase::sendUntimedData_sync(Link* link, Event* data)
{
    link->sendUntimedData_sync(data);
}
    
void
SyncBase::finalizeConfiguration(Link* link)
{
    link->finalizeConfiguration();
}


} // namespace SST


