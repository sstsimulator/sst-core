// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization/core.h"

#include <boost/mpi.hpp>
#include <boost/format.hpp>

#include <sst/core/simulation.h>
#include <sst/core/configGraph.h>
#include <sst/core/debug.h>
#include <sst/core/timeLord.h>

namespace mpi = boost::mpi;

using namespace std;

namespace SST {

#define _GRAPH_DBG( fmt, args...) __DBG( DBG_GRAPH, Graph, fmt, ## args )

int Vertex::count = 0;
int Edge::count = 0;

ComponentId_t ConfigComponent::count = 0;
LinkId_t ConfigLink::count = 0;

static inline int min( int x, int y ) { return x < y ? x : y; }

//void ConfigComponent::print_component(std::ostream &os) const {  // Renamed per Issue 70 - ALevine
void ConfigComponent::print(std::ostream &os) const {
    os << "Component " << name << " (id = " << id << ")" << std::endl;
    os << "  type = " << type << std::endl;
    os << "  weight = " << weight << std::endl;
    os << "  rank = " << rank << std::endl;
    os << "  isIntrospector = " << isIntrospector << std::endl;
    os << "  Links:" << std::endl;
    for (size_t i = 0 ; i != links.size() ; ++i) {
//	links[i]->print_link(os);  // Renamed per Issue 70 - ALevine
	links[i]->print(os);
    }
    
    os << "  Params:" << std::endl;
    params.print_all_params(os);
    
}

void
ConfigGraph::setComponentRanks(int rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
                            iter != comps.end(); ++iter )
    {
	(*iter).second->rank = rank;
    }
    
}

bool
ConfigGraph::containsComponentInRank(int rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
                            iter != comps.end(); ++iter )
    {
	if ( (*iter).second->rank == rank) return true;
    }
    return false;
    
}

bool
ConfigGraph::checkRanks(int ranks)
{
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
                            iter != comps.end(); ++iter )
    {
	int rank = (*iter).second->rank;
	if ( rank < 0 || rank >= ranks ) return false;
    }
    return true;
    
}



ComponentId_t
ConfigGraph::addComponent(string name, string type, float weight, int rank)
{
    ConfigComponent* comp = new ConfigComponent();
    comp->name = name;
    comp->type = type;
    comp->weight = weight;
    comp->rank = rank;

    comps[comp->id] = comp;
    return comp->id;
}

ComponentId_t
ConfigGraph::addComponent(string name, string type)
{
    ConfigComponent* comp = new ConfigComponent();
    comp->name = name;
    comp->type = type;

    comps[comp->id] = comp;
    return comp->id;
}

void
ConfigGraph::setComponentRank(ComponentId_t comp_id, int rank)
{
    if ( comps.find(comp_id) == comps.end() ) {
	cout << "Invalid component id: " << comp_id << " in call to ConfigGraph::setComponentRank, aborting..." << endl;
	abort();
    }
    comps[comp_id]->rank = rank;
}
    
void
ConfigGraph::setComponentWeight(ComponentId_t comp_id, float weight)
{
    if ( comps.find(comp_id) == comps.end() ) {
	cout << "Invalid component id: " << comp_id << " in call to ConfigGraph::setComponentRank, aborting..." << endl;
	abort();
    }
    comps[comp_id]->weight = weight;
}

void
ConfigGraph::addParams(ComponentId_t comp_id, Params& p)
{
    if ( comps.find(comp_id) == comps.end() ) {
	cout << "Invalid component id: " << comp_id << " in call to ConfigGraph::addParams, aborting..." << endl;
	abort();
    }

    comps[comp_id]->params.insert(p.begin(),p.end());
}
    
void
ConfigGraph::addParameter(ComponentId_t comp_id, const string key, const string value, bool overwrite)
{
    if ( comps.find(comp_id) == comps.end() ) {
	cout << "Invalid component id: " << comp_id << " in call to ConfigGraph::addParameter, aborting..." << endl;
	abort();
    }

    if ( overwrite ) {
	comps[comp_id]->params[key] = value;
    }
    else {
	comps[comp_id]->params.insert(pair<string,string>(key,value));
    }
}

void
ConfigGraph::addLink(ComponentId_t comp_id, string link_name, string port, string latency_str)
{
    if ( comps.find(comp_id) == comps.end() ) {
	cout << "Invalid component id: " << comp_id << " in call to ConfigGraph::addLink, aborting..." << endl;
	abort();
    }

    ConfigLink* link;
    if ( links.find(link_name) == links.end() ) {
	link = new ConfigLink();
	link->name = link_name;
	links[link_name] = link;
    }
    else {
	link = links[link_name];
	if ( link->current_ref >= 2 ) {
	    cout << "ERROR: Parsing SDL file: Link " << link_name << " referenced more than two times" << endl;
	    exit(1);
	}	
    }

    // Convert the latency string to a number
    SimTime_t latency = Simulation::getSimulation()->getTimeLord()->getSimCycles(latency_str, "ConfigGraph::addLink");
    
    int index = link->current_ref++;
    link->component[index] = comp_id;
    link->port[index] = port;
    link->latency[index] = latency;

    comps[comp_id]->links.push_back(link);
}
    
ComponentId_t
ConfigGraph::addIntrospector(string name, string type)
{
    ConfigComponent* comp = new ConfigComponent();
    comp->name = name;
    comp->type = type;
    comp->isIntrospector = true;

    comps[comp->id] = comp;
    return comp->id;    
}
    
} // namespace SST
