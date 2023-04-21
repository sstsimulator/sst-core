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

#include "sst/core/componentInfo.h"

#include "sst/core/configGraph.h"
#include "sst/core/linkMap.h"

namespace SST {

ComponentInfo::ComponentInfo(ComponentId_t id, const std::string& name) :
    id(id),
    parent_info(nullptr),
    name(name),
    type(""),
    link_map(nullptr),
    component(nullptr),
    params(nullptr),
    defaultTimeBase(nullptr),
    statConfigs(nullptr),
    enabledStatNames(nullptr),
    enabledAllStats(false),
    allStatConfig(nullptr),
    coordinates(3, 0.0),
    subIDIndex(1),
    slot_name(""),
    slot_num(-1),
    share_flags(0)
{}

// ComponentInfo::ComponentInfo(ComponentId_t id, ComponentInfo* parent_info, const std::string& type, const Params
// *params, const ComponentInfo *parent) :
//     id(parent->id),
//     name(parent->name),
//     type(type),
//     link_map(parent->link_map),
//     component(nullptr),
//     params(params),
//     enabledStats(parent->enabledStats),
//     coordinates(parent->coordinates),
//     subIDIndex(1),
//     slot_name(""),
//     slot_num(-1),
//     share_flags(0)
// {
// }

// Constructor used for Anonymous SubComponents
ComponentInfo::ComponentInfo(
    ComponentId_t id, ComponentInfo* parent_info, const std::string& type, const std::string& slot_name, int slot_num,
    uint64_t share_flags /*, const Params& params_in*/) :
    id(id),
    parent_info(parent_info),
    name(""),
    type(type),
    link_map(nullptr),
    component(nullptr),
    params(/*new Params()*/ nullptr),
    defaultTimeBase(nullptr),
    statConfigs(nullptr),
    enabledStatNames(nullptr),
    enabledAllStats(false),
    allStatConfig(nullptr),
    statLoadLevel(0),
    coordinates(parent_info->coordinates),
    subIDIndex(1),
    slot_name(slot_name),
    slot_num(slot_num),
    share_flags(share_flags)
{
    /*params.insert(params_in.getParams());*/
}

ComponentInfo::ComponentInfo(
    ConfigComponent* ccomp, const std::string& name, ComponentInfo* parent_info, LinkMap* link_map) :
    id(ccomp->id),
    parent_info(parent_info),
    name(name),
    type(ccomp->type),
    link_map(link_map),
    component(nullptr),
    params(&ccomp->params),
    defaultTimeBase(nullptr),
    statConfigs(&ccomp->statistics),
    enabledStatNames(&ccomp->enabledStatNames),
    enabledAllStats(ccomp->enabledAllStats),
    allStatConfig(&ccomp->allStatConfig),
    statLoadLevel(ccomp->statLoadLevel),
    coordinates(ccomp->coords),
    subIDIndex(1),
    slot_name(ccomp->name),
    slot_num(ccomp->slot_num),
    share_flags(0)
{
    // printf("ComponentInfo(ConfigComponent): id = %llx\n",ccomp->id);

    // See how many subcomponents are in each slot so we know how to name them
    std::map<std::string, int> counts;
    for ( auto sc : ccomp->subComponents ) {
        counts[sc->name]++;
    }

    for ( auto sc : ccomp->subComponents ) {
        std::string sub_name(name);
        sub_name += ":";
        sub_name += sc->name;
        // If there is more than one subcomponent in this slot, need
        // to add [index] to the end.
        if ( counts[sc->name] > 1 ) {
            sub_name += "[";
            sub_name += std::to_string(sc->slot_num);
            sub_name += "]";
        }
        subComponents.emplace_hint(
            subComponents.end(), std::piecewise_construct, std::make_tuple(sc->id),
            std::forward_as_tuple(sc, sub_name, this, new LinkMap()));
    }
}

ComponentInfo::ComponentInfo(ComponentInfo&& o) :
    id(o.id),
    parent_info(o.parent_info),
    name(std::move(o.name)),
    type(std::move(o.type)),
    link_map(o.link_map),
    component(o.component),
    subComponents(std::move(o.subComponents)),
    params(o.params),
    defaultTimeBase(o.defaultTimeBase),
    statConfigs(o.statConfigs),
    allStatConfig(o.allStatConfig),
    statLoadLevel(o.statLoadLevel),
    coordinates(o.coordinates),
    subIDIndex(o.subIDIndex),
    slot_name(o.slot_name),
    slot_num(o.slot_num),
    share_flags(o.share_flags)
{
    o.parent_info     = nullptr;
    o.link_map        = nullptr;
    o.component       = nullptr;
    o.defaultTimeBase = nullptr;
}

ComponentInfo::~ComponentInfo()
{
    if ( link_map ) delete link_map;
    if ( component ) {
        component->my_info = nullptr;
        delete component;
    }
}

LinkMap*
ComponentInfo::getLinkMap()
{
    if ( link_map == nullptr ) link_map = new LinkMap();
    return link_map;
}

ComponentId_t
ComponentInfo::addAnonymousSubComponent(
    ComponentInfo* parent_info, const std::string& type, const std::string& slot_name, int slot_num,
    uint64_t share_flags)
{
    // First, get the next subIDIndex by working our way up to the
    // actual component (parent pointer will be nullptr).
    ComponentInfo* real_comp = this;
    while ( real_comp->parent_info != nullptr )
        real_comp = real_comp->parent_info;

    // Get the subIDIndex and increment it for next time
    uint64_t sub_id = real_comp->subIDIndex++;

    ComponentId_t cid = COMPDEFINED_SUBCOMPONENT_ID_CREATE(COMPONENT_ID_MASK(id), sub_id);

    subComponents.emplace_hint(
        subComponents.end(), std::piecewise_construct, std::make_tuple(cid),
        std::forward_as_tuple(cid, parent_info, type, slot_name, slot_num, share_flags));

    return cid;
}

void
ComponentInfo::finalizeLinkConfiguration() const
{
    if ( nullptr != link_map ) {
        for ( auto& i : link_map->getLinkMap() ) {
            i.second->finalizeConfiguration();
        }
    }
    for ( auto& s : subComponents ) {
        s.second.finalizeLinkConfiguration();
    }
}

void
ComponentInfo::prepareForComplete() const
{
    if ( nullptr != link_map ) {
        for ( auto& i : link_map->getLinkMap() ) {
            i.second->prepareForComplete();
        }
    }
    for ( auto& s : subComponents ) {
        s.second.prepareForComplete();
    }
}

ComponentInfo*
ComponentInfo::findSubComponent(ComponentId_t id)
{
    /* See if it is us */
    if ( id == this->id ) return this;

    /* Check to make sure we're part of the same component */
    if ( COMPONENT_ID_MASK(id) != COMPONENT_ID_MASK(this->id) ) return nullptr;

    for ( auto& s : subComponents ) {
        ComponentInfo* found = s.second.findSubComponent(id);
        if ( found != nullptr ) return found;
    }
    return nullptr;
}

ComponentInfo*
ComponentInfo::findSubComponent(const std::string& slot, int slot_num)
{
    // Non-recursive, only look in current component
    for ( auto& sc : subComponents ) {
        if ( sc.second.slot_name == slot && sc.second.slot_num == slot_num ) return &sc.second;
    }
    return nullptr;
}

bool
ComponentInfo::hasLinks() const
{
    // If we find any links, return true.  Otherwise return false
    if ( !link_map->empty() ) return true;

    for ( auto& sc : subComponents ) {
        if ( sc.second.hasLinks() ) return true;
    }
    return false;
}

} // namespace SST
