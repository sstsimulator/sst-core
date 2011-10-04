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

    ConfigComponent() :
        isIntrospector(false)
    {
	id = count++;
    }
    
    void print_component(std::ostream &os) const;

private:
    static ComponentId_t count;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
	ar & BOOST_SERIALIZATION_NVP(id);
	ar & BOOST_SERIALIZATION_NVP(name);
	ar & BOOST_SERIALIZATION_NVP(type);
	ar & BOOST_SERIALIZATION_NVP(weight);
	ar & BOOST_SERIALIZATION_NVP(rank);
	ar & BOOST_SERIALIZATION_NVP(links);
	ar & BOOST_SERIALIZATION_NVP(params);
	ar & BOOST_SERIALIZATION_NVP(isIntrospector);
    }
    
};

class ConfigLink {
public:
    LinkId_t         id;
    std::string      name;
    // ConfigComponent* component[2];
    ComponentId_t    component[2];
    std::string      port[2];
    SimTime_t        latency[2];
    int              current_ref;
    
    ConfigLink() {
	id = count++;
	current_ref = 0;
    }

    SimTime_t getMinLatency() {
	if ( latency[0] < latency[1] ) return latency[0];
	return latency[1];
    }
    
    void print_link(std::ostream &os) const {
	os << "Link " << name << " (id = " << id << ")" << std::endl;
	os << "  component[0] = " << component[0] << std::endl;
	os << "  port[0] = " << port[0] << std::endl;
	os << "  latency[0] = " << latency[0] << std::endl;
	os << "  component[1] = " << component[1] << std::endl;
	os << "  port[1] = " << port[1] << std::endl;
	os << "  latency[1] = " << latency[1] << std::endl;
    }
    


private:
    static LinkId_t count;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
	ar & BOOST_SERIALIZATION_NVP(id);
	ar & BOOST_SERIALIZATION_NVP(name);
	ar & BOOST_SERIALIZATION_NVP(component);
	ar & BOOST_SERIALIZATION_NVP(port);
	ar & BOOST_SERIALIZATION_NVP(latency);
	ar & BOOST_SERIALIZATION_NVP(current_ref);
    }


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

    void print_graph(std::ostream &os) const {
	os << "Printing graph" << std::endl;
        for (ConfigComponentMap_t::const_iterator i = comps.begin() ; i != comps.end() ; ++i) {
	    (*i).second->print_component(os);
	}
    }

private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
	ar & BOOST_SERIALIZATION_NVP(links);
	ar & BOOST_SERIALIZATION_NVP(comps);
	ar & BOOST_SERIALIZATION_NVP(includes);
	ar & BOOST_SERIALIZATION_NVP(variables);
    }
    
    
};

 
} // namespace SST

#endif // SST_CONFIGGRAPH_H
