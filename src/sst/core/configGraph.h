// -*- c++ -*-

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

#ifndef SST_CORE_CONFIGGRAPH_H
#define SST_CORE_CONFIGGRAPH_H

#include "sst/core/params.h"
#include "sst/core/rankInfo.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sparseVectorMap.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/unitAlgebra.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

using namespace SST::Statistics;

namespace SST {

class Simulation_impl;

class Config;
class TimeLord;
class ConfigGraph;

using ComponentIdMap_t = SparseVectorMap<ComponentId_t>;
using LinkIdMap_t      = std::vector<LinkId_t>;

/** Represents the configuration of a generic Link */
class ConfigLink : public SST::Core::Serialization::serializable
{
    // Static data structures to map latency string to an index that
    // will hold the SimTime_t for the latency once the atomic
    // timebase has been set.
    static std::map<std::string, uint32_t> lat_to_index;

    static uint32_t               getIndexForLatency(const char* latency);
    static std::vector<SimTime_t> initializeLinkLatencyVector();
    SimTime_t                     getLatencyFromIndex(uint32_t index);

public:
    LinkId_t      id;    /*!< ID of this link */
    LinkId_t      order; /*!< Number of components currently referring to this Link.  After graph construction, it will
                               be repurposed to hold the enforce_order value */
    ComponentId_t component[2] = { 0, 0 }; /*!< IDs of the connected components */
    SimTime_t     latency[2]   = { 0, 0 };
    std::string   name;    /*!< Name of this link */
    std::string   port[2]; /*!< Names of the connected ports */

    // At construct time, holds the index into the lat_index_to_time
    // vector to get the SimTime_t factor for the latency

    bool no_cut; /*!< If set to true, partitioner will not make a cut through this Link */

    // inline const std::string& key() const { return name; }
    inline LinkId_t key() const { return id; }

    /** Return the minimum latency of this link (from both sides) */
    SimTime_t getMinLatency() const
    {
        if ( latency[0] < latency[1] ) return latency[0];
        return latency[1];
    }

    std::string latency_str(uint32_t index) const;

    /** Print the Link information */
    void print(std::ostream& os) const
    {
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

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(id);
        SST_SER(name);
        SST_SER(component[0]);
        SST_SER(component[1]);
        SST_SER(port[0]);
        SST_SER(port[1]);
        SST_SER(latency[0]);
        SST_SER(latency[1]);
        SST_SER(order);
    }

    ImplementSerializable(SST::ConfigLink)

private:
    friend class ConfigGraph;
    explicit ConfigLink(LinkId_t id) :
        id(id),
        no_cut(false)
    {
        order = 0;

        // Initialize the component data items
        component[0] = ULONG_MAX;
        component[1] = ULONG_MAX;
    }

    ConfigLink(LinkId_t id, const std::string& n) :
        id(id),
        no_cut(false)
    {
        order = 0;
        name  = n;

        // Initialize the component data items
        component[0] = ULONG_MAX;
        component[1] = ULONG_MAX;
    }

    void updateLatencies();
};

class ConfigStatistic : public SST::Core::Serialization::serializable
{
public:
    StatisticId_t id; /*!< Unique ID of this statistic */
    Params        params;
    bool          shared = false;
    std::string   name;

    ConfigStatistic(StatisticId_t _id, bool _shared = false, std::string _name = "") :
        id(_id),
        shared(_shared),
        name(_name)
    {}

    ConfigStatistic() :
        id(stat_null_id)
    {}

    ConfigStatistic(const ConfigStatistic&)            = default;
    ConfigStatistic(ConfigStatistic&&)                 = default;
    ConfigStatistic& operator=(const ConfigStatistic&) = default;
    ConfigStatistic& operator=(ConfigStatistic&&)      = default;
    ~ConfigStatistic() override                        = default;

    inline const StatisticId_t& getId() const { return id; }

