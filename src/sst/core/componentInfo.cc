// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/componentInfo.h"

#include "sst/core/configGraph.h"
#include "sst/core/linkMap.h"
#include "sst/core/serialization/serialize.h"
#include "sst/core/serialization/serializer.h"

#include <cinttypes>
#include <cstdio>

namespace SST {

ComponentInfo::ComponentInfo(ComponentId_t id, const std::string& name) :
    id_(id),
    parent_info(nullptr),
    name(name),
    type(""),
    link_map(nullptr),
    component(nullptr),
    params(nullptr),
    stat_configs_(nullptr),
    enabled_stat_names_(nullptr),
    enabled_all_stats_(false),
    all_stat_config_(nullptr),
    statLoadLevel(0),
    coordinates(3, 0.0),
    subIDIndex(1),
    slot_name(""),
    slot_num(-1),
    share_flags(0)
{}

ComponentInfo::ComponentInfo() :
    id_(-1),
    parent_info(nullptr),
    name(""),
    type(""),
    link_map(nullptr),
    component(nullptr),
    params(nullptr),
    stat_configs_(nullptr),
    enabled_stat_names_(nullptr),
    enabled_all_stats_(false),
    all_stat_config_(nullptr),
    statLoadLevel(0),
    coordinates(3, 0.0),
    subIDIndex(1),
    slot_name(""),
    slot_num(-1),
    share_flags(0)
{}

// ComponentInfo::ComponentInfo(ComponentId_t id, ComponentInfo* parent_info, const std::string& type, const Params
// *params, const ComponentInfo *parent) :
//     id_(parent->id),
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
ComponentInfo::ComponentInfo(ComponentId_t id, ComponentInfo* parent_info, const std::string& type,
    const std::string& slot_name, int slot_num, uint64_t share_flags /*, const Params& params_in*/) :
    id_(id),
    parent_info(parent_info),
    name(""),
    type(type),
    link_map(nullptr),
    component(nullptr),
    params(/*new Params()*/ nullptr),
    stat_configs_(nullptr),
    enabled_stat_names_(nullptr),
    enabled_all_stats_(false),
    all_stat_config_(nullptr),
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
    id_(ccomp->id),
    parent_info(parent_info),
    name(name),
    type(ccomp->type),
    link_map(link_map),
    component(nullptr),
    params(&ccomp->params),           // Inaccessible after construction
    portModules(&ccomp->portModules), // Inaccessible after construction
    enabled_all_stats_(ccomp->enabledAllStats),
    statLoadLevel(ccomp->statLoadLevel),
    coordinates(ccomp->coords),
    subIDIndex(1),
    slot_name(ccomp->name),
    slot_num(ccomp->slot_num),
    share_flags(0)
{
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
        subComponents.emplace_hint(subComponents.end(), std::piecewise_construct, std::make_tuple(sc->id),
            std::forward_as_tuple(sc, sub_name, this, new LinkMap()));
    }

    // TODO if possible, optimize enableAllStatisticsFor... calls to share rather than
    // replicate parameters across components
    all_stat_config_ = nullptr;
    if ( enabled_all_stats_ ) {
        all_stat_config_ = new ConfigStatistic(ccomp->allStatConfig);
    }

    enabled_stat_names_ = new std::map<std::string, StatisticId_t>(ccomp->enabledStatNames);
    stat_configs_       = new std::map<StatisticId_t, ConfigStatistic>(ccomp->statistics_);
}

ComponentInfo::ComponentInfo(ComponentInfo&& o) :
    id_(o.id_),
    parent_info(o.parent_info),
    name(std::move(o.name)),
    type(std::move(o.type)),
    link_map(o.link_map),
    component(o.component),
    subComponents(std::move(o.subComponents)),
    params(o.params),
    defaultTimeBase(o.defaultTimeBase),
    portModules(o.portModules),
    stat_configs_(o.stat_configs_),
    all_stat_config_(o.all_stat_config_),
    statLoadLevel(o.statLoadLevel),
    coordinates(std::move(o.coordinates)),
    subIDIndex(o.subIDIndex),
    slot_name(std::move(o.slot_name)),
    slot_num(o.slot_num),
    share_flags(o.share_flags)
{
    o.parent_info = nullptr;
    o.link_map    = nullptr;
    o.component   = nullptr;
    o.defaultTimeBase.reset();
}

ComponentInfo::~ComponentInfo()
{
    if ( link_map ) delete link_map;
    if ( component ) {
        component->my_info_ = nullptr;
        delete component;
    }

    if ( all_stat_config_ ) delete all_stat_config_;
    delete stat_configs_;
}

void
ComponentInfo::serialize_comp(SST::Core::Serialization::serializer& ser)
{
    SST_SER(component);
    SST_SER(link_map);
    for ( auto it = subComponents.begin(); it != subComponents.end(); ++it ) {
        it->second.serialize_comp(ser);
    }
}

