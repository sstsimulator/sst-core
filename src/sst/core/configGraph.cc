// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/configGraph.h"

#include <fstream>
#include <algorithm>

#include "sst/core/component.h"
#include "sst/core/config.h"
#include "sst/core/timeLord.h"
#include "sst/core/simulation.h"
#include "sst/core/factory.h"
#include "sst/core/from_string.h"

#include <string.h>

using namespace std;

namespace SST {


void ConfigLink::updateLatencies(TimeLord *timeLord)
{
    latency[0] = timeLord->getSimCycles(latency_str[0], __FUNCTION__);
    latency[1] = timeLord->getSimCycles(latency_str[1], __FUNCTION__);
}

void ConfigStatistic::addParameter(const std::string& key, const std::string& value, bool overwrite)
{
    bool bk = params.enableVerify(false);
    params.insert(key, value, overwrite);
    params.enableVerify(bk);
}

bool ConfigStatGroup::addComponent(ComponentId_t id)
{
    if ( std::find(components.begin(), components.end(), id) == components.end() ) {
        components.push_back(id);
    }
    return true;
}


bool ConfigStatGroup::addStatistic(const std::string& name, Params &p)
{
    statMap[name] = p;
    if ( outputFrequency.getRoundedValue() == 0 ) {
        /* aka, not yet really set to anything other than 0 */
        setFrequency(p.find<std::string>("rate", "0ns"));
    }
    return true;
}


bool ConfigStatGroup::setOutput(size_t id)
{
    outputID = id;
    return true;
}


bool ConfigStatGroup::setFrequency(const std::string& freq)
{
    UnitAlgebra uaFreq(freq);
    if ( uaFreq.hasUnits("s") || uaFreq.hasUnits("hz") ) {
        outputFrequency = uaFreq;
        return true;
    }
    return false;
}


std::pair<bool, std::string> ConfigStatGroup::verifyStatsAndComponents(const ConfigGraph *graph)
{
    for ( auto & id : components ) {
        const ConfigComponent* comp = graph->findComponent(id);
        if ( !comp ) {
            std::stringstream ss;
            ss << "Component id " << id << " is not a valid component";
            return std::make_pair(false, ss.str());
        }
        for ( auto & statKV : statMap ) {

            bool ok = Factory::getFactory()->DoesComponentInfoStatisticNameExist(comp->type, statKV.first);

            if ( !ok ) {
                std::stringstream ss;
                ss << "Component " << comp->name << " does not support statistic " << statKV.first;
                return std::make_pair(false, ss.str());
            }
        }
    }

    return std::make_pair(true, "");
}


void ConfigComponent::print(std::ostream &os) const {
    os << "Component " << name << " (id = " << std::hex << id << std::dec << ")" << std::endl;
    os << "  slot_num = " << slot_num << std::endl;
    os << "  type = " << type << std::endl;
    os << "  weight = " << weight << std::endl;
    os << "  rank = " << rank.rank << std::endl;
    os << "  thread = " << rank.thread << std::endl;
    os << "  Links:" << std::endl;
    for (size_t i = 0 ; i != links.size() ; ++i) {
        os << "    " << links[i];
    }
    os << std::endl;
    os << "  Params:" << std::endl;
    params.print_all_params(os, "    ");
    os << "  Statistics:" << std::endl;
    for ( auto & si : enabledStatistics ) {
        os << "    " << si.name << std::endl;
        os << "      Params:" << std::endl;
        si.params.print_all_params(os, "      ");
    }
    os << "  SubComponents:\n";
    for ( auto & sc : subComponents ) {
        sc.print(os);
    }
}

ConfigComponent
ConfigComponent::cloneWithoutLinks() const
{
    ConfigComponent ret;
    ret.id = id;
    ret.name = name;
    ret.slot_num = slot_num;
    ret.type = type;
    ret.weight = weight;
    ret.rank = rank;
    ret.params = params;
    ret.statLoadLevel = statLoadLevel;
    ret.enabledStatistics = enabledStatistics;
    ret.coords = coords;
    for ( auto &i : subComponents ) {
        ret.subComponents.emplace_back(i.cloneWithoutLinks());
    }
    return ret;
}


ConfigComponent
ConfigComponent::cloneWithoutLinksOrParams() const
{
    ConfigComponent ret;
    ret.id = id;
    ret.name = name;
    ret.slot_num = slot_num;
    ret.type = type;
    ret.weight = weight;
    ret.rank = rank;
    ret.statLoadLevel = statLoadLevel;
    ret.enabledStatistics = enabledStatistics;
    ret.coords = coords;
    for ( auto &i : subComponents ) {
        ret.subComponents.emplace_back(i.cloneWithoutLinksOrParams());
    }
    return ret;
}

ComponentId_t ConfigComponent::getNextSubComponentID()
{
    // If we are the ultimate component, get nextSubID and increment
    // for next time
    if ( id == COMPONENT_ID_MASK(id) ) {
        uint16_t subid = nextSubID;
        nextSubID++;
        return SUBCOMPONENT_ID_CREATE( id, subid );
    }
    else {
        // Get the ultimate parent and call getNextSubComponentID on
        // it
        return graph->findComponent(COMPONENT_ID_MASK(id))->getNextSubComponentID();
    }
        
}

StatisticId_t ConfigComponent::getNextStatisticID()
{
    // If we are the ultimate component, get nextStatID and increment
    // for next time
    if ( id == COMPONENT_ID_MASK(id) ) {
        uint16_t statId = nextStatID;
        nextStatID++;
        return STATISTIC_ID_CREATE( id, statId );
    }
    else {
        // Get the ultimate parent and call getNextStatisticID on
        // it
        return graph->findComponent(COMPONENT_ID_MASK(id))->getNextStatisticID();
    }
}

ConfigComponent* ConfigComponent::getParent() const {
    if ( id == COMPONENT_ID_MASK(id) ) {
        return nullptr;
    }
    return graph->findComponent( (((ComponentId_t)nextSubID) << COMPONENT_ID_BITS) | COMPONENT_ID_MASK(id) );
}


std::string ConfigComponent::getFullName() const {
    if ( id == COMPONENT_ID_MASK(id) ) {
        // We are a component
        return name;
    }

    // Get full name of parent
    std::string parent_name = getParent()->getFullName();

    // For ConfigComponent, we will always put in [] for the slot number.
    return parent_name + ":" + name + "[" + std::to_string(slot_num) + "]";
}


void ConfigComponent::setRank(RankInfo r)
{
    rank = r;
    for ( auto &i : subComponents ) {
        i.setRank(r);
    }

}


void ConfigComponent::setWeight(double w)
{
    weight = w;
    for ( auto &i : subComponents ) {
        i.setWeight(w);
    }
}

void ConfigComponent::setCoordinates(const std::vector<double> &c)
{
    coords = c;
    /* Maintain minimum of 3D information */
    while ( coords.size() < 3 )
        coords.push_back(0.0);
}


void ConfigComponent::addParameter(const std::string& key, const std::string& value, bool overwrite)
{
    bool bk = params.enableVerify(false);
    params.insert(key, value, overwrite);
    params.enableVerify(bk);
}

void ConfigComponent::enableStatistic(const std::string& statisticName, bool recursively)
{
    // NOTE: For every statistic in the enabledStatistics List, there must be
    //       a corresponding params entry in enabledStatParams list.  The two
    //       lists will always be the same size.

    if ( recursively ) {
        for ( auto &sc : subComponents ) {
            sc.enableStatistic(statisticName, true);
        }
    }

    // Check for Enable All Statistics
    if (statisticName == STATALLFLAG) {
        // Force the STATALLFLAG to always be on the bottom of the list.
        // First check to see if anything is in the vector, if vector is empty,
        // a STATALLFLAG flag will be added to the vector
        if (false == enabledStatistics.empty()) {
            // The vector is populated, so see if the STATALLFLAG
            // already exists if it does, we are done
            if (STATALLFLAG != enabledStatistics.back().name) {
                // Add a STATALLFLAG to end of the vector
                enabledStatistics.emplace_back(STATALLFLAG);
            }
        } else {
            // Add a STATALLFLAG to end of the vector
            enabledStatistics.emplace_back(STATALLFLAG);
        }
    } else {
        // Check to see if the stat is already in the list
        for ( auto & si : enabledStatistics ) {
            if ( statisticName == si.name ) {
                // We found the name already in the enabledStatistics list, do nothing
                return;
            }
        }

        // statisticName not in list, so add statistic and params to top of the vectors
        enabledStatistics.emplace(enabledStatistics.begin(), statisticName);
    }
}


void ConfigComponent::addStatisticParameter(const std::string& statisticName, const std::string& param, const std::string& value, bool recursively)
{
    // NOTE: For every statistic in the enabledStatistics List, there must be
    //       a corresponding params entry in enabledStatParams list.  The two
    //       lists will always be the same size.
    if ( recursively ) {
        for ( auto &sc : subComponents ) {
            sc.addStatisticParameter(statisticName, param, value, true);
        }
    }


    // Scan the enabledStatistics list for the statistic name
    for ( auto & si : enabledStatistics ) {
        // Check to see if the names match.  NOTE this also works for the STATALLFLAG
        if ( statisticName == si.name ) {
            // Add/set the parameter
            si.params.insert(param, value);
        }
    }
}


void ConfigComponent::setStatisticParameters(const std::string& statisticName, const Params &params, bool recursively)
{
    if ( recursively ) {
        for ( auto &sc : subComponents ) {
            sc.setStatisticParameters(statisticName, params, true);
        }
    }

    for ( auto & si : enabledStatistics ) {
        // Check to see if the names match.  NOTE this also works for the STATALLFLAG
        if ( statisticName == si.name ) {
            si.params.insert(params);
        }
    }

}

void ConfigComponent::setStatisticLoadLevel(uint8_t level, bool recursively)
{
    statLoadLevel = level;

    if ( recursively ) {
        for ( auto &sc : subComponents ) {
            sc.setStatisticLoadLevel(level, true);
        }
    }
}


ConfigComponent* ConfigComponent::addSubComponent(ComponentId_t sid, const std::string& name, const std::string& type, int slot_num)
{
    /* Check for existing subComponent with this name */
    for ( auto &i : subComponents ) {
        if ( i.name == name && i.slot_num == slot_num )
            return nullptr;
    }

    uint16_t parent_sub_id = SUBCOMPONENT_ID_MASK(id);

    subComponents.emplace_back(
        ConfigComponent(sid, graph, parent_sub_id, name, slot_num, type, this->weight, this->rank));

    return &(subComponents.back());
}

ConfigComponent* ConfigComponent::findSubComponent(ComponentId_t sid)
{
    return const_cast<ConfigComponent*>(const_cast<const ConfigComponent*>(this)->findSubComponent(sid));
}

const ConfigComponent* ConfigComponent::findSubComponent(ComponentId_t sid) const
{
    if ( sid == this->id ) return this;

    for ( auto &s : subComponents ) {
        const ConfigComponent* res = s.findSubComponent(sid);
        if ( res != nullptr )
            return res;
    }

    return nullptr;
}

ConfigComponent* ConfigComponent::findSubComponentByName(const std::string& name)
{
    size_t colon_index = name.find(":");
    std::string slot = name.substr(0,colon_index);

    // Get the slot number
    int slot_num = 0;
    size_t bracket_index = slot.find("[");
    if ( bracket_index == std::string::npos ) {
        // No brackets, slot_num 0
        slot_num = 0;
    }
    else {
        size_t close_index = slot.find("]");
        size_t length = close_index - bracket_index - 1;
        slot_num = Core::from_string<int>(slot.substr(bracket_index+1,length));
        slot = slot.substr(0,bracket_index);
    }

    // Now, see if we have something in this slot and slot_num
    for ( auto& sc : subComponents ) {
        if ( sc.name == slot && sc.slot_num == slot_num ) {
            // Found the subcomponent
            if ( colon_index == std::string::npos ) {
                // Last level of hierarchy
                return &sc;
            }
            else {
                return sc.findSubComponentByName(slot.substr(colon_index+1,std::string::npos));
            }
        }
    }
    return nullptr;
}

ConfigStatistic* ConfigComponent::addStatistic(StatisticId_t sid, const std::string& statisticName)
{
    // Check for Enable All Statistics
    if (statisticName == STATALLFLAG) {
        // Force the STATALLFLAG to always be on the bottom of the list.
        // First check to see if anything is in the vector, if vector is empty,
        // a STATALLFLAG flag will be added to the vector
        if (false == enabledStatistics.empty()) {
            // The vector is populated, so see if the STATALLFLAG
            // already exists if it does, we are done
            if (STATALLFLAG != enabledStatistics.back().name) {
                // Add a STATALLFLAG to end of the vector
                enabledStatistics.emplace_back(STATALLFLAG);
                return &(enabledStatistics.back());
            }
        } else {
            // Add a STATALLFLAG to end of the vector
            enabledStatistics.emplace_back(STATALLFLAG);
            return &(enabledStatistics.back());
        }
    } else {

      /* Check for existing statistic with this name */
      for ( auto &i : enabledStatistics ) {
          if ( i.name == statisticName)
          {
              return nullptr;
          }
      }

      enabledStatistics.emplace(enabledStatistics.begin(),
          ConfigStatistic(sid, id, statisticName));


      return &(enabledStatistics.front());
    }

    return nullptr;
}

ConfigStatistic* ConfigComponent::findStatistic(StatisticId_t sid) const
{
    // Check for the current component statistics
    for ( const ConfigStatistic &s : enabledStatistics ) {
        if ( s.id == sid ) {
            ConfigStatistic* res = const_cast<ConfigStatistic*>(&s);
            if ( res != nullptr ) {
                return res;
            }
        }
    }

    // Check for the subComponents statistics
    for ( auto &s : subComponents ) {
        ConfigStatistic* res = s.findStatistic(sid);
        if ( res != nullptr ) {
            return res;
        }
    }

    return nullptr;
}

std::vector<LinkId_t> ConfigComponent::allLinks() const {
    std::vector<LinkId_t> res;
    res.insert(res.end(), links.begin(), links.end());
    for ( auto& sc : subComponents ) {
        std::vector<LinkId_t> s = sc.allLinks();
        res.insert(res.end(), s.begin(), s.end());
    }
    return res;
}

void
ConfigComponent::checkPorts() const
{
    std::map<std::string,std::string> ports;

    auto& graph_links = graph->getLinkMap();

    // Loop over all the links
    for ( unsigned int i = 0; i < links.size(); i++ ) {
        const ConfigLink& link = graph_links[links[i]];
        for ( int j = 0; j < 2; j++ ) {

            if ( link.component[j] == id ) {
                // If port is not found, print an error
                if (!Factory::getFactory()->isPortNameValid(type, link.port[j]) ) {
                    // For now this is not a fatal error
                    // found_error = true;
                    Output::getDefaultObject().fatal(CALL_INFO, 1, "ERROR:  Attempting to connect to unknown port: %s, "
                                 "in component %s of type %s.\n",
                                 link.port[j].c_str(), name.c_str(), type.c_str());
                }

                // Check for multiple links hooked to port
                auto ret = ports.insert(std::make_pair(link.port[j],link.name));
                if ( !ret.second ) {
#ifndef SST_ENABLE_PREVIEW_BUILD
                    Output::getDefaultObject().output("Warning: Port %s of Component %s connected to two links: %s, %s (this will become a fatal error in SST 11)\n",
                                  link.port[j].c_str(),name.c_str(),link.name.c_str(),ret.first->second.c_str());

#else
                    Output::getDefaultObject().fatal(CALL_INFO,1,"ERROR: Port %s of Component %s connected to two links: %s, %s.\n",
                                 link.port[j].c_str(),name.c_str(),link.name.c_str(),ret.first->second.c_str());
#endif
                }
            }
        }
    }

    // Now loop over all subcomponents and call the check function
    for ( auto& subcomp : subComponents ) {
        subcomp.checkPorts();
    }
}

void
ConfigGraph::setComponentRanks(RankInfo rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
                            iter != comps.end(); ++iter )
    {
        iter->setRank(rank);
    }
}