    void addParameter(const std::string& key, const std::string& value, bool overwrite);

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(id);
        SST_SER(shared);
        SST_SER(name);
        SST_SER(params);
    }

    ImplementSerializable(ConfigStatistic)

    static constexpr StatisticId_t stat_null_id = std::numeric_limits<StatisticId_t>::max();
};

class ConfigStatGroup : public SST::Core::Serialization::serializable
{
public:
    std::string                   name;
    std::map<std::string, Params> statMap;
    std::vector<ComponentId_t>    components;
    size_t                        outputID;
    UnitAlgebra                   outputFrequency;

    explicit ConfigStatGroup(const std::string& name) :
        name(name),
        outputID(0)
    {}
    ConfigStatGroup() {} /* Do not use */

    bool addComponent(ComponentId_t id);
    bool addStatistic(const std::string& name, Params& p);
    bool setOutput(size_t id);
    bool setFrequency(const std::string& freq);

    /**
     * Checks to make sure that all components in the group support all
     * of the statistics as configured in the group.
     * @return pair of:  bool for OK, string for error message (if any)
     */
    std::pair<bool, std::string> verifyStatsAndComponents(const ConfigGraph* graph);

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(name);
        SST_SER(statMap);
        SST_SER(components);
        SST_SER(outputID);
        SST_SER(outputFrequency);
    }

    ImplementSerializable(SST::ConfigStatGroup)
};

class ConfigStatOutput : public SST::Core::Serialization::serializable
{
public:
    std::string type;
    Params      params;

    explicit ConfigStatOutput(const std::string& type) :
        type(type)
    {}
    ConfigStatOutput() {}

    void addParameter(const std::string& key, const std::string& val) { params.insert(key, val); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(type);
        SST_SER(params);
    }

    ImplementSerializable(SST::ConfigStatOutput)
};

using ConfigLinkMap_t = SparseVectorMap<LinkId_t, ConfigLink*>;

/**
   Class that represents a PortModule in ConfigGraph
 */
class ConfigPortModule : public SST::Core::Serialization::serializable
{
public:
    std::string type;
    Params      params;

    ConfigPortModule() = default;
    ConfigPortModule(const std::string& type, const Params& params) :
        type(type),
        params(params)
    {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(type);
        SST_SER(params);
    }
    ImplementSerializable(SST::ConfigPortModule)
};


/** Represents the configuration of a generic component */
class ConfigComponent : public SST::Core::Serialization::serializable
{
    friend class ComponentInfo;

public:
    ComponentId_t         id;            /*!< Unique ID of this component */
    ConfigGraph*          graph;         /*!< Graph that this component belongs to */
    std::string           name;          /*!< Name of this component, or slot name for subcomp */
    int                   slot_num;      /*!< Slot number.  Only valid for subcomponents */
    std::string           type;          /*!< Type of this component */
    float                 weight;        /*!< Partitioning weight for this component */
    RankInfo              rank;          /*!< Parallel Rank for this component */
    std::vector<LinkId_t> links;         /*!< List of links connected */
    Params                params;        /*!< Set of Parameters */
    uint8_t               statLoadLevel; /*!< Statistic load level for this component */

    std::map<std::string, std::vector<ConfigPortModule>> portModules;
    std::map<std::string, StatisticId_t>
                    enabledStatNames; /*!< Map of explicitly enabled statistic names to unique IDs */
    bool            enabledAllStats;  /*!< Whether all stats in this (sub)component have been enabled */
    ConfigStatistic allStatConfig;    /*!< If all stats are enabled, the config information for the stats */

    std::vector<ConfigComponent*> subComponents; /*!< List of subcomponents */
    std::vector<double>           coords;
    uint16_t nextSubID;  /*!< Next subID to use for children, if component, if subcomponent, subid of parent */
    uint16_t nextStatID; /*!< Next statID to use for children */
    bool     visited;    /*! Used when traversing graph to indicate component was visited already */

