// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_EXITEVENT_H
#define SST_EXITEVENT_H

#include <sst/core/event.h>

namespace SST {

#define _EXIT_DBG( fmt, args...) __DBG( DBG_EXIT, ExitEvent, fmt, ## args )

class ExitEvent : public Event
{
public:
    ExitEvent() { }
};

} // namespace SST

#endif // SST_EXITEVENT_H
