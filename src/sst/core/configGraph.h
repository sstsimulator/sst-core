// -*- c++ -*-

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CONFIGGRAPH_H
#define SST_CORE_CONFIGGRAPH_H

#include "sst/core/sst_types.h"

#include <vector>
#include <map>
#include <set>
#include <climits>

#include <sst/core/sparseVectorMap.h>
#include <sst/core/params.h>
#include <sst/core/statapi/statbase.h>
#include <sst/core/statapi/statoutput.h>
#include <sst/core/rankInfo.h>
#include <sst/core/unitAlgebra.h>

#include <sst/core/serialization/serializable.h>

// #include "sst/core/simulation.h"

using namespace SST::Statistics;

namespace SST {


class Simulation;

class Config;
class TimeLord;
class ConfigGraph;

typedef SparseVectorMap<ComponentId_t> ComponentIdMap_t;
typedef std::vector<LinkId_t> LinkIdMap_t;


/** Represents the configuration of a generic Link */
class ConfigLink : public SST::Core::Serialization::serializable {
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

    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        ser & id;
        ser & name;
        ser & component[0];
        ser & component[1];
        ser & port[0];
        ser & port[1];
        ser & latency[0];
        ser & latency[1];
        ser & current_ref;
    }

    ImplementSerializable(SST::ConfigLink)
    
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


};



class ConfigStatGroup : public SST::Core::Serialization::serializable {
public:
    std::string name;
    std::map<std::string, Params> statMap;
    std::vector<ComponentId_t> components;
    size_t outputID;
    UnitAlgebra outputFrequency;

    ConfigStatGroup(const std::string &name) : name(name), outputID(0) { }
    ConfigStatGroup() {} /* Do not use */


    bool addComponent(ComponentId_t id);
    bool addStatistic(const std::string &name, Params &p);
    bool setOutput(size_t id);
    bool setFrequency(const std::string &freq);

    /**
     * Checks to make sure that all components in the group support all
     * of the statistics as configured in the group.
     * @return pair of:  bool for OK, string for error message (if any)
     */
    std::pair<bool, std::string> verifyStatsAndComponents(const ConfigGraph* graph);



    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        ser & name;
        ser & statMap;
        ser & components;
        ser & outputID;
        ser & outputFrequency;
    }

    ImplementSerializable(SST::ConfigStatGroup)

};


class ConfigStatOutput : public SST::Core::Serialization::serializable {
public:

    std::string type;
    Params params;

    ConfigStatOutput(const std::string &type) : type(type) { }
    ConfigStatOutput() { }

    void addParameter(const std::string &key, const std::string &val) {
        params.insert(key, val);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        ser & type;
        ser & params;
    }

    ImplementSerializable(SST::ConfigStatOutput)
};


typedef SparseVectorMap<LinkId_t,ConfigLink> ConfigLinkMap_t;

/** Represents the configuration of a generic component */
class ConfigComponent : public SST::Core::Serialization::serializable {
public:
    ComponentId_t                 id;                /*!< Unique ID of this component */
    std::string                   name;              /*!< Name of this component, or slot name for subcomp */
    int                           slot_num;          /*!< Slot number.  Only valid for subcomponents */
    std::string                   type;              /*!< Type of this component */
    float                         weight;            /*!< Partitioning weight for this component */
    RankInfo                      rank;              /*!< Parallel Rank for this component */
    std::vector<LinkId_t>         links;             /*!< List of links connected */
    Params                        params;            /*!< Set of Parameters */
    std::vector<Statistics::StatisticInfo> enabledStatistics; /*!< List of statistics to be enabled */
    std::vector<ConfigComponent>  subComponents; /*!< List of subcomponents */
    std::vector<double>           coords;

    inline const ComponentId_t& key()const { return id; }

    /** Print Component information */
    void print(std::ostream &os) const;

    ConfigComponent cloneWithoutLinks() const;
    ConfigComponent cloneWithoutLinksOrParams() const;

    ~ConfigComponent() {}
    ConfigComponent() : id(-1) {}

    void setRank(RankInfo r);
    void setWeight(double w);
    void setCoordinates(const std::vector<double> &c);
    void addParameter(const std::string &key, const std::string &value, bool overwrite);
    ConfigComponent* addSubComponent(ComponentId_t, const std::string &name, const std::string &type, int slot);
    ConfigComponent* findSubComponent(ComponentId_t);
    const ConfigComponent* findSubComponent(ComponentId_t) const;
    void enableStatistic(const std::string &statisticName, bool recursively = false);
    void addStatisticParameter(const std::string &statisticName, const std::string &param, const std::string &value, bool recursively = false);
    void setStatisticParameters(const std::string &statisticName, const Params &params, bool recursively = false);