    static constexpr ComponentId_t null_id = std::numeric_limits<ComponentId_t>::max();

    inline const ComponentId_t& key() const { return id; }

    /** Print Component information */
    void print(std::ostream& os) const;

    ConfigComponent* cloneWithoutLinks(ConfigGraph* new_graph) const;
    ConfigComponent* cloneWithoutLinksOrParams(ConfigGraph* new_graph) const;
    void             setConfigGraphPointer(ConfigGraph* graph_ptr);

    ~ConfigComponent() {}
    ConfigComponent() :
        id(null_id),
        statLoadLevel(STATISTICLOADLEVELUNINITIALIZED),
        enabledAllStats(false),
        nextSubID(1),
        visited(false)
    {}

    StatisticId_t getNextStatisticID();

    ConfigComponent* getParent() const;
    std::string      getFullName() const;

    void                   setRank(RankInfo r);
    void                   setWeight(double w);
    void                   setCoordinates(const std::vector<double>& c);
    void                   addParameter(const std::string& key, const std::string& value, bool overwrite);
    ConfigComponent*       addSubComponent(const std::string& name, const std::string& type, int slot);
    ConfigComponent*       findSubComponent(ComponentId_t);
    const ConfigComponent* findSubComponent(ComponentId_t) const;
    ConfigComponent*       findSubComponentByName(const std::string& name);
    ConfigStatistic*       findStatistic(const std::string& name) const;
    ConfigStatistic*       insertStatistic(StatisticId_t id);
    ConfigStatistic*       findStatistic(StatisticId_t) const;
    ConfigStatistic*       enableStatistic(
              const std::string& statisticName, const SST::Params& params, bool recursively = false);
    ConfigStatistic* createStatistic();
    bool             reuseStatistic(const std::string& statisticName, StatisticId_t sid);
    void             addStatisticParameter(
                    const std::string& statisticName, const std::string& param, const std::string& value, bool recursively = false);
    void setStatisticParameters(const std::string& statisticName, const Params& params, bool recursively = false);
    void setStatisticLoadLevel(uint8_t level, bool recursively = false);

    void addSharedParamSet(const std::string& set) { params.addSharedParamSet(set); }
    [[deprecated(
        "addGlobalParamSet() has been deprecated and will be removed in SST 16.  Please use addSharedParamSet()")]]
    void addGlobalParamSet(const std::string& set)
    {
        params.addSharedParamSet(set);
    }
    std::vector<std::string> getParamsLocalKeys() const { return params.getLocalKeys(); }
    std::vector<std::string> getSubscribedSharedParamSets() const { return params.getSubscribedSharedParamSets(); }

    [[deprecated("getSubscribedGlobalParamSets() has been deprecated and will be removed in SST 16.  Please use "
                 "getSubscribedSharedParamSets()")]]
    std::vector<std::string> getSubscribedGlobalParamSets() const
    {
        return params.getSubscribedSharedParamSets();
    }

    void addPortModule(const std::string& port, const std::string& type, const Params& params);

    std::vector<LinkId_t> allLinks() const;

    // Gets all the links to return, then clears links from self and
    // all subcomponents.  Used when splitting graphs.
    std::vector<LinkId_t> clearAllLinks();

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(id);
        SST_SER(name);
        SST_SER(slot_num);
        SST_SER(type);
        SST_SER(weight);
        SST_SER(rank.rank);
        SST_SER(rank.thread);
        SST_SER(links);
        SST_SER(params);
        SST_SER(statLoadLevel);
        SST_SER(portModules);
        SST_SER(enabledStatNames);
        SST_SER(enabledAllStats);
        SST_SER(statistics_);
        SST_SER(allStatConfig);
        SST_SER(subComponents);
        SST_SER(coords);
        SST_SER(nextSubID);
        SST_SER(nextStatID);
    }

    ImplementSerializable(SST::ConfigComponent)

