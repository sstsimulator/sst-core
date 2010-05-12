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

#include "xbarV2.h"

extern "C" {
XbarV2*
xbarV2AllocComponent( SST::ComponentId_t id,
                      SST::Component::Params_t& params )
{
    return new XbarV2( id, params );
}
}

