// -*- c++ -*-

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

#ifndef SST_CORE_CONFIGGRAPH_H
#define SST_CORE_CONFIGGRAPH_H

#include "sst/core/sst_types.h"
#include <sst/core/serialization.h>

#include <vector>
#include <map>

#include "sst/core/graph.h"
#include "sst/core/params.h"
//#include "sst/core/sdl.h"


namespace SST {

class Simulation;

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
    
    void print(std::ostream &os) const;
    
private:

    friend class ConfigGraph;
    
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
    
    ConfigLink()
    {
	id = count++;
	current_ref = 0;

	// Initialize the component data items
	component[0] = ULONG_MAX;
	component[1] = ULONG_MAX;
    }

    SimTime_t getMinLatency() {
	if ( latency[0] < latency[1] ) return latency[0];
	return latency[1];
    }
    
    void print(std::ostream &os) const {
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
    void print(std::ostream &os) const {
	os << "Printing graph" << std::endl;
        for (ConfigComponentMap_t::const_iterator i = comps.begin() ; i != comps.end() ; ++i) {
	    (*i).second->print(os);
	}
    }

    // Helper function to set all the ranks to the same value
    void setComponentRanks(int rank);
    bool containsComponentInRank(int rank);
    bool checkRanks(int ranks);
    
    // ConfigComponent* addComponent(std::string name, std::string type, float weight, int rank);
    // ConfigComponent* addComponent(std::string name, std::string type);
    
    // ConfigComponent* addIntrospector(std::string name, std::string type);

    // API for programatic initialization
    ComponentId_t addComponent(std::string name, std::string type, float weight, int rank);
    ComponentId_t addComponent(std::string name, std::string type);

    void setComponentRank(ComponentId_t comp_id, int rank);
    void setComponentWeight(ComponentId_t comp_id, float weight);

    void addParams(ComponentId_t comp_id, Params& p); 
    void addParameter(ComponentId_t comp_id, std::string key, std::string value, bool overwrite = false);

    void addLink(ComponentId_t comp_id, std::string link_name, std::string port, std::string latency_str);
    
    ComponentId_t addIntrospector(std::string name, std::string type);

    bool checkForStructuralErrors();
    
    
    // Temporary until we have a better API
    ConfigComponentMap_t& getComponentMap() {
	return comps;
    }
    ConfigLinkMap_t& getLinkMap() {
	return links;
    }
    
private:
    friend class Simulation;
    friend class sdl_parser;
    friend class SSTSDLModelDefinition;
    
    ConfigLinkMap_t      links;
    ConfigComponentMap_t comps;


    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
	ar & BOOST_SERIALIZATION_NVP(links);
	ar & BOOST_SERIALIZATION_NVP(comps);
    }
    
    
};

 
} // namespace SST

#endif // SST_CORE_CONFIGGRAPH_H
