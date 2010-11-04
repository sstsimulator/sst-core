// -*- c++ -*-

// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CONFIGGRAPH_H
#define SST_CONFIGGRAPH_H

#include <vector>
#include <map>

#include "sst/core/graph.h"
#include "sst/core/sdl.h"

#include "sst/core/sst_types.h"
#include "sst/core/params.h"

namespace SST {

class Simulation;

extern void makeGraph(Simulation *sim, SDL_CompMap_t& map, Graph& graph );
extern int findMinPart(Graph &graph);

class ConfigLink;
    
class ConfigComponent {
public:
    ComponentId_t            id;
    std::string              name;
    std::string              type;
    float                    weight;
    int                      rank;
    std::vector<ConfigLink*> links;
    Params                   params;
    bool                     isIntrospector;

    ConfigComponent(std::string name, std::string type, float weight, int rank, bool isIntrospector) :
	name(name),
	type(type),
	weight(weight),
	rank(rank),
	isIntrospector(isIntrospector)
    {
	id = count++;
    }

    ConfigComponent() {
	id = count++;
    }
    
private:
    static ComponentId_t count;

};
    
class ConfigLink {
public:
    LinkId_t         id;
    std::string      name;
    ConfigComponent* component[2];
    std::string      port[2];
    std::string      latency[2];
//     ConfigComponent* left_component;
//     std::string      left_port;
//     std::string      left_latency;
//     ConfigComponent* right_component;
//     std::string      right_port;
//     std::string      right_latency;

    ConfigLink() {
	id = count++;
    }
    
private:
    static LinkId_t count;
};

typedef std::map<std::string,ConfigLink*> ConfigLinkMap_t;
typedef std::map<ComponentId_t,ConfigComponent*> ConfigComponentMap_t;
typedef std::map<std::string,Params*> ParamsMap_t;
typedef std::map<std::string,std::string> VariableMap_t;

    
class ConfigGraph {
public:
    ConfigLinkMap_t      links;
    ConfigComponentMap_t comps;

    ParamsMap_t          includes;
    VariableMap_t        variables;
};

 
} // namespace SST

#endif // SST_CONFIGGRAPH_H