bool
ConfigGraph::containsComponentInRank(RankInfo rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
                            iter != comps.end(); ++iter )
    {
        if ( iter->rank == rank ) return true;
    }
    return false;

}

bool
ConfigGraph::checkRanks(RankInfo ranks)
{
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
                            iter != comps.end(); ++iter )
    {
        if ( !iter->rank.isAssigned() || !ranks.inRange(iter->rank) ) {
            fprintf(stderr, "Bad rank: %u %u\n", iter->rank.rank, iter->rank.thread);
            return false;
        }
    }
    return true;
}

void
ConfigGraph::postCreationCleanup()
{
    TimeLord *timeLord = Simulation::getTimeLord();
    for ( ConfigLink &link : getLinkMap() ) {
        link.updateLatencies(timeLord);
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
    for( ConfigLinkMap_t::iterator iter = links.begin();
         iter != links.end(); ++iter )
    {
        ConfigLink& clink = *iter;
        // This one should never happen since the slots are
        // initialized in order, but just in case...
        if ( clink.component[0] == ULONG_MAX ) {
            output.output("WARNING:  Found dangling link: %s.  It is connected on one side to component %s.\n",clink.name.c_str(),
                   comps[clink.component[1]].name.c_str());
            found_error = true;
        }
        if ( clink.component[1] == ULONG_MAX ) {
            output.output("WARNING:  Found dangling link: %s.  It is connected on one side to component %s.\n",clink.name.c_str(),
                   comps[clink.component[0]].name.c_str());
            found_error = true;
        }
    }


    // Check to see if all the port names are valid and they are only
    // used once

    // Loop over all the Components
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
          iter != comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);
        ccomp->checkPorts();
    }

    return found_error;
}


