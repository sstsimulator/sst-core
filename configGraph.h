// -*- c++ -*-

// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
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
#include <set>

#include "sst/core/sparseVectorMap.h"
#include "sst/core/params.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/rankInfo.h"

// #include "sst/core/simulation.h"

using namespace SST::Statistics;

namespace SST {


class Simulation;

class Config;
class TimeLord;

typedef SparseVectorMap<ComponentId_t> ComponentIdMap_t;
typedef std::vector<LinkId_t> LinkIdMap_t;



/** Represents the configuration of a generic Link */
class ConfigLink {
public:
    LinkId_t         id;            /*!< ID of this link */
    std::string      name;          /*!< Name of this link */
    ComponentId_t    component[2];  /*!< IDs of the connected components */
    std::string      port[2];       /*!< Names of the connected ports */
    SimTime_t        latency[2];    /*!< Latency from each side */
    std::string      latency_str[2];/*!< Temp string holding latency */
    int              current_ref;   /*!< Number of components currently referring to this Link */
    bool             no_cut;        /*!< If set to true, partitioner will not make a cut through this Link */

    // inline const std::string& key() const { return name; }
    inline LinkId_t key() const { return id; }
    
    /** Return the minimum latency of this link (from both sides) */
    SimTime_t getMinLatency() const {
        if ( latency[0] < latency[1] ) return latency[0];
        return latency[1];
    }

    /** Print the Link information */
    void print(std::ostream &os) const {
        os << "Link " << name << " (id = " << id << ")" << std::endl;
        os << "  component[0] = " << component[0] << std::endl;
        os << "  port[0] = " << port[0] << std::endl;
        os << "  latency[0] = " << latency[0] << std::endl;
        os << "  component[1] = " << component[1] << std::endl;
        os << "  port[1] = " << port[1] << std::endl;
        os << "  latency[1] = " << latency[1] << std::endl;
    }

    /* Do not use.  For serialization only */
    ConfigLink() {}
private:
    friend class ConfigGraph;
    ConfigLink(LinkId_t id) :
        id(id),
        no_cut(false)
    {
        current_ref = 0;

        // Initialize the component data items
        component[0] = ULONG_MAX;
        component[1] = ULONG_MAX;
    }

    ConfigLink(LinkId_t id, const std::string &n) :
        id(id),
        no_cut(false)
    {
        current_ref = 0;
        name = n;

        // Initialize the component data items
        component[0] = ULONG_MAX;
        component[1] = ULONG_MAX;
    }

    void updateLatencies(TimeLord*);


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

typedef SparseVectorMap<LinkId_t,ConfigLink> ConfigLinkMap_t;

/** Represents the configuration of a generic component */
class ConfigComponent {
public:
    ComponentId_t                 id;                /*!< Unique ID of this component */
    std::string                   name;              /*!< Name of this component */
    std::string                   type;              /*!< Type of this component */
    float                         weight;            /*!< Parititoning weight for this component */
    RankInfo                      rank;              /*!< Parallel Rank for this component */
    std::vector<LinkId_t>         links;             /*!< List of links connected */
    Params                        params;            /*!< Set of Parameters */
    bool                          isIntrospector;    /*!< Is this an Introspector? */
    std::vector<std::string>      enabledStatistics; /*!< List of statistics to be enabled */
    std::vector<Params>           enabledStatParams; /*!< List of parameters for enabled statistics */

    inline const ComponentId_t& key()const { return id; }
    
    /** Print Component information */
    void print(std::ostream &os) const;

    ConfigComponent cloneWithoutLinks() const;
    ConfigComponent cloneWithoutLinksOrParams() const;
    
    ConfigComponent() {}
    ~ConfigComponent() {}
private:

