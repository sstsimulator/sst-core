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


#include <sst_config.h>
#include <sst/core/configGraph.h>

#include <fstream>

#include <sst/core/component.h>
#include <sst/core/config.h>
#include <sst/core/timeLord.h>
#include <sst/core/simulation.h>

#include <string.h>

#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif

using namespace std;

namespace SST {


void ConfigLink::updateLatencies(TimeLord *timeLord)
{
    latency[0] = timeLord->getSimCycles(latency_str[0], __FUNCTION__);
    latency[1] = timeLord->getSimCycles(latency_str[1], __FUNCTION__);
}


void ConfigComponent::print(std::ostream &os) const {
    os << "Component " << name << " (id = " << id << ")" << std::endl;
    os << "  type = " << type << std::endl;
    os << "  weight = " << weight << std::endl;
    os << "  rank = " << rank.rank << std::endl;
    os << "  thread = " << rank.thread << std::endl;
    os << "  isIntrospector = " << isIntrospector << std::endl;
    os << "  Links:" << std::endl;
    for (size_t i = 0 ; i != links.size() ; ++i) {
        os << "    " << links[i];
    }
    os << std::endl;
    os << "  Params:" << std::endl;
    params.print_all_params(os, "    ");
    os << "  Statistics:" << std::endl;
    for (size_t x = 0 ; x != enabledStatistics.size() ; ++x) {
        os << "    " << enabledStatistics[x] << std::endl;
        os << "      Params:" << std::endl;
        enabledStatParams[x].print_all_params(os, "      ");
    }
}

ConfigComponent
ConfigComponent::cloneWithoutLinks() const
{
    ConfigComponent ret;
    ret.id = id;
    ret.name = name;
    ret.type = type;
    ret.weight = weight;
    ret.rank = rank;
    ret.params = params;
    ret.isIntrospector = isIntrospector;
    ret.enabledStatistics = enabledStatistics;
    ret.enabledStatParams = enabledStatParams;
    return ret;
}
    
ConfigComponent
ConfigComponent::cloneWithoutLinksOrParams() const
{
    ConfigComponent ret;
    ret.id = id;
    ret.name = name;
    ret.type = type;
    ret.weight = weight;
    ret.rank = rank;
    ret.isIntrospector = isIntrospector;
    ret.enabledStatistics = enabledStatistics;
    ret.enabledStatParams = enabledStatParams;
    return ret;
}
    
void
ConfigGraph::setComponentRanks(RankInfo rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
                            iter != comps.end(); ++iter )
    {
        iter->rank = rank;
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


bool
ConfigGraph::checkForStructuralErrors()
{
    // Output object for error messages
    Output &output = Output::getDefaultObject();
    
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

    // Check to make sure all the component names are unique.  This
    // could be memory intensive for large graphs because we will
    // simply put things in a set and check to see if there are
    // duplicates.
    std::set<std::string> name_set;
    int count = 10;
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
          iter != comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);
        if ( name_set.find(ccomp->name) == name_set.end() ) {
            name_set.insert(ccomp->name);
        }
        else {
            found_error = true;
            output.output("WARNING:  Found duplicate component name: %s\n",ccomp->name.c_str());
            count--;
            if ( count == 0 ) {
                output.output("Maximum name clashes reached, no more checks will be made.\n");
                break;
            }
        }
    }

#if 1
    // Check to see if all the port names are valid
    count = 10;
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
          iter != comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);

//        bool found = false;
        for ( unsigned int i = 0; i < ccomp->links.size(); i++ ) {
            for ( int j = 0; j < 2; j++ ) {
                const ConfigLink& link = links[ccomp->links[i]];
                if ( link.component[j] == ccomp->id ) {
                    // If port is not found, print a warning
                    if (!Component::isPortValidForComponent(ccomp->type, link.port[j]) ) {
                        // For now this is not a fatal error
                        // found_error = true;
                        output.output("WARNING:  Attempling to connect to undocumented port: %s, "
                                      "in component %s of type %s.  The most likely "
                                      "outcome is a segfault.\n", link.port[j].c_str(),
                                      ccomp->name.c_str(), ccomp->type.c_str());
                        count--;
                    }
                }
            }
        }
        if ( count <= 0 ) {
            output.output("Maximum bad port names reached, no more checks will be made.\n");
            break;
        }

        // if ( name_set.find(ccomp->name) == name_set.end() ) {
        //     name_set.insert(ccomp->name);
        // }
        // else {
        //     found_error = true;
        //     output.output("Found duplicate component nane: %s\n",ccomp->name.c_str());
        //     count--;
        //     if ( count == 0 ) {
        //         output.output("Maximum name clashes reached, no more checks will be made.\n");
        //         break;
        //     }
        // }
    }
#endif
    
    return found_error;
}


ComponentId_t
ConfigGraph::addComponent(std::string name, std::string type, float weight, RankInfo rank)
{
	comps.push_back(ConfigComponent(nextCompID, name, type, weight, rank, false));
    return nextCompID++;
}

