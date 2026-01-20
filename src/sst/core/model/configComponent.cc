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

#include "sst/core/model/configComponent.h"

#include "sst/core/component.h"
#include "sst/core/config.h"
#include "sst/core/factory.h"
#include "sst/core/from_string.h"
#include "sst/core/model/configGraph.h"
#include "sst/core/namecheck.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeLord.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>


namespace SST {

// void
// ConfigStatistic::addParameter(const std::string& key, const std::string& value, bool overwrite)
// {
//     bool bk = params.enableVerify(false);
//     params.insert(key, value, overwrite);
//     params.enableVerify(bk);
// }

// bool
// ConfigStatGroup::addComponent(ComponentId_t id)
// {
//     if ( std::find(components.begin(), components.end(), id) == components.end() ) {
//         components.push_back(id);
//     }
//     return true;
// }

// bool
// ConfigStatGroup::addStatistic(const std::string& name, Params& p)
// {
//     statMap[name] = p;
//     if ( outputFrequency.getValue() == 0 ) {
//         /* aka, not yet really set to anything other than 0 */
//         setFrequency(p.find<std::string>("rate", "0ns"));
//     }
//     return true;
// }

// bool
// ConfigStatGroup::setOutput(size_t id)
// {
//     outputID = id;
//     return true;
// }

// bool
// ConfigStatGroup::setFrequency(const std::string& freq)
// {
//     UnitAlgebra uaFreq(freq);
//     if ( uaFreq.hasUnits("s") || uaFreq.hasUnits("hz") ) {
//         outputFrequency = uaFreq;
//         return true;
//     }
//     return false;
// }

// std::pair<bool, std::string>
// ConfigStatGroup::verifyStatsAndComponents(const ConfigGraph* graph)
// {
//     for ( auto id : components ) {
//         const ConfigComponent* comp = graph->findComponent(id);
//         if ( !comp ) {
//             std::stringstream ss;
//             ss << "Component id " << id << " is not a valid component";
//             return std::make_pair(false, ss.str());
//         }
//         for ( auto& statKV : statMap ) {

//             bool ok = Factory::getFactory()->GetStatisticValidityAndEnableLevel(comp->type, statKV.first) != 255;

//             if ( !ok ) {
//                 std::stringstream ss;
//                 ss << "Component " << comp->name << " does not support statistic " << statKV.first;
//                 return std::make_pair(false, ss.str());
//             }
//         }
//     }

//     return std::make_pair(true, "");
// }

void
ConfigPortModule::addParameter(const std::string& key, const std::string& value)
{
    params.insert(key, value);
}

void
ConfigPortModule::addSharedParamSet(const std::string& set)
{
    params.addSharedParamSet(set);
}

void
ConfigPortModule::setStatisticLoadLevel(const uint8_t level)
{
    stat_load_level = level;
}

void
ConfigPortModule::enableAllStatistics(const SST::Params& params)
{
    all_stat_config.insert(params);
}

void
ConfigPortModule::enableStatistic(const std::string& statistic_name, const SST::Params& params)
{
    auto iter = per_stat_configs.find(statistic_name);
    if ( iter == per_stat_configs.end() ) {
        per_stat_configs[statistic_name] = params;
    }
    else {
        per_stat_configs[statistic_name].insert(params);
    }
}

void
ConfigComponent::print(std::ostream& os) const
{
    os << "Component " << name << " (id = " << std::hex << id << std::dec << ")" << std::endl;
    os << "  slot_num = " << slot_num << std::endl;
    os << "  type = " << type << std::endl;
    os << "  weight = " << weight << std::endl;
    os << "  rank = " << rank.rank << std::endl;
    os << "  thread = " << rank.thread << std::endl;
    os << "  Links:" << std::endl;
    for ( unsigned int link : links ) {
        os << "    " << link;
    }
    os << std::endl;
    os << "  Params:" << std::endl;
    params.print_all_params(os, "    ");
    os << "  statLoadLevel = " << (uint32_t)statLoadLevel << std::endl;
    os << "  enabledAllStats = " << enabledAllStats << std::endl;
    os << "    Params:" << std::endl;
    allStatConfig.params.print_all_params(os, "      ");
    os << "  Statistics:" << std::endl;
    for ( auto& pair : enabledStatNames ) {
        os << "    " << pair.first << std::endl;
        os << "      Params:" << std::endl;
        auto iter = statistics_.find(pair.second);
        iter->second.params.print_all_params(os, "      ");
    }
    os << "  SubComponents:\n";
    for ( auto* sc : subComponents ) {
        sc->print(os);
    }
}

void
ConfigComponent::setConfigGraphPointer(ConfigGraph* graph_ptr)
{
    graph = graph_ptr;
    for ( auto* x : subComponents ) {
        x->setConfigGraphPointer(graph_ptr);
    }
}

ComponentId_t
ConfigComponent::getNextSubComponentID()
{
    // If we are the ultimate component, get nextSubID and increment
    // for next time
    if ( id == COMPONENT_ID_MASK(id) ) {
        uint16_t subid = nextSubID;
        nextSubID++;
        return SUBCOMPONENT_ID_CREATE(id, subid);
    }
    else {
        // Get the ultimate parent and call getNextSubComponentID on
        // it
        return graph->findComponent(COMPONENT_ID_MASK(id))->getNextSubComponentID();
    }
}

StatisticId_t
ConfigComponent::getNextStatisticID()
{
    uint16_t statId = nextStatID++;
    return STATISTIC_ID_CREATE(id, statId);
}

ConfigComponent*
ConfigComponent::getParent() const
{
    if ( id == COMPONENT_ID_MASK(id) ) {
        return nullptr;
    }
    return graph->findComponent((((ComponentId_t)nextSubID) << COMPONENT_ID_BITS) | COMPONENT_ID_MASK(id));
}

std::string
ConfigComponent::getFullName() const
{
    if ( id == COMPONENT_ID_MASK(id) ) {
        // We are a component
        return name;
    }

    // Get full name of parent
    std::string parent_name = getParent()->getFullName();

    // For ConfigComponent, we will always put in [] for the slot number.
    return parent_name + ":" + name + "[" + std::to_string(slot_num) + "]";
}

void
ConfigComponent::setRank(RankInfo r)
{
    rank = r;
    for ( auto* i : subComponents ) {
        i->setRank(r);
    }
}

void
ConfigComponent::setWeight(double w)
{
    weight = w;
    for ( auto* i : subComponents ) {
        i->setWeight(w);
    }
}

void
ConfigComponent::setCoordinates(const std::vector<double>& c)
{
    coords = c;
    /* Maintain minimum of 3D information */
    while ( coords.size() < 3 )
        coords.push_back(0.0);
}

void
ConfigComponent::addParameter(const std::string& key, const std::string& value, bool overwrite)
{
    bool bk = params.enableVerify(false);
    params.insert(key, value, overwrite);
    params.enableVerify(bk);
}

ConfigStatistic*
ConfigComponent::createStatistic()
{
    StatisticId_t stat_id = getNextStatisticID();

    auto*            parent = getParent();
    ConfigStatistic* cs     = nullptr;
    if ( parent ) {
        cs = parent->insertStatistic(stat_id);
    }
    else {
        cs = &statistics_[stat_id];
    }
    cs->id = stat_id;
    return cs;
}

ConfigStatistic*
ConfigComponent::enableStatistic(const std::string& statisticName, const SST::Params& params, bool recursively)
{
    // NOTE: For every statistic in the statistics List, there must be
    //       a corresponding params entry in enabledStatParams list.  The two
    //       lists will always be the same size.
    if ( recursively ) {
        for ( auto* sc : subComponents ) {
            sc->enableStatistic(statisticName, params, true);
        }
    }
    StatisticId_t stat_id;
    if ( statisticName == STATALLFLAG ) {
        // Special sentinel id for enable all
        // The ConfigStatistic object for STATALLFLAG is not an entry of the statistics
        // It has its own ConfigStatistic as a member variable of ConfigComponent which must be used
        // in case of enabledAllStats == true.
        enabledAllStats  = true;
        allStatConfig.id = STATALL_ID;
        allStatConfig.params.insert(params);
        return &allStatConfig;
    }
    else {
        // this is a valid statistic
        auto iter = enabledStatNames.find(statisticName);
        if ( iter == enabledStatNames.end() ) {
            // this is the first time being enabled
            stat_id                         = getNextStatisticID();
            enabledStatNames[statisticName] = stat_id;
            auto* parent                    = getParent();
            if ( parent ) {
                ConfigStatistic* cs = parent->insertStatistic(stat_id);
                cs->id              = stat_id;
                cs->params.insert(params);
                return cs;
            }
        }
        else {
            // this was already enabled
            stat_id = iter->second;
        }
    }

    ConfigStatistic& cs = statistics_[stat_id];
    cs.id               = stat_id;
    cs.params.insert(params);
    return &cs;
}

bool
ConfigComponent::reuseStatistic(const std::string& statisticName, StatisticId_t sid)
{
    if ( statisticName == STATALLFLAG ) {
        // We cannot use reuseStatistic with STATALLFLAG
        Output::getDefaultObject().fatal(CALL_INFO, 1, "Cannot reuse a Statistic with STATALLFLAG as parameter");
        return false;
    }

    auto* comp = this;
    while ( comp->getParent() != nullptr ) {
        comp = comp->getParent();
    }

    if ( !Factory::getFactory()->DoesComponentInfoStatisticNameExist(type, statisticName) ) {
        Output::getDefaultObject().fatal(CALL_INFO, 1,
            "Failed to create statistic '%s' on '%s' of type '%s' - this is not a valid statistic\n",
            statisticName.c_str(), name.c_str(), type.c_str());
        return false;
    }

    auto iter = comp->statistics_.find(sid);
    if ( iter == comp->statistics_.end() ) {
        Output::getDefaultObject().fatal(CALL_INFO, 1, "Cannot reuse a statistic that doesn't exist for the parent");
        return false;
    }
    else {
        enabledStatNames[statisticName] = sid;
        return true;
    }
}

void
ConfigComponent::addStatisticParameter(
    const std::string& statisticName, const std::string& param, const std::string& value, bool recursively)
{
    // NOTE: For every statistic in the statistics map, there must be
    //       a corresponding params entry in enabledStatParams list.  The two
    //       lists will always be the same size.
    if ( recursively ) {
        for ( auto* sc : subComponents ) {
            sc->addStatisticParameter(statisticName, param, value, true);
        }
    }

    ConfigStatistic* cs = nullptr;
    if ( statisticName == STATALLFLAG ) {
        cs = &allStatConfig;
    }
    else {
        cs = findStatistic(statisticName);
    }
    if ( !cs ) {
        Output::getDefaultObject().fatal(
            CALL_INFO, 1, "cannot add parameter '%s' to unknown statistic '%s'", param.c_str(), statisticName.c_str());
    }
    cs->params.insert(param, value);
}

void
ConfigComponent::setStatisticParameters(const std::string& statisticName, const Params& params, bool recursively)
{
    if ( recursively ) {
        for ( auto* sc : subComponents ) {
            sc->setStatisticParameters(statisticName, params, true);
        }
    }

    if ( statisticName == STATALLFLAG ) {
        allStatConfig.params.insert(params);
        ;
    }
    else {
        findStatistic(statisticName)->params.insert(params);
    }
}

void
ConfigComponent::setStatisticLoadLevel(uint8_t level, bool recursively)
{
    statLoadLevel = level;

    if ( recursively ) {
        for ( auto* sc : subComponents ) {
            sc->setStatisticLoadLevel(level, true);
        }
    }
}

ConfigComponent*
ConfigComponent::addSubComponent(const std::string& name, const std::string& type, int slot_num)
{
    ComponentId_t sid = getNextSubComponentID();
    /* Check for existing subComponent with this name */
    for ( auto* i : subComponents ) {
        if ( i->name == name && i->slot_num == slot_num ) return nullptr;
    }

    uint16_t parent_sub_id = SUBCOMPONENT_ID_MASK(id);

    subComponents.push_back(
        new ConfigComponent(sid, graph, parent_sub_id, name, slot_num, type, this->weight, this->rank));

    return subComponents.back();
}

ConfigComponent*
ConfigComponent::findSubComponent(ComponentId_t sid)
{
    return const_cast<ConfigComponent*>(const_cast<const ConfigComponent*>(this)->findSubComponent(sid));
}

const ConfigComponent*
ConfigComponent::findSubComponent(ComponentId_t sid) const
{
    if ( sid == this->id ) return this;

    for ( auto* s : subComponents ) {
        const ConfigComponent* res = s->findSubComponent(sid);
        if ( res != nullptr ) return res;
    }

    return nullptr;
}

ConfigComponent*
ConfigComponent::findSubComponentByName(const std::string& name)
{
    size_t      colon_index = name.find(":");
    std::string slot        = name.substr(0, colon_index);

    // Get the slot number
    int    slot_num      = 0;
    size_t bracket_index = slot.find("[");
    if ( bracket_index == std::string::npos ) {
        // No brackets, slot_num 0
        slot_num = 0;
    }
    else {
        size_t close_index = slot.find("]");
        size_t length      = close_index - bracket_index - 1;
        slot_num           = Core::from_string<int>(slot.substr(bracket_index + 1, length));
        slot               = slot.substr(0, bracket_index);
    }

    // Now, see if we have something in this slot and slot_num
    for ( auto* sc : subComponents ) {
        if ( sc->name == slot && sc->slot_num == slot_num ) {
            // Found the subcomponent
            if ( colon_index == std::string::npos ) {
                // Last level of hierarchy
                return sc;
            }
            else {
                return sc->findSubComponentByName(name.substr(colon_index + 1, std::string::npos));
            }
        }
    }
    return nullptr;
}

ConfigStatistic*
ConfigComponent::insertStatistic(StatisticId_t sid)
{
    ConfigComponent* parent = getParent();
    if ( parent ) {
        return parent->insertStatistic(sid);
    }
    else {
        return &statistics_[sid];
    }
}

ConfigStatistic*
ConfigComponent::findStatistic(const std::string& name) const
{
    auto iter = enabledStatNames.find(name);
    if ( iter != enabledStatNames.end() ) {
        StatisticId_t id = iter->second;
        return findStatistic(id);
    }
    else {
        return nullptr;
    }
}

ConfigStatistic*
ConfigComponent::findStatistic(StatisticId_t sid) const
{
    auto* parent = getParent();
    if ( parent ) {
        return parent->findStatistic(sid);
    }
    else {
        auto iter = statistics_.find(sid);
        if ( iter == statistics_.end() ) {
            return nullptr;
        }
        else {
            // I hate that I have to do this
            return const_cast<ConfigStatistic*>(&iter->second);
        }
    }
}

size_t
ConfigComponent::addPortModule(const std::string& port, const std::string& type, const Params& params)
{
    port_modules[port].emplace_back(type, params);
    return port_modules.size() - 1;
}

std::vector<LinkId_t>
ConfigComponent::allLinks() const
{
    std::vector<LinkId_t> res;
    res.insert(res.end(), links.begin(), links.end());
    for ( auto* sc : subComponents ) {
        std::vector<LinkId_t> s = sc->allLinks();
        res.insert(res.end(), s.begin(), s.end());
    }
    return res;
}

std::vector<LinkId_t>
ConfigComponent::clearAllLinks()
{
    std::vector<LinkId_t> res;
    res.insert(res.end(), links.begin(), links.end());
    links.clear();
    for ( auto* sc : subComponents ) {
        std::vector<LinkId_t> s = sc->clearAllLinks();
        res.insert(res.end(), s.begin(), s.end());
    }
    return res;
}

void
ConfigComponent::checkPorts() const
{
    std::map<std::string, std::string> ports;

    auto& graph_links = graph->getLinkMap();

    // Loop over all the links
    for ( unsigned int i : links ) {
        const ConfigLink* link = graph_links[i];
        for ( int j = 0; j < 2; j++ ) {
            // If this is a nonlocal link, then there is no port to check for index 1
            if ( link->nonlocal && j == 1 ) continue;
            if ( link->component[j] == id ) {
                // If port is not found, print an error
                if ( !Factory::getFactory()->isPortNameValid(type, link->port[j]) ) {
                    // For now this is not a fatal error
                    // found_error = true;
                    Output::getDefaultObject().fatal(CALL_INFO, 1,
                        "ERROR:  Attempting to connect to unknown port: %s, "
                        "in component %s of type %s.\n",
                        link->port[j].c_str(), name.c_str(), type.c_str());
                }

                // Check for multiple links hooked to port
                auto ret = ports.insert(std::make_pair(link->port[j], link->name));
                if ( !ret.second ) {
                    // Check to see if this is a loopback link
                    if ( ret.first->second != link->name )
                        // Not a loopback link, fatal...
                        Output::getDefaultObject().fatal(CALL_INFO, 1,
                            "ERROR: Port %s of Component %s connected to two links: %s, %s.\n", link->port[j].c_str(),
                            name.c_str(), link->name.c_str(), ret.first->second.c_str());
                }
            }
        }
    }

    // Now loop over all subcomponents and call the check function
    for ( auto* subcomp : subComponents ) {
        subcomp->checkPorts();
    }
}

} // namespace SST
