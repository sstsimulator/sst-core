// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
// #include "sst/core/serialization.h"

#include <sst/core/componentInfo.h>
#include <sst/core/linkMap.h>

namespace SST {

ComponentInfo::ComponentInfo(ComponentId_t id, const std::string &name, const std::string &type, LinkMap* link_map) :
    id(id),
    name(name),
    type(type),
    link_map(link_map),
    component(NULL),
    enabledStats(NULL),
    statParams(NULL)
{ }


ComponentInfo::ComponentInfo(const ConfigComponent *ccomp, LinkMap* link_map) :
    id(ccomp->id),
    name(ccomp->name),
    type(ccomp->type),
    link_map(link_map),
    component(NULL),
    enabledStats(NULL),
    statParams(NULL)
{
    for ( auto sc : ccomp->subComponents ) {
        subComponents.emplace(std::make_pair(sc.first, ComponentInfo(&sc.second, new LinkMap())));
    }
}


ComponentInfo::~ComponentInfo() {
    delete link_map;
    delete component;
}

} // namespace SST

// BOOST_CLASS_EXPORT_IMPLEMENT(SST::InitQueue)