ComponentId_t
ConfigGraph::addComponent(const std::string& name, const std::string& type, float weight, RankInfo rank)
{
    ComponentId_t cid = nextComponentId++;
    comps.push_back(ConfigComponent(cid, this, name, type, weight, rank));

    auto ret = compsByName.insert(std::make_pair(name,cid));
    // Check to see if the name has already been used
    if ( !ret.second ) {
        output.fatal(CALL_INFO,1,"ERROR: trying to add Component with name that already exists: %s\n",name.c_str());
    }
    return cid;
}

ComponentId_t
ConfigGraph::addComponent(const std::string& name, const std::string& type)
{
    ComponentId_t cid = nextComponentId++;
    comps.push_back(ConfigComponent(cid, this, name, type, 1.0f, RankInfo()));

    auto ret = compsByName.insert(std::make_pair(name,cid));
    // Check to see if the name has already been used
    if ( !ret.second ) {
        output.fatal(CALL_INFO,1,"ERROR: trying to add Component with name that already exists: %s\n",name.c_str());
    }
    return cid;
}



void
ConfigGraph::setStatisticOutput(const std::string& name)
{
    statOutputs[0].type = name;
}

void
ConfigGraph::setStatisticOutputParams(const Params& p)
{
    statOutputs[0].params = p;
}

