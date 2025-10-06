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

#include "sst/core/configGraph.h"

#include "sst/core/component.h"
#include "sst/core/config.h"
#include "sst/core/factory.h"
#include "sst/core/from_string.h"
#include "sst/core/namecheck.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeLord.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>

namespace {
// bool zero_latency_warning = false;

// Functions to check component and link names
int bad_comp_name_count = 0;
int bad_link_name_count = 0;

const int max_invalid_name_prints = 10;

void
checkForValidComponentName(const std::string& name)
{
    if ( SST::NameCheck::isComponentNameValid(name) ) return;
    if ( bad_comp_name_count < max_invalid_name_prints ) {
        printf("WARNING: Component name '%s' is not valid\n", name.c_str());
        bad_comp_name_count++;
    }
    else if ( bad_comp_name_count == max_invalid_name_prints ) {
        printf("WARNING: Number of invalid component names exceeds limit of %d, no more messages will be printed\n",
            max_invalid_name_prints);
        bad_comp_name_count++;
    }
}

void
checkForValidLinkName(const std::string& name)
{
    if ( SST::NameCheck::isLinkNameValid(name) ) return;
    if ( bad_link_name_count < max_invalid_name_prints ) {
        printf("WARNING: Link name '%s' is not valid\n", name.c_str());
        bad_link_name_count++;
    }
    else if ( bad_link_name_count == max_invalid_name_prints ) {
        printf("WARNING: Number of invalid link names exceeds limit of %d, no more messages will be printed\n",
            max_invalid_name_prints);
        bad_link_name_count++;
    }
}
} // anonymous namespace