private:
    std::map<StatisticId_t, ConfigStatistic>
        statistics_; /* Map of explicitly enabled stat IDs to config info for each stat */

    ComponentId_t getNextSubComponentID();

    friend class ConfigGraph;
    /** Checks to make sure port names are valid and that a port isn't used twice
     */
    void checkPorts() const;

    /** Create a new Component */
    ConfigComponent(ComponentId_t id, ConfigGraph* graph, const std::string& name, const std::string& type,
        float weight, RankInfo rank) :
        id(id),
        graph(graph),
        name(name),
        slot_num(0),
        type(type),
        weight(weight),
        rank(rank),
        statLoadLevel(STATISTICLOADLEVELUNINITIALIZED),
        enabledAllStats(false),
        nextSubID(1),
        nextStatID(1)
    {
        coords.resize(3, 0.0);
    }

    ConfigComponent(ComponentId_t id, ConfigGraph* graph, uint16_t parent_subid, const std::string& name, int slot_num,
        const std::string& type, float weight, RankInfo rank) :
        id(id),
        graph(graph),
        name(name),
        slot_num(slot_num),
        type(type),
        weight(weight),
        rank(rank),
        statLoadLevel(STATISTICLOADLEVELUNINITIALIZED),
        enabledAllStats(false),
        nextSubID(parent_subid),
        nextStatID(parent_subid)
    {
        coords.resize(3, 0.0);
    }
};

/** Map names to Links */
// using ConfigLinkMap_t = std::map<std::string,ConfigLink>;
// using ConfigLinkMap_t = SparseVectorMap<std::string,ConfigLink>;
/** Map IDs to Components */
using ConfigComponentMap_t     = SparseVectorMap<ComponentId_t, ConfigComponent*>;
/** Map names to Components */
using ConfigComponentNameMap_t = std::map<std::string, ComponentId_t>;
/** Map names to Parameter Sets: XML only */
using ParamsMap_t              = std::map<std::string, Params*>;
/** Map names to variable values:  XML only */
using VariableMap_t            = std::map<std::string, std::string>;

class PartitionGraph;

struct StatsConfig
{
    std::map<std::string, ConfigStatGroup> groups;
    std::vector<ConfigStatOutput>          outputs; // [0] is default group
    uint8_t                                load_level;

    void serialize_order(SST::Core::Serialization::serializer& ser)
    {
        SST_SER(groups);
        SST_SER(outputs);
        SST_SER(load_level);
    }
};

/** A Configuration Graph
 *  A graph representing Components and Links
 */
class ConfigGraph : public SST::Core::Serialization::serializable
{
public:
    /** Print the configuration graph */
    void print(std::ostream& os) const
    {
        os << "Printing graph" << std::endl;
        for ( ConfigComponentMap_t::const_iterator i = comps_.begin(); i != comps_.end(); ++i ) {
            (*i)->print(os);
        }
    }

    ConfigGraph() :
        nextComponentId(0)
    {
        links_.clear();
        comps_.clear();
        // Init the statistic output settings
        stats_config_             = new StatsConfig();
        stats_config_->load_level = STATISTICSDEFAULTLOADLEVEL;
        stats_config_->outputs.emplace_back(STATISTICSDEFAULTOUTPUTNAME);
        // Output is only used for warnings or fatal that should go to stderr
        Output& o = Output::getDefaultObject();
        output.init(o.getPrefix(), o.getVerboseLevel(), o.getVerboseMask(), Output::STDERR);
    }

    ~ConfigGraph()
    {
        for ( auto comp : comps_ ) {
            delete comp;
        }

        if ( stats_config_ ) delete stats_config_;
    }

    size_t getNumComponents() { return comps_.data.size(); }

    size_t getNumComponentsInMPIRank(uint32_t rank);