void
ConfigGraph::addStatisticOutputParameter(const std::string& param, const std::string& value)
{
    statOutputs[0].params.insert(param, value);
}

void
ConfigGraph::setStatisticLoadLevel(uint8_t loadLevel)
{
    statLoadLevel = loadLevel;
}



void
ConfigGraph::enableStatisticForComponentName(const std::string& ComponentName, const std::string& statisticName, bool recursively)
{
    bool found;

    // Search all the components for a matching name
    for (ConfigComponentMap_t::iterator iter = comps.begin(); iter != comps.end(); ++iter) {
        // Check to see if the names match or All components are selected
        found = ((ComponentName == iter->name) || (ComponentName == STATALLFLAG));
        if (true == found) {
            comps[iter->id].enableStatistic(statisticName, (ComponentName == STATALLFLAG) || recursively );
        }
    }
}





template <class PredicateFunc, class UnaryFunc>
size_t for_each_subcomp_if(ConfigComponent &c, PredicateFunc p, UnaryFunc f) {
    size_t count = 0;
    if ( p(c) ) {
        count++;
        f(c);
    }
    for ( auto &sc : c.subComponents ) {
        count += for_each_subcomp_if(sc, p, f);
    }
    return count;
}


void
ConfigGraph::enableStatisticForComponentType(const std::string& ComponentType, const std::string& statisticName, bool recursively)
{
    if ( ComponentType == STATALLFLAG ) {
        for ( auto &c : comps ) {
            for_each_subcomp_if(c,
                    [](ConfigComponent & UNUSED(c)) -> bool {return true;},
                                [statisticName](ConfigComponent &c){ c.enableStatistic(statisticName); } );
        }
    } else {
        for ( auto &c : comps ) {
            for_each_subcomp_if(c,
                    [ComponentType](ConfigComponent &c) -> bool { return c.type == ComponentType; },
                                [statisticName,recursively](ConfigComponent &c){ c.enableStatistic(statisticName,recursively);} );
        }
    }
}