namespace SST {
std::map<std::string, uint32_t> ConfigLink::lat_to_index;

uint32_t
ConfigLink::getIndexForLatency(const char* latency)
{
    std::string lat(latency);
    uint32_t&   index = lat_to_index[lat];
    if ( index == 0 ) {
        // Wasn't there, set it to lat_to_index.size(), which is the
        // next index (skipping zero as that is the check for a new
        // entry)
        index = lat_to_index.size();
    }
    return index;
}

std::vector<SimTime_t>
ConfigLink::initializeLinkLatencyVector()
{
    TimeLord*              timeLord = Simulation_impl::getTimeLord();
    std::vector<SimTime_t> vec;
    vec.resize(lat_to_index.size() + 1);
    for ( auto& [lat, index] : lat_to_index ) {
        vec[index] = timeLord->getSimCycles(lat, __FUNCTION__);
    }
    return vec;
}

SimTime_t
ConfigLink::getLatencyFromIndex(uint32_t index)
{
    static std::vector<SimTime_t> vec = initializeLinkLatencyVector();
    return vec[index];
}

std::string
ConfigLink::latency_str(uint32_t index) const
{
    static TimeLord* timelord = Simulation_impl::getTimeLord();
    UnitAlgebra      tb       = timelord->getTimeBase();
    auto             tmp      = tb * latency[index];
    return tmp.toStringBestSI();
}

void
ConfigLink::setAsNonLocal(int which_local, RankInfo remote_rank_info)
{
    // First, if which_local != 0, we need to swap the data for index
    // 0 and 1
    if ( which_local == 1 ) {
        std::swap(component[0], component[1]);
        std::swap(port[0], port[1]);
        std::swap(latency[0], latency[1]);
    }

    // Now add remote annotations.  Rank goes in component[1], thread
    // goes in latency[1], port[1] is not needed
    component[1] = remote_rank_info.rank;
    latency[1]   = remote_rank_info.thread;
    port[1].clear();

    nonlocal = true;
}

void
ConfigLink::updateLatencies()
{
    // Need to clean up some elements before we can test for zero latency
    if ( order >= 1 ) {
        latency[0] = ConfigLink::getLatencyFromIndex(latency[0]);
    }
    if ( order >= 2 ) {
        latency[1] = ConfigLink::getLatencyFromIndex(latency[1]);
    }
}

void
ConfigStatistic::addParameter(const std::string& key, const std::string& value, bool overwrite)
{
    bool bk = params.enableVerify(false);
    params.insert(key, value, overwrite);
    params.enableVerify(bk);
}

bool
ConfigStatGroup::addComponent(ComponentId_t id)
{
    if ( std::find(components.begin(), components.end(), id) == components.end() ) {
        components.push_back(id);
    }
    return true;
}

bool
ConfigStatGroup::addStatistic(const std::string& name, Params& p)
{
    statMap[name] = p;
    if ( outputFrequency.getValue() == 0 ) {
        /* aka, not yet really set to anything other than 0 */
        setFrequency(p.find<std::string>("rate", "0ns"));
    }
    return true;
}

bool
ConfigStatGroup::setOutput(size_t id)
{
    outputID = id;
    return true;
}

bool
ConfigStatGroup::setFrequency(const std::string& freq)
{
    UnitAlgebra uaFreq(freq);
    if ( uaFreq.hasUnits("s") || uaFreq.hasUnits("hz") ) {
        outputFrequency = uaFreq;
        return true;
    }
    return false;
}

std::pair<bool, std::string>
ConfigStatGroup::verifyStatsAndComponents(const ConfigGraph* graph)
{
    for ( auto id : components ) {
        const ConfigComponent* comp = graph->findComponent(id);
        if ( !comp ) {
            std::stringstream ss;
            ss << "Component id " << id << " is not a valid component";
            return std::make_pair(false, ss.str());
        }
        for ( auto& statKV : statMap ) {

            bool ok = Factory::getFactory()->GetStatisticValidityAndEnableLevel(comp->type, statKV.first) != 255;

            if ( !ok ) {
                std::stringstream ss;
                ss << "Component " << comp->name << " does not support statistic " << statKV.first;
                return std::make_pair(false, ss.str());
            }
        }
    }

    return std::make_pair(true, "");
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
    for ( size_t i = 0; i != links.size(); ++i ) {
        os << "    " << links[i];
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

ConfigComponent*
ConfigComponent::cloneWithoutLinks(ConfigGraph* new_graph) const
{
    ConfigComponent* ret  = new ConfigComponent();
    ret->id               = id;
    ret->name             = name;
    ret->slot_num         = slot_num;
    ret->type             = type;
    ret->weight           = weight;
    ret->rank             = rank;
    ret->params           = params;
    ret->statLoadLevel    = statLoadLevel;
    ret->statistics_      = statistics_;
    ret->enabledStatNames = enabledStatNames;
    ret->enabledAllStats  = enabledAllStats;
    ret->allStatConfig    = allStatConfig;
    ret->coords           = coords;
    ret->nextSubID        = nextSubID;
    ret->graph            = new_graph;
    for ( auto* i : subComponents ) {
        ret->subComponents.emplace_back(i->cloneWithoutLinks(new_graph));
    }
    return ret;
}

ConfigComponent*
ConfigComponent::cloneWithoutLinksOrParams(ConfigGraph* new_graph) const
{
    ConfigComponent* ret = new ConfigComponent();
    ret->id              = id;
    ret->name            = name;
    ret->slot_num        = slot_num;
    ret->type            = type;
    ret->weight          = weight;
    ret->rank            = rank;
    ret->statLoadLevel   = statLoadLevel;
    ret->coords          = coords;
    ret->nextSubID       = nextSubID;
    ret->graph           = new_graph;
    for ( auto* i : subComponents ) {
        ret->subComponents.emplace_back(i->cloneWithoutLinksOrParams(new_graph));
    }
    return ret;
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

void
ConfigComponent::addPortModule(const std::string& port, const std::string& type, const Params& params)
{
    portModules[port].emplace_back(type, params);
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
    for ( unsigned int i = 0; i < links.size(); i++ ) {
        const ConfigLink* link = graph_links[links[i]];
        for ( int j = 0; j < 2; j++ ) {

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

size_t
ConfigGraph::getNumComponentsInMPIRank(uint32_t rank)
{
    size_t count = 0;
    for ( auto* comp : comps_ ) {
        if ( comp->rank.rank == rank ) ++count;
    }
    return count;
}


void
ConfigGraph::setComponentRanks(RankInfo rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps_.begin(); iter != comps_.end(); ++iter ) {
        (*iter)->setRank(rank);
    }
}

bool
ConfigGraph::containsComponentInRank(RankInfo rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps_.begin(); iter != comps_.end(); ++iter ) {
        if ( (*iter)->rank == rank ) return true;
    }
    return false;
}

bool
ConfigGraph::checkRanks(RankInfo ranks)
{
    for ( auto& comp : comps_ ) {
        if ( !comp->rank.isAssigned() || !ranks.inRange(comp->rank) ) {
            fprintf(stderr, "Bad rank: %u %u\n", comp->rank.rank, comp->rank.thread);
            return false;
        }
    }

    // Set the cross_rank and cross_thread flags
    for ( auto& link : links_ ) {
        RankInfo r0(-1, -1);
        RankInfo r1(-1, -1);
        r0 = comps_[COMPONENT_ID_MASK(link->component[0])]->rank;

        if ( link->nonlocal ) {
            r1.rank   = link->component[1];
            r1.thread = link->latency[1];
        }
        else {
            r1 = comps_[COMPONENT_ID_MASK(link->component[1])]->rank;
        }

        if ( r0.rank != r1.rank ) {
            link->cross_rank = true;
        }
        else if ( r0.thread != r1.thread ) {
            link->cross_thread = true;
        }
    }

    return true;
}

void
ConfigGraph::postCreationCleanup()
{
    for ( ConfigLink* link : getLinkMap() ) {
        link->updateLatencies();
    }

    // Need to assign the link delivery order.  This is done
    // alphabetically by link name. To save memory, we'll sort links_
    // by name, then sort it back by link_id
    std::sort(links_.begin(), links_.end(),
        [](const ConfigLink* lhs, const ConfigLink* rhs) -> bool { return lhs->name < rhs->name; });

    LinkId_t count = 1;
    for ( auto* link : links_ ) {
        link->order = count;
        count++;
    }

    links_.sort();

    /* Force component / statistic registration for Group stats */
    for ( auto& cfg : getStatGroups() ) {
        for ( ComponentId_t compID : cfg.second.components ) {
            ConfigComponent* ccomp = findComponent(compID);
            if ( ccomp ) { /* Should always be true */
                for ( auto& kv : cfg.second.statMap ) {
                    ccomp->enableStatistic(kv.first, kv.second);
                }
            }
        }
    }
}

// Checks for errors that can't be easily detected during the build
// process
bool
ConfigGraph::checkForStructuralErrors()
{
    // Check to make sure there are no dangling links.  A dangling
    // link is found by looking though the links in the graph and
    // making sure there are components on both sides of the link.

    bool found_error = false;
    for ( ConfigLinkMap_t::iterator iter = links_.begin(); iter != links_.end(); ++iter ) {
        ConfigLink* clink = *iter;

        // First check to see if the link is completely unused
        if ( clink->order == 0 ) {
            output.output("WARNING:  Found unused link: %s\n", clink->name.c_str());
            found_error = true;
        }

        // If component[0] is not initialized, this is an unused link
        if ( clink->component[0] == ULONG_MAX ) {
            output.output("WARNING:  Found unused link: %s\n", clink->name.c_str());
            found_error = true;
        }
        // If component[1] is not initialized, this is a dangling link
        else if ( clink->component[1] == ULONG_MAX ) {
            output.output("WARNING:  Found dangling link: %s.  It is connected on one side to component %s.\n",
                clink->name.c_str(), comps_[clink->component[0]]->name.c_str());
            found_error = true;
        }
    }

    // Check to see if all the port names are valid and they are only
    // used once

    // Loop over all the Components
    for ( ConfigComponentMap_t::iterator iter = comps_.begin(); iter != comps_.end(); ++iter ) {
        ConfigComponent* ccomp = *iter;
        ccomp->checkPorts();
    }

    return found_error;
}

ComponentId_t
ConfigGraph::addComponent(const std::string& name, const std::string& type)
{
    checkForValidComponentName(name);
    ComponentId_t cid = nextComponentId++;
    comps_.insert(new ConfigComponent(cid, this, name, type, 1.0f, RankInfo()));

    auto ret = comps_by_name_.insert(std::make_pair(name, cid));
    // Check to see if the name has already been used
    if ( !ret.second ) {
        output.fatal(CALL_INFO, 1, "ERROR: trying to add Component with name that already exists: %s\n", name.c_str());
    }
    return cid;
}

void
ConfigGraph::addSharedParam(const std::string& shared_set, const std::string& key, const std::string& value)
{
    Params::insert_shared(shared_set, key, value);
}

void
ConfigGraph::setStatisticOutput(const std::string& name)
{
    stats_config_->outputs[0].type = name;
}

void
ConfigGraph::setStatisticOutputParams(const Params& p)
{
    stats_config_->outputs[0].params = p;
}

void
ConfigGraph::addStatisticOutputParameter(const std::string& param, const std::string& value)
{
    stats_config_->outputs[0].params.insert(param, value);
}

void
ConfigGraph::setStatisticLoadLevel(uint8_t loadLevel)
{
    stats_config_->load_level = loadLevel;
}

void
ConfigGraph::addLink(ComponentId_t comp_id, LinkId_t link_id, const char* port, const char* latency_str)
{
    // checkForValidLinkName(link_name);

    // The Link was created earlier, just get it out of the links_
    // data structure.
    ConfigLink* link = links_[link_id];

    // Check to make sure the link has not been referenced too many
    // times.
    if ( link->order >= 2 ) {
        output.fatal(
            CALL_INFO, 1, "ERROR: Parsing SDL file: Link %s referenced more than two times\n", link->name.c_str());
    }
    else if ( link->order == 1 && link->nonlocal ) {
        output.fatal(CALL_INFO, 1,
            "ERROR: Parsing SDL file: Attempting to connect second component to link %s which is set as non-local\n",
            link->name.c_str());
    }

    // Check to make sure that a latency was specified, either in the
    // call or at ConfigLink construct time
    if ( nullptr == latency_str && link->latency[0] == 0 ) {
        output.fatal(CALL_INFO, 1, "ERROR: Parsing SDL file: Connecting link with no latency assigned: %s\n",
            link->name.c_str());
    }

    // Update link information
    int index              = link->order++;
    link->component[index] = comp_id;
    link->port[index]      = port;

    // A nullptr for latency_str means use the latency specified at
    // link creation
    if ( latency_str ) link->latency[index] = ConfigLink::getIndexForLatency(latency_str);

    // Need to add this link to the ConfigComponent's link list.
    // Check to make sure the link doesn't already exist in the
    // component.  Only possible way it could be there is if the link
    // is attached to the component at both ends.  So, if this is the
    // first reference to the link, or if link->component[0] is not
    // equal to the current component sent into this call, then it is
    // not already in the list.
    if ( link->order == 1 || link->component[0] != comp_id ) {
        auto compLinks = &findComponent(comp_id)->links;
        compLinks->push_back(link->id);
    }
}

void
ConfigGraph::addNonLocalLink(LinkId_t link_id, int rank, int thread)
{
    ConfigLink* link = links_[link_id];
    if ( link->nonlocal ) {
        output.fatal(CALL_INFO, 1,
            "ERROR: Parsing SDL file: Trying to set link %s as as non-local, which is already set to non-local\n",
            link->name.c_str());
    }
    else if ( link->order == 2 ) {
        output.fatal(CALL_INFO, 1,
            "ERROR: Parsing SDL file: Link %s being set as non-local, but is already connected to two components\n",
            link->name.c_str());
    }
    link->nonlocal     = true;
    link->component[1] = rank;
    link->latency[1]   = thread;
}


LinkId_t
ConfigGraph::createLink(const char* name, const char* latency)
{
    checkForValidLinkName(name);
    LinkId_t    id   = (LinkId_t)links_.size();
    ConfigLink* link = new ConfigLink(id, name);
    links_.insert(link);
    if ( latency ) {
        uint32_t index   = ConfigLink::getIndexForLatency(latency);
        link->latency[0] = index;
        link->latency[1] = index;
    }
    return id;
}

void
ConfigGraph::setLinkNoCut(LinkId_t link_id)
{
    links_[link_id]->no_cut = true;
}

bool
ConfigGraph::containsComponent(ComponentId_t id) const
{
    return comps_.contains(id);
}

ConfigComponent*
ConfigGraph::findComponent(ComponentId_t id)
{
    return const_cast<ConfigComponent*>(const_cast<const ConfigGraph*>(this)->findComponent(id));
}

const ConfigComponent*
ConfigGraph::findComponent(ComponentId_t id) const
{
    /* Check to make sure we're part of the same component */
    if ( COMPONENT_ID_MASK(id) == id ) {
        return comps_[id];
    }

    return comps_[COMPONENT_ID_MASK(id)]->findSubComponent(id);
}

ConfigComponent*
ConfigGraph::findComponentByName(const std::string& name)
{
    std::string origname(name);
    auto        index    = origname.find(":");
    std::string compname = origname.substr(0, index);
    auto        itr      = comps_by_name_.find(compname);

    // Check to see if component was found
    if ( itr == comps_by_name_.end() ) return nullptr;

    ConfigComponent* cc = comps_[itr->second];

    // If this was just a component name
    if ( index == std::string::npos ) return cc;

    // See if this is a valid subcomponent name
    cc = cc->findSubComponentByName(origname.substr(index + 1, std::string::npos));
    if ( cc ) return cc;
    return nullptr;
}

ConfigStatistic*
ConfigGraph::findStatistic(StatisticId_t id) const
{
    ComponentId_t cfg_id = CONFIG_COMPONENT_ID_MASK(id);
    return findComponent(cfg_id)->findStatistic(id);
}


ConfigGraph::GraphFilter::GraphFilter(ConfigGraph* original_graph, ConfigGraph* new_graph,
    const std::set<uint32_t>& original_rank_set, const std::set<uint32_t>& new_rank_set) :
    ograph_(original_graph),
    ngraph_(new_graph),
    oset_(original_rank_set),
    nset_(new_rank_set)
{}

ConfigLink*
ConfigGraph::GraphFilter::operator()(ConfigLink* link)
{
    // Need to see if the link is connected to components in the
    // old and/or new graph
    RankInfo ranks[2];
    ranks[0] = ograph_->findComponent(link->component[0])->rank;
    if ( link->nonlocal ) {
        ranks[1].rank = -1;
    }
    else {
        ranks[1] = ograph_->findComponent(link->component[1])->rank;
    }

    // Check to see which components are in which sets
    bool c0_in_orig = oset_.count(ranks[0].rank);
    bool c1_in_orig = oset_.count(ranks[1].rank);
    bool c0_in_new  = nset_.count(ranks[0].rank);
    bool c1_in_new  = nset_.count(ranks[1].rank);

    // First bit of flag checks to see if either end is in the set
    // that will stay in the original graph
    uint8_t flag = c0_in_orig | c1_in_orig;

    // Second bit will check for the set that will be in the new graph
    flag |= ((c0_in_new | c1_in_new) << 1);

    switch ( flag ) {
    case 0:
        // Not connected in either of the partitions.  This shouldn't
        // happen, but if it did, it's just an extraneous link.  Just
        // delete it and remove from the original graph.
        delete link;
        return nullptr;
    case 1:
        // Connected in original graph, not in new. Just return it

        // See if link needs to be set as nonlocal
        if ( !link->nonlocal && (c0_in_orig ^ c1_in_orig) ) {
            // Only one side is in the set. Figure out which one and set the link as nonlocal
            int index = c0_in_orig ? 0 : 1;
            link->setAsNonLocal(index, ranks[(index + 1) % 2]);
        }
        return link;
    case 2:
        // Connected in new graph, but not original.  Move to new
        // graph
        ngraph_->links_.insert(link);

        // See if link needs to be set as nonlocal
        if ( !link->nonlocal && (c0_in_new ^ c1_in_new) ) {
            // Only one side is in the set. Figure out which one and set the link as nonlocal
            int index = c0_in_new ? 0 : 1;
            link->setAsNonLocal(index, ranks[(index + 1) % 1]);
        }
        return nullptr;
    case 3:
        // Connected in both graphs.  Make a copy for the new graph and mark both links as cross partition.  NOTE: we
        // won't get to this state unless the graph originally used a ghost component for the cross partition
        // link. Links marked as cross platform can't get here because ranks[1] is set to -1.
        ConfigLink* link_new = new ConfigLink(*link);
        ngraph_->links_.insert(link_new);

        // Now change the two links to cross partition
        if ( c0_in_new ) {
            // component[0] in new set
            link->setAsNonLocal(1, ranks[0]);
            link_new->setAsNonLocal(0, ranks[1]);
        }
        else {
            // component[1] in new set
            link->setAsNonLocal(0, ranks[1]);
            link_new->setAsNonLocal(1, ranks[0]);
        }

        return link;
    }
    // Silence warning even though every possible path has a return
    return nullptr;
}

// Must run the link filter first!
ConfigComponent*
ConfigGraph::GraphFilter::operator()(ConfigComponent* comp)
{
    // Figure out which graph it belongs in and put it there.  All the
    // cross partition info is now held in ConfigLink

    if ( oset_.count(comp->rank.rank) ) {
        // Stays in the current graph
        return comp;
    }
    else if ( nset_.count(comp->rank.rank) ) {
        // Move to new graph
        comp->graph = ngraph_;
        ngraph_->comps_.insert(comp);
        return nullptr;
    }
    else {
        // Not in either group.  Need to delete comp and return
        // nullptr.  This should only happen if the user used ghost
        // components to specify remote rank
        delete comp;
        return nullptr;
    }
}


ConfigGraph*
ConfigGraph::splitGraph(const std::set<uint32_t>& orig_rank_set, const std::set<uint32_t>& new_rank_set)
{
    ConfigGraph* graph = nullptr;

    if ( !new_rank_set.empty() ) {
        graph = new ConfigGraph();

        // Need to copy over any restart data
        graph->cpt_ranks           = cpt_ranks;
        graph->cpt_currentSimCycle = cpt_currentSimCycle;
        graph->cpt_currentPriority = cpt_currentPriority;
        graph->cpt_minPart         = cpt_minPart;
        graph->cpt_minPartTC       = cpt_minPartTC;
        graph->cpt_max_event_id    = cpt_max_event_id;

        graph->cpt_libnames       = cpt_libnames;
        graph->cpt_shared_objects = cpt_shared_objects;
        graph->cpt_stats_config   = cpt_stats_config;
    }

    // Split up the links
    GraphFilter filter(this, graph, orig_rank_set, new_rank_set);
    links_.filter(filter);

    // Split up the components
    comps_.filter(filter);

    // Copy the statistic configuration to the sub-graph
    if ( graph ) graph->stats_config_->outputs = this->stats_config_->outputs;

    // Need to copy statgroups contained in new graph and remove statgroups that are no longer needed in original graph.
    // However, for now, we need to leave all stat group information on rank 0 so that all statgroups get checkpointed
    // (only rank 0 adds statgroups to the checkpoint)

    // for ( auto& kv : this->statGroups ) {

    bool orig_includes_zero = orig_rank_set.count(0);
    bool new_includes_zero  = new_rank_set.count(0);
    for ( auto it = this->stats_config_->groups.begin(); it != this->stats_config_->groups.end();
        /* increment in loop body */ ) {
        bool copy   = false;
        bool remove = true;
        for ( auto& id : it->second.components ) {
            if ( (graph && graph->containsComponent(id)) || new_includes_zero ) {
                copy = true;
                // We can stop if we've verified it belongs in both
                // already.  In this case, if remove is false, then we
                // can break
                if ( !remove ) break;
            }
            if ( containsComponent(id) || orig_includes_zero ) {
                remove = false;
                // We can stop if we've verified it belongs in both
                // already.  In this case, if copy is true, then we
                // can break
                if ( (nullptr == graph) || copy ) break;
            }
        }

        // See if we need to copy into new graph
        if ( copy ) { // If graph is nullptr, then copy can't be true
            graph->stats_config_->groups.insert(std::make_pair(it->first, it->second));
        }

        // See if we need to remove from the original graph.
        if ( remove ) {
            it = this->stats_config_->groups.erase(it);
        }
        else {
            ++it;
        }
    }

    if ( graph ) graph->setStatisticLoadLevel(this->getStatLoadLevel());


    return graph;
}

void
ConfigGraph::reduceGraphToSingleRank(uint32_t rank)
{
    std::set<uint32_t> ranks = { rank };
    splitGraph(ranks, std::set<uint32_t>());
}

SimTime_t
ConfigGraph::getMinimumPartitionLatency()
{
    if ( getNumComponents() == 0 ) {
        // This is a restart run with no repartitioning, so just return the minPart from the checkpoint
        return cpt_minPart;
    }

    SimTime_t graph_min_part = std::numeric_limits<SimTime_t>::max();

    for ( auto& link : links_ ) {
        if ( link->cross_rank ) {
            SimTime_t min_lat = link->getMinLatency();
            if ( min_lat < graph_min_part ) {
                graph_min_part = min_lat;
            }
        }
    }
    return graph_min_part;
}


PartitionGraph*
ConfigGraph::getPartitionGraph()
{
    PartitionGraph* graph = new PartitionGraph();

    PartitionComponentMap_t& pcomps = graph->getComponentMap();
    PartitionLinkMap_t&      plinks = graph->getLinkMap();

    // SparseVectorMap is slow for random inserts, so make sure we
    // insert both components and links in order of ID, which is the
    // key for the SparseVectorMap
    for ( ConfigComponentMap_t::iterator it = comps_.begin(); it != comps_.end(); ++it ) {
        const ConfigComponent* comp = *it;

        pcomps.insert(new PartitionComponent(comp));
    }

    for ( ConfigLinkMap_t::iterator it = links_.begin(); it != links_.end(); ++it ) {
        const ConfigLink* link = *it;

        const ConfigComponent* comp0 = comps_[COMPONENT_ID_MASK(link->component[0])];
        const ConfigComponent* comp1 = comps_[COMPONENT_ID_MASK(link->component[1])];

        plinks.insert(PartitionLink(*link));

        pcomps[comp0->id]->links.push_back(link->id);
        pcomps[comp1->id]->links.push_back(link->id);
    }
    return graph;
}

PartitionGraph*
ConfigGraph::getCollapsedPartitionGraph()
{
    PartitionGraph* graph = new PartitionGraph();

    std::set<LinkId_t> deleted_links;

    PartitionComponentMap_t& pcomps = graph->getComponentMap();
    PartitionLinkMap_t&      plinks = graph->getLinkMap();

    // Mark all Components as not visited
    for ( ConfigComponentMap_t::iterator it = comps_.begin(); it != comps_.end(); ++it )
        (*it)->visited = false;

    // SparseVectorMap is slow for random inserts, so make sure we
    // insert both components and links in order of ID, which is the
    // key for the SparseVectorMap in both cases

    // Use an ordered set so that when we insert the ids for the group
    // into a SparseVectorMap, we are inserting in order.
    std::set<ComponentId_t> group;
    for ( ConfigComponentMap_t::iterator it = comps_.begin(); it != comps_.end(); ++it ) {
        auto comp = *it;
        // If this component ended up in a connected group we already
        // looked at, skip it
        if ( comp->visited ) continue;

        // Get the no-cut group for this component
        group.clear();
        getConnectedNoCutComps(comp->id, group);

        // Create a new PartitionComponent for this group
        ComponentId_t       id    = pcomps.size();
        PartitionComponent* pcomp = pcomps.insert(new PartitionComponent(id));

        // Iterate over the group and add the weights and add any
        // links that connect outside the group
        for ( std::set<ComponentId_t>::const_iterator i = group.begin(); i != group.end(); ++i ) {
            const ConfigComponent* comp = comps_[*i];
            // Compute the new weight
            pcomp->weight += comp->weight;
            // Inserting in order because the iterator is from an
            // ordered set
            pcomp->group.insert(*i);

            // Walk through all the links and insert the ones that connect
            // outside the group
            for ( LinkId_t id : comp->allLinks() ) {
                const ConfigLink* link = links_[id];

                if ( group.find(COMPONENT_ID_MASK(link->component[0])) == group.end() ||
                     group.find(COMPONENT_ID_MASK(link->component[1])) == group.end() ) {
                    pcomp->links.push_back(link->id);
                }
                else {
                    deleted_links.insert(link->id);
                }
            }
        }
    }

    // Now add all but the deleted links to the partition graph.  We
    // do it here so that we insert in order because we are using a
    // SparseVectorMap.  It may look like we are inserting the actual
    // ConfigLink into the map, but this actually adds a PartitionLink
    // to the set by passing the ConfigLink into the constructor.
    // This will insert in order since the iterator is from a
    // SparseVectorMap.
    for ( ConfigLinkMap_t::iterator i = links_.begin(); i != links_.end(); ++i ) {
        if ( deleted_links.find((*i)->id) == deleted_links.end() ) plinks.insert(*(*i));
    }

    // Just need to fix up the component fields for the links.  Do
    // this by walking through the components and checking each of it's
    // links to see if it points to something in the group.  If so,
    // change ID to point to super group.
    for ( PartitionComponentMap_t::iterator i = pcomps.begin(); i != pcomps.end(); ++i ) {
        PartitionComponent* pcomp = *i;
        for ( LinkIdMap_t::iterator j = pcomp->links.begin(); j != pcomp->links.end(); ++j ) {
            PartitionLink& plink = plinks[*j];
            if ( pcomp->group.contains(plink.component[0]) ) plink.component[0] = pcomp->id;
            if ( pcomp->group.contains(plink.component[1]) ) plink.component[1] = pcomp->id;
        }
    }

    return graph;
}

void
ConfigGraph::annotateRanks(PartitionGraph* graph)
{
    PartitionComponentMap_t& pcomps = graph->getComponentMap();

    for ( PartitionComponentMap_t::iterator it = pcomps.begin(); it != pcomps.end(); ++it ) {
        const PartitionComponent* comp = *it;

        for ( ComponentIdMap_t::const_iterator c_iter = comp->group.begin(); c_iter != comp->group.end(); ++c_iter ) {
            comps_[*c_iter]->setRank(comp->rank);
        }
    }
}

void
ConfigGraph::getConnectedNoCutComps(ComponentId_t start, std::set<ComponentId_t>& group)
{
    // We'll do this as a simple recursive depth first search
    group.insert(COMPONENT_ID_MASK(start));

    // First, get the component
    ConfigComponent* comp = comps_[start];
    comp->visited         = true;

    for ( LinkId_t id : comp->allLinks() ) {
        ConfigLink* link = links_[id];

        // If this is a no_cut link, need to follow it to next
        // component if next component is not already in group
        if ( link->no_cut ) {
            ComponentId_t id = COMPONENT_ID_MASK(
                (COMPONENT_ID_MASK(link->component[0]) == COMPONENT_ID_MASK(start) ? link->component[1]
                                                                                   : link->component[0]));
            // Check to see if this id is already in the group.  We
            // can do it one of two ways: check the visited variable,
            // or see if it is in the group set already.  We look in
            // the group set because they are both lookups into
            // associative structures, but the group will be much
            // smaller.
            if ( group.find(id) == group.end() ) {
                getConnectedNoCutComps(id, group);
            }
        }
    }
}

void
ConfigGraph::setComponentConfigGraphPointers()
{
    for ( auto* x : comps_ ) {
        x->setConfigGraphPointer(this);
    }
}

void
ConfigGraph::restoreRestartData()
{
    SST::Core::Serialization::serializer ser;
    ser.enable_pointer_tracking();

    // Load libraries from the checkpoint
    Factory::getFactory()->loadUnloadedLibraries(*(cpt_libnames.get()));

    // Initialize SharedObjectManager
    auto* vec_som = cpt_shared_objects.get();
    ser.start_unpacking(vec_som->data(), vec_som->size());
    Simulation_impl::serializeSharedObjectManager(ser);

    // Get the stats config
    auto* vec_sc = cpt_stats_config.get();
    ser.start_unpacking(vec_sc->data(), vec_sc->size());
    SST_SER(Simulation_impl::stats_config_);

    cpt_libnames.reset();
    cpt_shared_objects.reset();
    cpt_stats_config.reset();
}

void
PartitionComponent::print(std::ostream& os, const PartitionGraph* graph) const
{
    os << "Component " << id << "  ( ";
    for ( ComponentIdMap_t::const_iterator git = group.begin(); git != group.end(); ++git ) {
        os << *git << " ";
    }
    os << ")" << std::endl;
    os << "  weight = " << weight << std::endl;
    os << "  rank = " << rank.rank << std::endl;
    os << "  thread = " << rank.thread << std::endl;
    os << "  Links:" << std::endl;
    for ( LinkIdMap_t::const_iterator it = links.begin(); it != links.end(); ++it ) {
        graph->getLink(*it).print(os);
    }
}

} // namespace SST