    /** Helper function to set all the ranks to the same value */
    void setComponentRanks(RankInfo rank);
    /** Checks to see if rank contains at least one component */
    bool containsComponentInRank(RankInfo rank);
    /** Verify that all components have valid Ranks assigned */
    bool checkRanks(RankInfo ranks);

    // API for programmatic initialization
    /** Create a new component */
    ComponentId_t addComponent(const std::string& name, const std::string& type);

    /** Add a parameter to a shared param set */
    void addSharedParam(const std::string& shared_set, const std::string& key, const std::string& value);

    [[deprecated("addGlobalParam() has been deprecated and will be removed in SST 16.  Please use addSharedParam()")]]
    void addGlobalParam(const std::string& shared_set, const std::string& key, const std::string& value);

    /** Set the statistic output module */
    void setStatisticOutput(const std::string& name);

    /** Add parameter to the statistic output module */
    void addStatisticOutputParameter(const std::string& param, const std::string& value);

    /** Set a set of parameter to the statistic output module */
    void setStatisticOutputParams(const Params& p);

    /** Set the statistic system load level */
    void setStatisticLoadLevel(uint8_t loadLevel);

    std::vector<ConfigStatOutput>& getStatOutputs() { return stats_config_->outputs; }

    const ConfigStatOutput& getStatOutput(size_t index = 0) const { return stats_config_->outputs[index]; }

    long getStatLoadLevel() const { return stats_config_->load_level; }

    /**
       Create link and return it's ID.  The provided name is not
       checked against existing links
     */
    LinkId_t createLink(const char* name, const char* latency = nullptr);

    /** Add a Link to a Component on a given Port */
    void addLink(ComponentId_t comp_id, LinkId_t link_id, const char* port, const char* latency_str);
    /** Set a Link to be no-cut */
    void setLinkNoCut(LinkId_t link_name);

    /** Perform any post-creation cleanup processes */
    void postCreationCleanup();

    /** Check the graph for Structural errors */
    bool checkForStructuralErrors();

    // Temporary until we have a better API
    /** Return the map of components */
    ConfigComponentMap_t& getComponentMap() { return comps_; }

    const std::map<std::string, ConfigStatGroup>& getStatGroups() const { return stats_config_->groups; }
    ConfigStatGroup*                              getStatGroup(const std::string& name)
    {
        auto found = stats_config_->groups.find(name);
        if ( found == stats_config_->groups.end() ) {
            bool ok;
            std::tie(found, ok) = stats_config_->groups.emplace(name, name);
        }
        return &(found->second);
    }

    bool                   containsComponent(ComponentId_t id) const;
    ConfigComponent*       findComponent(ComponentId_t);
    ConfigComponent*       findComponentByName(const std::string& name);
    const ConfigComponent* findComponent(ComponentId_t) const;

    bool             containsStatistic(StatisticId_t id) const;
    ConfigStatistic* findStatistic(StatisticId_t) const;

    /** Return the map of links */
    ConfigLinkMap_t& getLinkMap() { return links_; }

    ConfigGraph* getSubGraph(uint32_t start_rank, uint32_t end_rank);
    ConfigGraph* getSubGraph(const std::set<uint32_t>& rank_set);

    ConfigGraph* splitGraph(const std::set<uint32_t>& orig_rank_set, const std::set<uint32_t>& new_rank_set);

    PartitionGraph* getPartitionGraph();
    PartitionGraph* getCollapsedPartitionGraph();
    void            annotateRanks(PartitionGraph* graph);
    void            getConnectedNoCutComps(ComponentId_t start, std::set<ComponentId_t>& group);
    StatsConfig*    getStatsConfig() { return stats_config_; }
    StatsConfig*    takeStatsConfig()
    {
        auto* ret     = stats_config_;
        stats_config_ = nullptr;
        return ret;
    }

    void setComponentConfigGraphPointers();
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(links_);
        SST_SER(comps_);
        SST_SER(stats_config_);
        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            // Need to reintialize the ConfigGraph ptrs in the
            // ConfigComponents
            setComponentConfigGraphPointers();
        }
    }

