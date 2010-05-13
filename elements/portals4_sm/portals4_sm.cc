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

#include "trig_cpu/trig_cpu.h"
#include "trig_nic/trig_nic.h"

static Component* 
create_trig_cpu(SST::ComponentId_t id, 
                SST::Component::Params_t& params)
{
    return new trig_cpu( id, params );
}

static Component* 
create_trig_nic(SST::ComponentId_t id, 
                SST::Component::Params_t& params)
{
    return new trig_nic( id, params );
}

static const ElementInfoComponent components[] = {
    { "trig_cpu",
      "Triggered CPU for Portals 4 research",
      NULL,
      create_trig_cpu,
    },
    { "trig_nic",
      "Triggered NIC for Portals 4 research",
      NULL,
      create_trig_nic,
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo portals4_sm_eli = {
        "portals4_sm",
        "State-machine based processor/nic for Portals 4 research",
        components,
    };
}
