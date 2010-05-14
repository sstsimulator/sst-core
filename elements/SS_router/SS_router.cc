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

#include <sst/element.h>
#include "elements/SS_router/SS_router/SS_router.h"
#include "elements/SS_router/test_driver/RtrIF.h"

static Component* 
create_router(SST::ComponentId_t id, 
                SST::Component::Params_t& params)
{
    return new SS_router( id, params );
}

static Component* 
create_test_driver(SST::ComponentId_t id, 
                SST::Component::Params_t& params)
{
    return new RtrIF( id, params );
}

static const ElementInfoComponent components[] = {
    { "SS_router",
      "Cycle-accurate 3D torus network router",
      NULL,
      create_router
    },
    { "test_driver",
      "test driver for the SS_router",
      NULL,
      create_test_driver
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo SS_router_eli = {
        "SS_router",
        "Cycle-accurate 3D torus network router",
        components,
    };
}