    friend class ConfigGraph;
    /** Create a new Component */
    ConfigComponent(ComponentId_t id, std::string name, std::string type, float weight, RankInfo rank, bool isIntrospector) :
        id(id),
        name(name),
        type(type),
        weight(weight),
        rank(rank),
        isIntrospector(isIntrospector)
    { }

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
        ar & BOOST_SERIALIZATION_NVP(enabledStatistics);
        ar & BOOST_SERIALIZATION_NVP(enabledStatParams);
    }

};


/** Map names to Links */
// typedef std::map<std::string,ConfigLink> ConfigLinkMap_t;
// typedef SparseVectorMap<std::string,ConfigLink> ConfigLinkMap_t;
/** Map IDs to Components */
typedef SparseVectorMap<ComponentId_t,ConfigComponent> ConfigComponentMap_t;
/** Map names to Parameter Sets: XML only */
typedef std::map<std::string,Params*> ParamsMap_t;
/** Map names to variable values:  XML only */
typedef std::map<std::string,std::string> VariableMap_t;

class PartitionGraph;
    
/** A Configuration Graph
 *  A graph representing Components and Links
 */
class ConfigGraph {
public:
    /** Print the configuration graph */
	void print(std::ostream &os) const {
		os << "Printing graph" << std::endl;
		for (ConfigComponentMap_t::const_iterator i = comps.begin() ; i != comps.end() ; ++i) {
			i->print(os);
		}
	}

    ConfigGraph() {
        links.clear();
        comps.clear();
        nextCompID = 0;
        // Init the statistic output settings
        statOutputName = STATISTICSDEFAULTOUTPUTNAME;
        statLoadLevel = STATISTICSDEFAULTLOADLEVEL;
    }

    size_t getNumComponents() { return comps.data.size(); }
    
    /** Helper function to set all the ranks to the same value */
    void setComponentRanks(RankInfo rank);
    /** Checks to see if rank contains at least one component */
    bool containsComponentInRank(RankInfo rank);
    /** Verify that all components have valid Ranks assigned */
    bool checkRanks(RankInfo ranks);


    // API for programatic initialization
    /** Create a new component with weight and rank */
    ComponentId_t addComponent(std::string name, std::string type, float weight, RankInfo rank);
    /** Create a new component */
    ComponentId_t addComponent(std::string name, std::string type);

    /** Set on which rank a Component will exist (partitioning) */
    void setComponentRank(ComponentId_t comp_id, RankInfo rank);
    /** Set the weight of a Component (partitioning) */
    void setComponentWeight(ComponentId_t comp_id, float weight);

    /** Add a set of Parameters to a component */
    void addParams(ComponentId_t comp_id, Params& p);
    /** Add a single parameter to a component */
    void addParameter(ComponentId_t comp_id, std::string key, std::string value, bool overwrite = false);

    /** Set the statistic ouput module */
    void setStatisticOutput(const char* name);
    
    /** Add parameter to the statistic output module */
    void addStatisticOutputParameter(const char* param, const char* value);

    /** Set a set of parameter to the statistic output module */
    void setStatisticOutputParams(const Params& p);

    /** Set the statistic system load level */
    void setStatisticLoadLevel(uint8_t loadLevel);

    /** Enable a Statistics assigned to a component */
    void enableComponentStatistic(ComponentId_t comp_id, std::string statisticName);
    void enableStatisticForComponentName(std::string ComponentName, std::string statisticName);
    void enableStatisticForComponentType(std::string ComponentType, std::string statisticName);

    /** Add Parameters for a Statistic */
    void addComponentStatisticParameter(ComponentId_t comp_id, std::string statisticName, const char* param, const char* value);
    void addStatisticParameterForComponentName(std::string ComponentName, std::string statisticName, const char* param, const char* value);
    void addStatisticParameterForComponentType(std::string ComponentType, std::string statisticName, const char* param, const char* value);
    
    const std::string& getStatOutput() const {return statOutputName;}
    const Params&      getStatOutputParams() const {return statOutputParams;}
    long               getStatLoadLevel() const {return statLoadLevel;}
    
    /** Add a Link to a Component on a given Port */
    void addLink(ComponentId_t comp_id, std::string link_name, std::string port, std::string latency_str, bool no_cut = false);

    /** Create a new Introspector */
    ComponentId_t addIntrospector(std::string name, std::string type);

    /** Perform any post-creation cleanup processes */
    void postCreationCleanup();

    /** Check the graph for Structural errors */
    bool checkForStructuralErrors();

    // Temporary until we have a better API
    /** Return the map of components */
    ConfigComponentMap_t& getComponentMap() {
        return comps;
    }
    /** Return the map of links */
    ConfigLinkMap_t& getLinkMap() {
        return links;
    }

