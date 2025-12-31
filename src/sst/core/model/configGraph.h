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

#ifndef SST_CORE_MODEL_CONFIGGRAPH_H
#define SST_CORE_MODEL_CONFIGGRAPH_H

#include "sst/core/model/configComponent.h"
#include "sst/core/model/configLink.h"
#include "sst/core/model/configStatistic.h"
#include "sst/core/params.h"
#include "sst/core/rankInfo.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sparseVectorMap.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/timeConverter.h"
#include "sst/core/unitAlgebra.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

using namespace SST::Statistics;

namespace SST {

class Simulation_impl;

class Config;
class TimeLord;
struct StatsConfig;
class ConfigComponent;

using ComponentIdMap_t = SparseVectorMap<ComponentId_t>;
using LinkIdMap_t      = std::vector<LinkId_t>;

class ConfigLink;


using ConfigLinkMap_t = SparseVectorMap<LinkId_t, ConfigLink*>;

/** Map names to Links */
/** Map IDs to Components */
using ConfigComponentMap_t     = SparseVectorMap<ComponentId_t, ConfigComponent*>;
/** Map names to Components */
using ConfigComponentNameMap_t = std::map<std::string, ComponentId_t>;
/** Map names to Parameter Sets: XML only */
using ParamsMap_t              = std::map<std::string, Params*>;
/** Map names to variable values:  XML only */
using VariableMap_t            = std::map<std::string, std::string>;

class PartitionGraph;


/** A Configuration Graph
 *  A graph representing Components and Links
 */
class ConfigGraph : public SST::Core::Serialization::serializable
{
public:
    /** Print the configuration graph */
    void print(std::ostream& os) const
    {
        os << "Printing graph\n";
        os << "Components:\n";
        for ( ConfigComponentMap_t::const_iterator i = comps_.begin(); i != comps_.end(); ++i ) {
            (*i)->print(os);
        }
        os << "Links:\n";
        for ( auto i = links_.begin(); i != links_.end(); ++i ) {
            (*i)->print(os);
        }
    }

    ConfigGraph();
    ~ConfigGraph();

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

    std::vector<ConfigStatOutput>& getStatOutputs();

    const ConfigStatOutput& getStatOutput(size_t index = 0) const;

    long getStatLoadLevel() const;

    /**
       Create link and return it's ID.  The provided name is not
       checked against existing links
     */
    LinkId_t createLink(const char* name, const char* latency = nullptr);

    /** Add a Link to a Component on a given Port */
    void addLink(ComponentId_t comp_id, LinkId_t link_id, const char* port, const char* latency_str);

    /**
       Adds the remote rank info for nonlocal links
     */
    void addNonLocalLink(LinkId_t link_id, int rank, int thread);

    /** Set a Link to be no-cut */
    void setLinkNoCut(LinkId_t link_name);

    /** Perform any post-creation cleanup processes */
    void postCreationCleanup();

    /** Check the graph for Structural errors */
    bool checkForStructuralErrors();

    // Temporary until we have a better API
    /** Return the map of components */
    ConfigComponentMap_t& getComponentMap() { return comps_; }

    const std::map<std::string, ConfigStatGroup>& getStatGroups() const;
    ConfigStatGroup*                              getStatGroup(const std::string& name);

    bool                   containsComponent(ComponentId_t id) const;
    ConfigComponent*       findComponent(ComponentId_t);
    ConfigComponent*       findComponentByName(const std::string& name);
    const ConfigComponent* findComponent(ComponentId_t) const;

    ConfigStatistic* findStatistic(StatisticId_t) const;

    /** Return the map of links */
    ConfigLinkMap_t& getLinkMap() { return links_; }

    ConfigGraph* splitGraph(const std::set<uint32_t>& orig_rank_set, const std::set<uint32_t>& new_rank_set);
    void         reduceGraphToSingleRank(uint32_t rank);

    SimTime_t getMinimumPartitionLatency();

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
            // Need to reinitialize the ConfigGraph ptrs in the
            // ConfigComponents
            setComponentConfigGraphPointers();
        }

        SST_SER(cpt_ranks);
        SST_SER(cpt_currentSimCycle);
        SST_SER(cpt_currentPriority);
        SST_SER(cpt_minPart);
        SST_SER(cpt_minPartTC);
        SST_SER(cpt_max_event_id);

        SST_SER(*(cpt_libnames.get()));
        SST_SER(*(cpt_shared_objects.get()));
        SST_SER(*(cpt_stats_config.get()));
    }

    void restoreRestartData();

    /********* vv Variables used on restarts only vv ***********/
    RankInfo      cpt_ranks;
    SimTime_t     cpt_currentSimCycle = 0;
    int           cpt_currentPriority = 0;
    SimTime_t     cpt_minPart         = std::numeric_limits<SimTime_t>::max();
    TimeConverter cpt_minPartTC;
    uint64_t      cpt_max_event_id = 0;

    std::shared_ptr<std::set<std::string>> cpt_libnames       = std::make_shared<std::set<std::string>>();
    std::shared_ptr<std::vector<char>>     cpt_shared_objects = std::make_shared<std::vector<char>>();
    std::shared_ptr<std::vector<char>>     cpt_stats_config   = std::make_shared<std::vector<char>>();

    /********* ^^ Variables used on restarts only ^^ ***********/


private:
    friend class Simulation_impl;

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
        os << "Printing graph\n";
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

#endif // SST_CORE_MODEL_CONFIGGRAPH_H