ComponentId_t
ConfigGraph::addComponent(std::string name, std::string type)
{
	comps.push_back(ConfigComponent(nextCompID, name, type, 1.0f, RankInfo(), false));
    return nextCompID++;
}

void
ConfigGraph::setComponentRank(ComponentId_t comp_id, RankInfo rank)
{
    comps[comp_id].rank = rank;
}

void
ConfigGraph::setComponentWeight(ComponentId_t comp_id, float weight)
{
	comps[comp_id].weight = weight;
}

void
ConfigGraph::addParams(ComponentId_t comp_id, Params& p)
{
    bool bk = comps[comp_id].params.enableVerify(false);
    // comps[comp_id].params.insert(p.begin(),p.end());
    comps[comp_id].params.insert(p);
    comps[comp_id].params.enableVerify(bk);
}

void
ConfigGraph::addParameter(ComponentId_t comp_id, const string key, const string value, bool overwrite)
{
    bool bk = comps[comp_id].params.enableVerify(false);
	// if ( overwrite ) {
	// 	comps[comp_id].params[key] = value;
	// }
	// else {
	// 	comps[comp_id].params.insert(pair<string,string>(key,value));
	// }
    comps[comp_id].params.insert(key,value,overwrite);
    comps[comp_id].params.enableVerify(bk);
}

void 
ConfigGraph::setStatisticOutput(const char* name)
{
    statOutputName = name;
}

void
ConfigGraph::setStatisticOutputParams(const Params& p)
{
    statOutputParams = p;
}

void 
ConfigGraph::addStatisticOutputParameter(const char* param, const char* value)
{
    // statOutputParams[param] = value;
    statOutputParams.insert(std::string(param), std::string(value));
}

void 
ConfigGraph::setStatisticLoadLevel(uint8_t loadLevel)
{
    statLoadLevel = loadLevel;
}

void 
ConfigGraph::enableComponentStatistic(ComponentId_t comp_id, string statisticName)
{
    // NOTE: For every statistic in the enabledStatistics List, there must be
    //       a coresponding params entry in enabledStatParams list.  The two
    //       lists will always be the same size.  
    
    // Check for Enable All Statistics
    if (statisticName == STATALLFLAG) {
        // Force the STATALLFLAG to always be on the bottom of the list.
        // First check to see if anything is in the vector, if vector is empty, 
        // a STATALLFLAG flag will be added to the vector
        if (false == comps[comp_id].enabledStatistics.empty()) {
            // The vector is populated, so see if the STATALLFLAG 
            // already exists if it does, we are done
            if (STATALLFLAG != comps[comp_id].enabledStatistics.back()) {
                // Add a STATALLFLAG to end of the vector
                comps[comp_id].enabledStatistics.push_back(STATALLFLAG);
                comps[comp_id].enabledStatParams.push_back(Params());
            }
        } else {
            // Add a STATALLFLAG to end of the vector
            comps[comp_id].enabledStatistics.push_back(STATALLFLAG);
            comps[comp_id].enabledStatParams.push_back(Params());
        }
    } else {
        // Check to see if the stat is already in the list
        for (std::vector<std::string>::iterator iter = comps[comp_id].enabledStatistics.begin(); iter != comps[comp_id].enabledStatistics.end(); ++iter) {
            if (statisticName == *iter) {
                // We found the name already in the enabledStatistics list, do nothing
                return;                
            }
        }
        
        // statisticName not in list, so add statistic and params to top of the vectors
        comps[comp_id].enabledStatistics.insert(comps[comp_id].enabledStatistics.begin(), statisticName);
        comps[comp_id].enabledStatParams.insert(comps[comp_id].enabledStatParams.begin(), Params());
    }
}

void 
ConfigGraph::enableStatisticForComponentName(string ComponentName, string statisticName)
{
    bool found;
    
    // Search all the components for a matching name
    for (ConfigComponentMap_t::iterator iter = comps.begin(); iter != comps.end(); ++iter) {
        // Check to see if the names match or All components are selected
        found = ((ComponentName == iter->name) || (ComponentName == STATALLFLAG));
        if (true == found) {
            enableComponentStatistic(iter->id, statisticName);
        }
    }
}

void 
ConfigGraph::enableStatisticForComponentType(string ComponentType, string statisticName)
{
    bool found;
    
    // Search all the components for a matching type
    for (ConfigComponentMap_t::iterator iter = comps.begin(); iter != comps.end(); ++iter) {
        // Check to see if the types match or All components are selected
        found = ((ComponentType == iter->type) || (ComponentType == STATALLFLAG));
        if (true == found) {
            enableComponentStatistic(iter->id, statisticName);
        }
    }
}