void
ConfigGraph::setStatisticLoadLevelForComponentType(const std::string& ComponentType, uint8_t level, bool recursively)
{
    if ( ComponentType == STATALLFLAG ) {
        for ( auto &c : comps ) {
            for_each_subcomp_if(c,
                    [](ConfigComponent & UNUSED(c)) -> bool {return true;},
                                [level](ConfigComponent &c){ c.setStatisticLoadLevel(level); } );
        }
    } else {
        for ( auto &c : comps ) {
            for_each_subcomp_if(c,
                    [ComponentType](ConfigComponent &c) -> bool { return c.type == ComponentType; },
                                [level,recursively](ConfigComponent &c){ c.setStatisticLoadLevel(level,recursively);} );
        }
    }
}

void
ConfigGraph::addStatisticParameterForComponentName(const std::string& ComponentName, const std::string& statisticName, const std::string& param, const std::string& value, bool recursively)
{
    bool found;

    // Search all the components for a matching name
    for (ConfigComponentMap_t::iterator iter = comps.begin(); iter != comps.end(); ++iter) {
        // Check to see if the names match or All components are selected
        found = ((ComponentName == iter->name) || (ComponentName == STATALLFLAG));
        if (true == found) {
            comps[iter->id].addStatisticParameter(statisticName, param, value, (ComponentName == STATALLFLAG) || recursively);
        }
    }
}

