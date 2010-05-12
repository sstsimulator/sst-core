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


#include <sst_config.h>

#include "xbarShell.h"
#include <sst/memEvent.h>

extern "C" {
XbarShell*
xbarShellAllocComponent( SST::ComponentId_t id,
                         SST::Component::Params_t& params )
{
    return new XbarShell( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(XbarShell)
#endif
