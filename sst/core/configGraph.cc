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
#include "sst/core/serialization.h"
#include <sst/core/configGraph.h>

#include <boost/format.hpp>

//#include <sst/core/debug.h>
#include <sst/core/timeLord.h>
#include <sst/core/simulation.h>

#include <string.h>

using namespace std;

namespace SST {

#define _GRAPH_DBG( fmt, args...) __DBG( DBG_GRAPH, Graph, fmt, ## args )

int Vertex::count = 0;
int Edge::count = 0;

ComponentId_t ConfigComponent::count = 0;
LinkId_t ConfigLink::count = 0;

static inline int min( int x, int y ) { return x < y ? x : y; }

void ConfigComponent::print(std::ostream &os) const {
    os << "Component " << name << " (id = " << id << ")" << std::endl;
    os << "  type = " << type << std::endl;
    os << "  weight = " << weight << std::endl;
    os << "  rank = " << rank << std::endl;
    os << "  isIntrospector = " << isIntrospector << std::endl;
    os << "  Links:" << std::endl;
    for (size_t i = 0 ; i != links.size() ; ++i) {
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
	ConfigLink* clink = (*iter).second;
	// This one should never happen since the slots are
	// initialized in order, but just in case...
	if ( clink->component[0] == ULONG_MAX ) {
	    printf("Found dangling link: %s.  It is connected on one side to component %s.\n",clink->name.c_str(),
		   comps[clink->component[1]]->name.c_str());
	    found_error = true;
	}
	if ( clink->component[1] == ULONG_MAX ) {
	    printf("Found dangling link: %s.  It is connected on one side to component %s.\n",clink->name.c_str(),
		   comps[clink->component[0]]->name.c_str());
	    found_error = true;
	}
    }
    return found_error;
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

void ConfigGraph::dumpToFile(const std::string filePath, Config* cfg) {
    FILE* dumpFile = fopen(filePath.c_str(), "wt");
    assert(dumpFile);

    ConfigComponentMap_t::iterator comp_itr;
    std::map<string, string>::iterator param_itr;

    fprintf(dumpFile, "# Automatically generated SST Python input\n");
    fprintf(dumpFile, "import sst\n\n");
    fprintf(dumpFile, "# Define SST core options\n");
    fprintf(dumpFile, "sst.setProgramOption(\"timebase\", \"%s\")\n", cfg->timeBase.c_str());
    fprintf(dumpFile, "sst.setProgramOption(\"stop-at\", \"%s\")\n\n", cfg->stopAtCycle.c_str());
    fprintf(dumpFile, "# Define the simulation components\n");
    for(comp_itr = comps.begin(); comp_itr != comps.end(); comp_itr++) {
	ConfigComponent* the_comp = comp_itr->second;

	fprintf(dumpFile, "%s = sst.Component(\"%s\", \"%s\")\n",
		makeNamePythonSafe(the_comp->name).c_str(),
		makeNamePythonSafe(the_comp->name).c_str(),
		the_comp->type.c_str());

	param_itr = the_comp->params.begin();

	if(param_itr != the_comp->params.end()) {
		fprintf(dumpFile, "%s.addParams({\n", makeNamePythonSafe(the_comp->name).c_str());
		fprintf(dumpFile, "      \"%s\" : \"\"\"%s\"\"\"", param_itr->first.c_str(), param_itr->second.c_str());
		param_itr++;

		for(; param_itr != the_comp->params.end(); param_itr++) {
			fprintf(dumpFile, ",\n      \"%s\" : \"\"\"%s\"\"\"",
				param_itr->first.c_str(), param_itr->second.c_str());
		}

		fprintf(dumpFile, "\n})\n");
	}
    }

    fprintf(dumpFile, "\n\n# Define the simulation links\n");

    ConfigLinkMap_t::iterator link_itr;
    for(link_itr = links.begin(); link_itr != links.end(); link_itr++) {
	ConfigComponent* link_left = comps[link_itr->second->component[0]];
	ConfigComponent* link_right = comps[link_itr->second->component[1]];

	fprintf(dumpFile, "%s = sst.Link(\"%s\")\n",
		makeNamePythonSafe(link_itr->second->name).c_str(), makeNamePythonSafe(link_itr->second->name).c_str());
	fprintf(dumpFile, "%s.connect( (%s, \"%s\", \"%" PRIu64 "ps\"), (%s, \"%s\", \"%" PRIu64 "ps\") )\n",
		makeNamePythonSafe(link_itr->second->name).c_str(),
		makeNamePythonSafe(link_left->name).c_str(),
		makeNamePythonSafe(link_itr->second->port[0]).c_str(),
		*link_itr->second->latency,
		makeNamePythonSafe(link_right->name).c_str(),
		makeNamePythonSafe(link_itr->second->port[1]).c_str(),
		*link_itr->second->latency );
    }

    fprintf(dumpFile, "# End of generated output.\n");
    fclose(dumpFile);
}

std::string ConfigGraph::makeNamePythonSafe(const std::string name) {
	const uint32_t name_length = (uint32_t) name.size();
	char* safe_name = (char*) malloc(sizeof(char) * (name_length + 1));
	strcpy(safe_name, name.c_str());

	for(uint32_t i = 0; i < name_length; ++i) {
		switch(safe_name[i]) {
		case '.':
			safe_name[i] = '_';
			break;
		case ':':
			safe_name[i] = '_';
			break;
		case ',':
			safe_name[i] = '_';
			break;
		case '-':
			safe_name[i] = '_';
			break;
		}
	}

	if(name_length > 0 && isdigit(safe_name[0])) {
		std::string safe_name_str = safe_name;
		std::string safe_name_prefix = "s_";
		return safe_name_prefix + safe_name_str;
	} else {
		std::string safe_name_str = safe_name;
		return safe_name_str;
	}
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