void
ConfigGraph::addStatisticParameterForComponentType(const std::string& ComponentType, const std::string& statisticName, const std::string& param, const std::string& value, bool recursively)
{
    if ( ComponentType == STATALLFLAG ) {
        for ( auto &c : comps ) {
            for_each_subcomp_if(c,
                    [](ConfigComponent & UNUSED(c)) -> bool {return true;},
                                [statisticName, param, value](ConfigComponent &c){ c.addStatisticParameter(statisticName, param, value); } );
        }
    } else {
        for ( auto &c : comps ) {
            for_each_subcomp_if(c,
                    [ComponentType](ConfigComponent &c) -> bool { return c.type == ComponentType; },
                                [statisticName, param, value, recursively](ConfigComponent &c){ c.addStatisticParameter(statisticName, param, value, recursively);} );
        }
    }
}

void
ConfigGraph::addLink(ComponentId_t comp_id, const std::string& link_name, const std::string& port, const std::string& latency_str, bool no_cut)
{
    if ( link_names.find(link_name) == link_names.end() ) {
        LinkId_t id = links.size();
        link_names[link_name] = id;
        links.insert(ConfigLink(id, link_name));
    }
    ConfigLink &link = links[link_names[link_name]];
    if ( link.current_ref >= 2 ) {
        output.fatal(CALL_INFO,1,"ERROR: Parsing SDL file: Link %s referenced more than two times\n",link_name.c_str());
    }

    // Convert the latency string to a number

    int index = link.current_ref++;
    link.component[index] = comp_id;
    link.port[index] = port;
    link.latency_str[index] = latency_str;
    link.no_cut = link.no_cut | no_cut;

    // Check to make sure the link doesn't already exist in the component
    auto compLinks = &findComponent(comp_id)->links;
    if (std::find(compLinks->begin(), compLinks->end(), link.id) == compLinks->end()){
        compLinks->push_back(link.id);
    }
}

void
ConfigGraph::setLinkNoCut(const std::string& link_name)
{
    // If link doesn't exist, return
    if ( link_names.find(link_name) == link_names.end() ) return;

    ConfigLink &link = links[link_names[link_name]];
    link.no_cut = true;
}



bool ConfigGraph::containsComponent(ComponentId_t id) const {
    return comps.contains(id);
}

ConfigComponent* ConfigGraph::findComponent(ComponentId_t id)
{
    return const_cast<ConfigComponent*>(const_cast<const ConfigGraph*>(this)->findComponent(id));
}


const ConfigComponent* ConfigGraph::findComponent(ComponentId_t id) const
{
    /* Check to make sure we're part of the same component */
    if ( COMPONENT_ID_MASK(id) == id ) {
        return &comps[id];
    }

    return comps[COMPONENT_ID_MASK(id)].findSubComponent(id);
}

ConfigComponent* ConfigGraph::findComponentByName(const std::string& name) {
    std::string origname(name);
    auto index = origname.find(":");
    std::string compname = origname.substr(0,index);
    auto itr = compsByName.find(compname);

    // Check to see if component was found
    if ( itr == compsByName.end() ) return nullptr;

    ConfigComponent* cc = &comps[itr->second];

    // If this was just a component name
    if ( index == std::string::npos ) return cc;

    // See if this is a valid subcomponent name
    cc = cc->findSubComponentByName(origname.substr(index+1,std::string::npos));
    if ( cc ) return cc;
    return nullptr;
}

ConfigStatistic* ConfigGraph::findStatistic(StatisticId_t id) const
{
   return comps[COMPONENT_ID_MASK(id)].findStatistic(id);
}

ConfigGraph*
ConfigGraph::getSubGraph(uint32_t start_rank, uint32_t end_rank)
{
    set<uint32_t> rank_set;
    for ( uint32_t i = start_rank; i <= end_rank; i++ ) {
        rank_set.insert(i);
    }
    return getSubGraph(rank_set);
}