private:
    friend class Simulation_impl;
    friend class SSTSDLModelDefinition;

    Output output;

    ComponentId_t nextComponentId;

    ConfigLinkMap_t          links_;         // SparseVectorMap
    ConfigComponentMap_t     comps_;         // SparseVectorMap
    ConfigComponentNameMap_t comps_by_name_; // std::map

    std::map<std::string, LinkId_t> link_names_;

    StatsConfig* stats_config_;

    ImplementSerializable(SST::ConfigGraph)

    // Filter class
    class GraphFilter
    {
        ConfigGraph*              ograph_;
        ConfigGraph*              ngraph_;
        const std::set<uint32_t>& oset_;
        const std::set<uint32_t>& nset_;

    public:
        GraphFilter(ConfigGraph* original_graph, ConfigGraph* new_graph, const std::set<uint32_t>& original_rank_set,
            const std::set<uint32_t>& new_rank_set);

        ConfigLink*      operator()(ConfigLink* link);
        ConfigComponent* operator()(ConfigComponent* comp);
    };
};

class PartitionComponent
{
public:
    ComponentId_t id;
    float         weight;
    RankInfo      rank;
    LinkIdMap_t   links;

    ComponentIdMap_t group;

    explicit PartitionComponent(const ConfigComponent* cc)
    {
        id     = cc->id;
        weight = cc->weight;
        rank   = cc->rank;
    }

    explicit PartitionComponent(LinkId_t id) :
        id(id),
        weight(0),
        rank(RankInfo(RankInfo::UNASSIGNED, 0))
    {}

    // PartitionComponent(ComponentId_t id, ConfigGraph* graph, const ComponentIdMap_t& group);
    void print(std::ostream& os, const PartitionGraph* graph) const;

    inline ComponentId_t key() const { return id; }
};

class PartitionLink
{
public:
    LinkId_t      id;
    ComponentId_t component[2];
    SimTime_t     latency[2];
    bool          no_cut;

    PartitionLink(const ConfigLink& cl)
    {
        id           = cl.id;
        component[0] = cl.component[0];
        component[1] = cl.component[1];
        latency[0]   = cl.latency[0];
        latency[1]   = cl.latency[1];
        no_cut       = cl.no_cut;
    }

    inline LinkId_t key() const { return id; }

    /** Return the minimum latency of this link (from both sides) */
    SimTime_t getMinLatency() const
    {
        if ( latency[0] < latency[1] ) return latency[0];
        return latency[1];
    }

    /** Print the Link information */
    void print(std::ostream& os) const
    {
        os << "    Link " << id << std::endl;
        os << "      component[0] = " << component[0] << std::endl;
        os << "      latency[0] = " << latency[0] << std::endl;
        os << "      component[1] = " << component[1] << std::endl;
        os << "      latency[1] = " << latency[1] << std::endl;
    }
};

using PartitionComponentMap_t = SparseVectorMap<ComponentId_t, PartitionComponent*>;
using PartitionLinkMap_t      = SparseVectorMap<LinkId_t, PartitionLink>;

class PartitionGraph
{
private:
    PartitionComponentMap_t comps_;
    PartitionLinkMap_t      links_;

public:
    /** Print the configuration graph */
    void print(std::ostream& os) const
    {
        os << "Printing graph" << std::endl;
        for ( PartitionComponentMap_t::const_iterator i = comps_.begin(); i != comps_.end(); ++i ) {
            (*i)->print(os, this);
        }
    }

    PartitionComponentMap_t& getComponentMap() { return comps_; }
    PartitionLinkMap_t&      getLinkMap() { return links_; }

    const PartitionLink& getLink(LinkId_t id) const { return links_[id]; }

    size_t getNumComponents() { return comps_.size(); }
};

} // namespace SST

#endif // SST_CORE_CONFIGGRAPH_H