void 
ConfigGraph::addComponentStatisticParameter(ComponentId_t comp_id, string statisticName, const char* param, const char* value)
{
    // NOTE: For every statistic in the enabledStatistics List, there must be
    //       a coresponding params entry in enabledStatParams list.  The two
    //       lists will always be the same size.  
    
    // Scan the enabledStatistics list for the statistic name
    for (size_t x = 0; x < comps[comp_id].enabledStatistics.size(); x++) {
        // Check to see if the names match.  NOTE this also works for the STATALLFLAG
        if (statisticName == comps[comp_id].enabledStatistics.at(x)) {
            // Add/set the parameter
            // comps[comp_id].enabledStatParams.at(x)[param] = value;
            comps[comp_id].enabledStatParams.at(x).insert(std::string(param),std::string(value));
        }
    }
}

void 
ConfigGraph::addStatisticParameterForComponentName(string ComponentName, string statisticName, const char* param, const char* value)
{
    bool found;
    
    // Search all the components for a matching name
    for (ConfigComponentMap_t::iterator iter = comps.begin(); iter != comps.end(); ++iter) {
        // Check to see if the names match or All components are selected
        found = ((ComponentName == iter->name) || (ComponentName == STATALLFLAG));
        if (true == found) {
            addComponentStatisticParameter(iter->id, statisticName, param, value);
        }
    }
}

void 
ConfigGraph::addStatisticParameterForComponentType(string ComponentType, string statisticName, const char* param, const char* value)
{
    bool found;
    
    // Search all the components for a matching type
    for (ConfigComponentMap_t::iterator iter = comps.begin(); iter != comps.end(); ++iter) {
        // Check to see if the types match or All components are selected
        found = ((ComponentType == iter->type) || (ComponentType == STATALLFLAG));
        if (true == found) {
            addComponentStatisticParameter(iter->id, statisticName, param, value);
        }
    }
}

void
ConfigGraph::addLink(ComponentId_t comp_id, string link_name, string port, string latency_str, bool no_cut)
{
	if ( link_names.find(link_name) == link_names.end() ) {
        LinkId_t id = links.size();
        link_names[link_name] = id;
        links.insert(ConfigLink(id, link_name));
    }
	ConfigLink &link = links[link_names[link_name]];
    if ( link.current_ref >= 2 ) {
        cout << "ERROR: Parsing SDL file: Link " << link_name << " referenced more than two times" << endl;
        exit(1);
    }

	// Convert the latency string to a number

	int index = link.current_ref++;
	link.component[index] = comp_id;
	link.port[index] = port;
    link.latency_str[index] = latency_str;
    link.no_cut = link.no_cut | no_cut;
    
	comps[comp_id].links.push_back(link.id);
}
ComponentId_t
ConfigGraph::addIntrospector(string name, string type)
{
	comps.push_back(ConfigComponent(nextCompID, name, type, 0.0f, RankInfo(0, 0), true));
    return nextCompID++;

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
            for ( LinkIdMap_t::const_iterator link_it = comp.links.begin();
                  link_it != comp.links.end(); ++link_it ) {
                const ConfigLink& link = links[*link_it];
                ComponentId_t remote = link.component[0] == comp.id ?
                    link.component[1] : link.component[0];
                if ( rank_set.find(comps[remote].rank.rank) != rank_set.end() ) {
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

        const ConfigComponent& comp0 = comps[link.component[0]];
        const ConfigComponent& comp1 = comps[link.component[1]];

        bool comp0_in_ranks = (rank_set.find(comp0.rank.rank) != rank_set.end());
        bool comp1_in_ranks = (rank_set.find(comp1.rank.rank) != rank_set.end());
        
        if ( comp0_in_ranks || comp1_in_ranks ) {
            // Clone the link and add to new lin k map
            graph->links.insert(ConfigLink(link));  // Will make a copy into map

            graph->comps[comp0.id].links.push_back(link.id);
            graph->comps[comp1.id].links.push_back(link.id);
        }
    }

    // Copy the statistic configuration to the sub-graph
    graph->setStatisticOutput(this->statOutputName.c_str());
    graph->setStatisticOutputParams(this->getStatOutputParams());
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
        
        const ConfigComponent& comp0 = comps[link.component[0]];
        const ConfigComponent& comp1 = comps[link.component[1]];

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
        // by seeing if the first item in the connected componets is
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
                for ( LinkIdMap_t::const_iterator link_it = comp.links.begin();
                      link_it != comp.links.end(); ++link_it ) {

                    const ConfigLink& link = links[*link_it];
                    
                    if ( !group.contains(link.component[0]) || !group.contains(link.component[1] ) ) {
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
    // chsnge ID to point to super group.
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
            comps[*c_iter].rank = comp.rank;
        }
    }
}

void
ConfigGraph::getConnectedNoCutComps(ComponentId_t start, ComponentIdMap_t& group)
{
    // We'll do this as a simple recursive depth first search
    group.insert(start);
    
    // First, get the component
    ConfigComponent& comp = comps[start];

    for ( vector<LinkId_t>::iterator it = comp.links.begin(); it != comp.links.end(); ++it ) {
        ConfigLink& link = links[*it];

        // If this is a no_cut link, need to follow it to next
        // component if next component is not already in group
        if ( link.no_cut ) {
            ComponentId_t id = (link.component[0] == start ? link.component[1] : link.component[0]);
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