ConfigGraph*
ConfigGraph::getSubGraph(const std::set<uint32_t>& rank_set)
{
    ConfigGraph* graph = new ConfigGraph();

    // SparseVectorMap is a extremely slow at random inserts, so make
    // sure things go in in order into both comps and links, then tie
    // it all together.
    for ( ConfigComponentMap_t::iterator it = comps.begin(); it != comps.end(); ++it) {
        const ConfigComponent& comp = *it;

        if ( rank_set.find(comp.rank.rank) != rank_set.end() ) {
            graph->comps.push_back(comp.cloneWithoutLinks());
        }
        else {
            // See if the other side of any of component's links is in
            // set, if so, add to graph
            for ( LinkId_t l : comp.allLinks() ) {
                const ConfigLink& link = links[l];
                ComponentId_t remote = COMPONENT_ID_MASK(link.component[0]) == COMPONENT_ID_MASK(comp.id) ?
                    link.component[1] : link.component[0];
                if ( rank_set.find(comps[COMPONENT_ID_MASK(remote)].rank.rank) != rank_set.end() ) {
                    graph->comps.push_back(comp.cloneWithoutLinksOrParams());
                    break;
                }
            }
        }
    }

    // Look through all the links.  Add any link that has either side
    // hooked to a component in the specified rank set.  Then add link
    // to components (which are already in the graph)
    for ( ConfigLinkMap_t::iterator it = links.begin(); it != links.end(); ++it ) {
        const ConfigLink& link = *it;

        const ConfigComponent* comp0 = findComponent(link.component[0]);
        const ConfigComponent* comp1 = findComponent(link.component[1]);

        bool comp0_in_ranks = (rank_set.find(comp0->rank.rank) != rank_set.end());
        bool comp1_in_ranks = (rank_set.find(comp1->rank.rank) != rank_set.end());

        if ( comp0_in_ranks || comp1_in_ranks ) {
            // Clone the link and add to new lin k map
            graph->links.insert(ConfigLink(link));  // Will make a copy into map

            graph->findComponent(comp0->id)->links.push_back(link.id);
            graph->findComponent(comp1->id)->links.push_back(link.id);
        }
    }

    // Copy the statistic configuration to the sub-graph
    graph->statOutputs = this->statOutputs;
    /* Only need to copy StatGroups which are referenced in this subgraph */
    for ( auto & kv : this->statGroups ) {
        for ( auto & id : kv.second.components ) {
            if ( graph->containsComponent(id) ) {
                graph->statGroups.insert(std::make_pair(kv.first, kv.second));
                break;
            }
        }
    }
    graph->setStatisticLoadLevel(this->getStatLoadLevel());

    return graph;
}

PartitionGraph*
ConfigGraph::getPartitionGraph()
{
    PartitionGraph* graph = new PartitionGraph();

    PartitionComponentMap_t& pcomps = graph->getComponentMap();
    PartitionLinkMap_t& plinks = graph->getLinkMap();

    // SparseVectorMap is slow for random inserts, so make sure we
    // insert both components and links in order of ID, which is the
    // key for the SparseVectorMap
    for ( ConfigComponentMap_t::iterator it = comps.begin(); it != comps.end(); ++it ) {
        const ConfigComponent& comp = *it;

        pcomps.insert(PartitionComponent(comp));
    }


    for ( ConfigLinkMap_t::iterator it = links.begin(); it != links.end(); ++it ) {
        const ConfigLink& link = *it;

        const ConfigComponent& comp0 = comps[COMPONENT_ID_MASK(link.component[0])];
        const ConfigComponent& comp1 = comps[COMPONENT_ID_MASK(link.component[1])];

        plinks.insert(PartitionLink(link));

        pcomps[comp0.id].links.push_back(link.id);
        pcomps[comp1.id].links.push_back(link.id);
    }
    return graph;
}