    ConfigGraph* getSubGraph(uint32_t start_rank, uint32_t end_rank);
    ConfigGraph* getSubGraph(const std::set<uint32_t>& rank_set);

    PartitionGraph* getPartitionGraph();
    PartitionGraph* getCollapsedPartitionGraph();
    void annotateRanks(PartitionGraph* graph);
    void getConnectedNoCutComps(ComponentId_t start, ComponentIdMap_t& group);
    
private:
    friend class Simulation;
    friend class SSTSDLModelDefinition;

    ConfigLinkMap_t      links;
    ConfigComponentMap_t comps;

    // temporary as a test
    std::map<std::string,LinkId_t> link_names;
    
    ComponentId_t  nextCompID;
    
    std::string statOutputName;
    Params      statOutputParams;
    uint8_t     statLoadLevel;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
	{
        // std::cout <<Simulation::getSimulation()->getRank() << ": serializing links" << std::endl;
		ar & BOOST_SERIALIZATION_NVP(links);
        // std::cout << "serializing comps" << std::endl;
		ar & BOOST_SERIALIZATION_NVP(comps);
        // std::cout << "done serializing" << std::endl;
		ar & BOOST_SERIALIZATION_NVP(statOutputName);
		ar & BOOST_SERIALIZATION_NVP(statOutputParams);
		ar & BOOST_SERIALIZATION_NVP(statLoadLevel);
	}


};

    
class PartitionComponent {
public:
    ComponentId_t             id;
    float                     weight;
    RankInfo                  rank;
    LinkIdMap_t               links;

    ComponentIdMap_t          group;

    PartitionComponent(const ConfigComponent& cc) {
        id = cc.id;
        weight = cc.weight;
        rank = cc.rank;
    }

    PartitionComponent(LinkId_t id) :
        id(id),
        weight(0),
        rank(RankInfo(RankInfo::UNASSIGNED, 0))
    {}
    
    // PartitionComponent(ComponentId_t id, ConfigGraph* graph, const ComponentIdMap_t& group);
    void print(std::ostream &os, const PartitionGraph* graph) const;

    inline const ComponentId_t key() const { return id; }

};

class PartitionLink {
public:
    LinkId_t                  id;
    ComponentId_t             component[2];
    SimTime_t                 latency[2];
    bool                      no_cut;

    PartitionLink(const ConfigLink& cl) {
        id = cl.id;
        component[0] = cl.component[0];
        component[1] = cl.component[1];
        latency[0] = cl.latency[0];
        latency[1] = cl.latency[1];
        no_cut = cl.no_cut;
    }

    inline const LinkId_t key() const { return id; }

    /** Return the minimum latency of this link (from both sides) */
    SimTime_t getMinLatency() const {
        if ( latency[0] < latency[1] ) return latency[0];
        return latency[1];    }
    
    /** Print the Link information */
    void print(std::ostream &os) const {
        os << "    Link " << id << std::endl;
        os << "      component[0] = " << component[0] << std::endl;
        os << "      latency[0] = " << latency[0] << std::endl;
        os << "      component[1] = " << component[1] << std::endl;
        os << "      latency[1] = " << latency[1] << std::endl;
    }
};

typedef SparseVectorMap<ComponentId_t,PartitionComponent> PartitionComponentMap_t;
typedef SparseVectorMap<LinkId_t,PartitionLink> PartitionLinkMap_t;

class PartitionGraph {
private:
    PartitionComponentMap_t comps;
    PartitionLinkMap_t      links;

public:
    /** Print the configuration graph */
	void print(std::ostream &os) const {
		os << "Printing graph" << std::endl;
		for (PartitionComponentMap_t::const_iterator i = comps.begin() ; i != comps.end() ; ++i) {
			i->print(os,this);
		}
	}

    PartitionComponentMap_t& getComponentMap() {
        return comps;
    }
    PartitionLinkMap_t& getLinkMap() {
        return links;
    }

    const PartitionLink& getLink(LinkId_t id) const {
        return links[id];
    }

    size_t getNumComponents() { return comps.size(); }

};

} // namespace SST

#endif // SST_CORE_CONFIGGRAPH_H