void
ComponentInfo::serialize_order(SST::Core::Serialization::serializer& ser)
{
    // The ComponentInfo for the Component will make sure all of the
    // hierarchy of ComponentInfos are serialized before serializing
    // any components, which includes serializing links because they
    // have handlers which have pointers to components.  This is done
    // to ensure that the ComponentInfo objects in the subComponents
    // map serialize first.  If they don't, then we may end up with a
    // corrupted subComponents map when we restart.

    // Serialize all my data except the component and link_map

    SST_SER(const_cast<ComponentId_t&>(id_));
    SST_SER(parent_info);
    SST_SER(const_cast<std::string&>(name));
    SST_SER(const_cast<std::string&>(type));

    // Not used after construction, no need to serialize
    // SST_SER(params);

    SST_SER(defaultTimeBase);

    // SST_SER(coordinates);
    SST_SER(subIDIndex);
    SST_SER(const_cast<std::string&>(slot_name));
    SST_SER(slot_num);
    SST_SER(share_flags);

    // Serialize statistic data structures - only needed for late stat registration
    // No one else has these pointers so serialize the data structure & reallocate on UNPACK
    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        std::map<StatisticId_t, ConfigStatistic> stat_configs;
        ConfigStatistic                          all_stat_config;
        std::map<std::string, StatisticId_t>     enabled_stat_names;
        bool                                     is_null = true;

        SST_SER(is_null);
        if ( !is_null ) {
            SST_SER(stat_configs);
            stat_configs_ = new std::map<StatisticId_t, ConfigStatistic>(stat_configs);
        }

        SST_SER(is_null);
        if ( !is_null ) {
            SST_SER(all_stat_config);
            all_stat_config_ = new ConfigStatistic(all_stat_config);
        }

        SST_SER(is_null);
        if ( !is_null ) {
            SST_SER(enabled_stat_names);
            enabled_stat_names_ = new std::map<std::string, StatisticId_t>(enabled_stat_names);
        }
    }
    else {
        bool is_null = stat_configs_ == nullptr;
        SST_SER(is_null);
        if ( !is_null ) SST_SER(*stat_configs_);

        is_null = all_stat_config_ == nullptr;
        SST_SER(is_null);
        if ( !is_null ) SST_SER(*all_stat_config_);

        is_null = enabled_stat_names_ == nullptr;
        SST_SER(is_null);
        if ( !is_null ) SST_SER(*enabled_stat_names_);
    }

    // Potentially needed for late stat registration
    SST_SER(statLoadLevel);
    SST_SER(enabled_all_stats_);

    // For SubComponents map, need to serialize map by hand since we
    // we will need to use the track non-pointer as pointer feature in
    // the serializer. This is becaues the SubComponent's ComponentInfo
    // object is actually stored in the map and they may have their
    // own SubCompenents that will need to point to the data location
    // in the map.

    SST_SER(subComponents, SerOption::as_ptr_elem);

    // Only the parent Component will call serialize_comp directly.
    // This function will walk the hierarchy and call it on all of its
    // subcomponent children.
    if ( parent_info == nullptr ) {
        serialize_comp(ser);
    }
}


LinkMap*
ComponentInfo::getLinkMap()
{
    if ( link_map == nullptr ) link_map = new LinkMap();
    return link_map;
}

ComponentId_t
ComponentInfo::addAnonymousSubComponent(ComponentInfo* parent_info, const std::string& type,
    const std::string& slot_name, int slot_num, uint64_t share_flags)
{
    // First, get the next subIDIndex by working our way up to the
    // actual component (parent pointer will be nullptr).
    ComponentInfo* real_comp = this;
    while ( real_comp->parent_info != nullptr )
        real_comp = real_comp->parent_info;

    // Get the subIDIndex and increment it for next time
    uint64_t sub_id = real_comp->subIDIndex++;

    ComponentId_t cid = COMPDEFINED_SUBCOMPONENT_ID_CREATE(COMPONENT_ID_MASK(id_), sub_id);

    subComponents.emplace_hint(subComponents.end(), std::piecewise_construct, std::make_tuple(cid),
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
    if ( id == this->id_ ) return this;

    /* Check to make sure we're part of the same component */
    if ( COMPONENT_ID_MASK(id) != COMPONENT_ID_MASK(this->id_) ) return nullptr;

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

////  Functions for testing serialization

ComponentInfo::ComponentInfo(
    ComponentId_t id, const std::string& name, const std::string& slot_name, TimeConverter tv) :
    id_(id),
    parent_info(nullptr),
    name(name),
    type(""),
    link_map(nullptr),
    component(nullptr),
    params(nullptr),
    defaultTimeBase(tv),
    stat_configs_(nullptr),
    enabled_stat_names_(nullptr),
    enabled_all_stats_(false),
    all_stat_config_(nullptr),
    coordinates(3, 0.0),
    subIDIndex(1),
    slot_name(slot_name),
    slot_num(-1),
    share_flags(0)
{}

ComponentInfo*
ComponentInfo::test_addSubComponentInfo(const std::string& name, const std::string& slot_name, TimeConverter tv)
{
    // Get next id, which is stored only in the ultimate parent
    ComponentInfo* real_comp = this;
    while ( real_comp->parent_info != nullptr )
        real_comp = real_comp->parent_info;

    auto ret = subComponents.emplace(std::piecewise_construct, std::forward_as_tuple(real_comp->subIDIndex),
        std::forward_as_tuple(real_comp->subIDIndex, name, slot_name, tv));
    real_comp->subIDIndex++;
    ret.first->second.parent_info = this;
    return &(ret.first->second);
}

void
ComponentInfo::test_printComponentInfoHierarchy(int indent)
{
    for ( int i = 0; i < indent; ++i )
        printf("  ");
    printf("id = %" PRIu64 ", name = %s, slot_name = %s", id_, name.c_str(), slot_name.c_str());
    if ( defaultTimeBase.isInitialized() ) printf(", defaultTimeBase = %" PRI_SIMTIME, defaultTimeBase.getFactor());
    if ( parent_info != nullptr ) printf(", parent_id = %" PRIu64, parent_info->id_);
    printf("\n");

    for ( auto& x : subComponents ) {
        x.second.test_printComponentInfoHierarchy(indent + 1);
    }
}

} // namespace SST