PartitionGraph*
ConfigGraph::getCollapsedPartitionGraph()
{
    PartitionGraph* graph = new PartitionGraph();

    SparseVectorMap<LinkId_t> deleted_links;

    PartitionComponentMap_t& pcomps = graph->getComponentMap();
    PartitionLinkMap_t& plinks = graph->getLinkMap();

    // SparseVectorMap is slow for random inserts, so make sure we
    // insert both components and links in order of ID, which is the
    // key for the SparseVectorMap in both cases
    ComponentIdMap_t group;
    for ( ConfigComponentMap_t::iterator it = comps.begin(); it != comps.end(); ++it ) {
        const ConfigComponent& comp = *it;

        // Get the no-cut group for this component
        group.clear();
        getConnectedNoCutComps(it->id,group);


        // Check to see if this has already been put in map.  Do this
        // by seeing if the first item in the connected components is
        // the current ID.  If not, then it's already in the list.
        if ( *group.begin() == comp.id ) {
            ComponentId_t id = pcomps.size();
            pcomps.insert(PartitionComponent(id));
            PartitionComponent& pcomp = pcomps[id];

            // Iterate over the group and add the weights and add any
            // links that connect outside the group
            for ( ComponentIdMap_t::const_iterator i = group.begin(); i != group.end(); ++i ) {
                const ConfigComponent& comp = comps[*i];
                // Compute the new weight
                pcomp.weight += comp.weight;
                pcomp.group.insert(comp.id);

                // Walk through all the links and insert the ones that connect
                // outside the group
                for ( LinkId_t id : comp.allLinks() ) {
                    const ConfigLink& link = links[id];

                    if ( !group.contains(COMPONENT_ID_MASK(link.component[0])) || !group.contains(COMPONENT_ID_MASK(link.component[1]) ) ) {
                        pcomp.links.push_back(link.id);
                    }
                    else {
                        deleted_links.insert(link.id);
                    }
                }
            }
        }
    }

    // Now add all but the deleted links to the partition graph
    for ( ConfigLinkMap_t::iterator i = links.begin(); i != links.end(); ++i ) {
        if ( !deleted_links.contains(i->id) ) plinks.push_back(*i);
    }

    // Just need to fix up the component fields for the links.  Do
    // this by walking through the components and checking each of it
    // links to see if it points to something in the group.  If so,
    // change ID to point to super group.
    for ( PartitionComponentMap_t::iterator i = pcomps.begin(); i != pcomps.end(); ++i ) {
        PartitionComponent& pcomp = *i;
        for ( LinkIdMap_t::iterator j = pcomp.links.begin(); j != pcomp.links.end(); ++j ) {
            PartitionLink& plink = plinks[*j];
            if ( pcomp.group.contains(plink.component[0]) ) plink.component[0] = pcomp.id;
            if ( pcomp.group.contains(plink.component[1]) ) plink.component[1] = pcomp.id;
        }
    }

    return graph;
}

void
ConfigGraph::annotateRanks(PartitionGraph* graph)
{
    PartitionComponentMap_t& pcomps = graph->getComponentMap();

    for ( PartitionComponentMap_t::iterator it = pcomps.begin(); it != pcomps.end(); ++it ) {
        const PartitionComponent& comp = *it;

        for ( ComponentIdMap_t::const_iterator c_iter = comp.group.begin();
              c_iter != comp.group.end(); ++ c_iter ) {
            comps[*c_iter].setRank(comp.rank);
        }
    }
}

void
ConfigGraph::getConnectedNoCutComps(ComponentId_t start, ComponentIdMap_t& group)
{
    // We'll do this as a simple recursive depth first search
    group.insert(COMPONENT_ID_MASK(start));

    // First, get the component
    ConfigComponent& comp = comps[start];

    for ( LinkId_t id : comp.allLinks() ) {
        ConfigLink& link = links[id];

        // If this is a no_cut link, need to follow it to next
        // component if next component is not already in group
        if ( link.no_cut ) {
            ComponentId_t id = COMPONENT_ID_MASK((COMPONENT_ID_MASK(link.component[0]) == COMPONENT_ID_MASK(start) ? link.component[1] : link.component[0]));
            if ( !group.contains(id) ) {
                getConnectedNoCutComps(id,group);
            }
        }
    }
}

void
PartitionComponent::print(std::ostream &os, const PartitionGraph* graph) const
{
    os << "Component " << id << "  ( ";
    for ( ComponentIdMap_t::const_iterator git = group.begin(); git != group.end(); ++git ) {
        os << *git << " ";
    }
    os << ")" << endl;
    os << "  weight = " << weight << std::endl;
    os << "  rank = " << rank.rank << std::endl;
    os << "  thread = " << rank.thread << std::endl;
    os << "  Links:" << std::endl;
    for ( LinkIdMap_t::const_iterator it = links.begin(); it != links.end(); ++it ) {
        graph->getLink(*it).print(os);
    }
}


} // namespace SST

