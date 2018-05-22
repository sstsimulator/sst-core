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
// #include "sst/core/serialization.h"

#include <sst/core/componentInfo.h>
#include <sst/core/configGraph.h>
#include <sst/core/linkMap.h>

namespace SST {

ComponentInfo::ComponentInfo(ComponentId_t id, const std::string &name) :
    id(id),
    name(name),
    type(""),
    link_map(NULL),
    component(NULL),
    params(NULL),
    enabledStats(NULL),
    coordinates(3, 0.0)
{
}


ComponentInfo::ComponentInfo(const std::string &type, const Params *params, const ComponentInfo *parent) :
    id(parent->id),
    name(parent->name),
    type(type),
    link_map(parent->link_map),
    component(NULL),
    params(params),
    enabledStats(parent->enabledStats),
    coordinates(parent->coordinates)
{
}


ComponentInfo::ComponentInfo(ConfigComponent *ccomp, const std::string& name, LinkMap* link_map) :
    id(ccomp->id),
    name(name),
    slot_name(ccomp->name),
    slot_num(ccomp->slot_num),
    type(ccomp->type),
    link_map(link_map),
    component(NULL),
    params(&ccomp->params),
    enabledStats(&ccomp->enabledStatistics),
    coordinates(ccomp->coords)
{

    // See how many subcomponents are in each slot so we know how to name them
    std::map<std::string, int> counts;
    for ( auto &sc : ccomp->subComponents ) {
        counts[sc.name]++;
    }
    
    
    for ( auto &sc : ccomp->subComponents ) {
        std::string sub_name(name);
        sub_name += ":";
        sub_name += sc.name;
        // If there is more than one subcomponent in this slot, need
        // to add [index] to the end.
        if ( counts[sc.name] > 1 ) {
            sub_name += "[";
            sub_name += std::to_string(sc.slot_num);
            sub_name += "]";
        }
        subComponents.emplace_back(ComponentInfo(&sc, sub_name, new LinkMap()));

    }   
}

ComponentInfo::ComponentInfo(ComponentInfo &&o) :
    id(o.id),
    name(std::move(o.name)),
    slot_name(o.slot_name),
    slot_num(o.slot_num),
    type(std::move(o.type)),
    link_map(o.link_map),
    component(o.component),
    params(o.params),
    enabledStats(o.enabledStats),
    coordinates(o.coordinates)
{
    o.link_map = NULL;
    o.component = NULL;
}


ComponentInfo::~ComponentInfo() {
    if ( link_map ) delete link_map;
    if ( component ) delete component;
}

void ComponentInfo::finalizeLinkConfiguration() {
    for ( auto & i : link_map->getLinkMap() ) {
        i.second->finalizeConfiguration();
    }
    for ( auto &s : subComponents ) {
        s.finalizeLinkConfiguration();
    }
}

void ComponentInfo::prepareForComplete() {
    for ( auto & i : link_map->getLinkMap() ) {
        i.second->prepareForComplete();
    }
    for ( auto &s : subComponents ) {
        s.prepareForComplete();
    }
}

ComponentInfo* ComponentInfo::findSubComponent(ComponentId_t id)
{
    /* See if it is us */
    if ( id == this->id )
        return this;

    /* Check to make sure we're part of the same component */
    if ( COMPONENT_ID_MASK(id) != COMPONENT_ID_MASK(this->id) )
        return NULL;

    for ( auto &s : subComponents ) {
        ComponentInfo* found = s.findSubComponent(id);
        if ( found != NULL )
            return found;
    }
    return NULL;
}

ComponentInfo* ComponentInfo::findSubComponent(std::string slot, int slot_num)
{
    // Non-recursive, only look in current component
    for ( auto &sc : subComponents ) {
        if ( sc.slot_name == slot && sc.slot_num == slot_num ) return &sc;
    }
    return NULL;
}


std::vector<LinkId_t> ComponentInfo::getAllLinkIds() const
{
    std::vector<LinkId_t> res;
    for ( auto & l : link_map->getLinkMap() ) {
        res.push_back(l.second->id);
    }
    for ( auto& sc : subComponents ) {
        std::vector<LinkId_t> s = sc.getAllLinkIds();
        res.insert(res.end(), s.begin(), s.end());
    }
    return res;
}

} // namespace SST

// BOOST_CLASS_EXPORT_IMPLEMENT(SST::InitQueue)