    std::vector<LinkId_t> allLinks() const;

    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        ser & id;
        ser & name;
        ser & slot_num;
        ser & type;
        ser & weight;
        ser & rank.rank;
        ser & rank.thread;
        ser & links;
        ser & params;
        ser & enabledStatistics;
        ser & subComponents;
    }

    ImplementSerializable(SST::ConfigComponent)

private:

    friend class ConfigGraph;
    /** Create a new Component */
    ConfigComponent(ComponentId_t id, std::string name, std::string type, float weight, RankInfo rank) :
        id(id),
        name(name),
        type(type),
        weight(weight),
        rank(rank)
    {
        coords.resize(3, 0.0);
    }

    ConfigComponent(ComponentId_t id, std::string name, int slot_num, std::string type, float weight, RankInfo rank) :
        id(id),
        name(name),
        slot_num(slot_num),
        type(type),
        weight(weight),
        rank(rank)
    {
        coords.resize(3, 0.0);
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
class ConfigGraph : public SST::Core::Serialization::serializable {
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
        // Init the statistic output settings
        statLoadLevel = STATISTICSDEFAULTLOADLEVEL;
        statOutputs.emplace_back(STATISTICSDEFAULTOUTPUTNAME);
    }

    size_t getNumComponents() { return comps.data.size(); }
    
    /** Helper function to set all the ranks to the same value */
    void setComponentRanks(RankInfo rank);
    /** Checks to see if rank contains at least one component */
    bool containsComponentInRank(RankInfo rank);
    /** Verify that all components have valid Ranks assigned */
    bool checkRanks(RankInfo ranks);


    // API for programmatic initialization
    /** Create a new component with weight and rank */
    ComponentId_t addComponent(ComponentId_t id, std::string name, std::string type, float weight, RankInfo rank);
    /** Create a new component */
    ComponentId_t addComponent(ComponentId_t id, std::string name, std::string type);


    /** Set the statistic output module */
    void setStatisticOutput(const std::string &name);

    /** Add parameter to the statistic output module */
    void addStatisticOutputParameter(const std::string &param, const std::string &value);

    /** Set a set of parameter to the statistic output module */
    void setStatisticOutputParams(const Params& p);

    /** Set the statistic system load level */
    void setStatisticLoadLevel(uint8_t loadLevel);

    /** Enable a Statistics assigned to a component */
    void enableStatisticForComponentName(std::string ComponentName, std::string statisticName);
    void enableStatisticForComponentType(std::string ComponentType, std::string statisticName);

    /** Add Parameters for a Statistic */
    void addStatisticParameterForComponentName(const std::string &ComponentName, const std::string &statisticName, const std::string &param, const std::string &value);
    void addStatisticParameterForComponentType(const std::string &ComponentType, const std::string &statisticName, const std::string &param, const std::string &value);

    std::vector<ConfigStatOutput>& getStatOutputs() {return statOutputs;}
    const ConfigStatOutput& getStatOutput(size_t index = 0) const {return statOutputs[index];}
    long getStatLoadLevel() const {return statLoadLevel;}

    /** Add a Link to a Component on a given Port */
    void addLink(ComponentId_t comp_id, std::string link_name, std::string port, std::string latency_str, bool no_cut = false);

    /** Set a Link to be no-cut */
    void setLinkNoCut(std::string link_name);

    /** Perform any post-creation cleanup processes */
    void postCreationCleanup();

    /** Check the graph for Structural errors */
    bool checkForStructuralErrors();

    // Temporary until we have a better API
    /** Return the map of components */
    ConfigComponentMap_t& getComponentMap() {
        return comps;
    }

    const std::map<std::string, ConfigStatGroup>& getStatGroups() const { return statGroups; }
    ConfigStatGroup* getStatGroup(const std::string &name) {
        auto found = statGroups.find(name);
        if ( found == statGroups.end() ) {
            bool ok;
            std::tie(found, ok) = statGroups.emplace(name, name);
        }
        return &(found->second);
    }

    bool containsComponent(ComponentId_t id) const;
    ConfigComponent* findComponent(ComponentId_t);
    const ConfigComponent* findComponent(ComponentId_t) const;

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

    void serialize_order(SST::Core::Serialization::serializer &ser) override
	{
		ser & links;
		ser & comps;
		ser & statOutputs;
		ser & statLoadLevel;
        ser & statGroups;
	}

private:
    friend class Simulation;
    friend class SSTSDLModelDefinition;

    ConfigLinkMap_t      links;
    ConfigComponentMap_t comps;
    std::map<std::string, ConfigStatGroup> statGroups;

    // temporary as a test
    std::map<std::string,LinkId_t> link_names;

    std::vector<ConfigStatOutput> statOutputs; // [0] is default
    uint8_t     statLoadLevel;

    ImplementSerializable(SST::ConfigGraph)

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

    inline ComponentId_t key() const { return id; }

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

    inline LinkId_t key() const { return id; }

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
