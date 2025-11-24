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

#ifndef SST_CORE_MODEL_CONFIGCOMPONENT_H
#define SST_CORE_MODEL_CONFIGCOMPONENT_H

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
#include <utility>
#include <vector>

using namespace SST::Statistics;

namespace SST {

class ConfigGraph;
class ConfigLink;

/**
   Class that represents a PortModule in ConfigGraph
 */
class ConfigPortModule
{
public:
    std::string type;
    Params      params;
    uint8_t     stat_load_level = STATISTICLOADLEVELUNINITIALIZED;
    Params      all_stat_config; /*!< If all stats are enabled, the config information for the stats */
    std::map<std::string, Params> per_stat_configs;

    ConfigPortModule() = default;
    ConfigPortModule(const std::string& type, const Params& params) :
        type(type),
        params(params)
    {}

    void addParameter(const std::string& key, const std::string& value);
    void addSharedParamSet(const std::string& set);
    void setStatisticLoadLevel(const uint8_t level);
    void enableAllStatistics(const SST::Params& params);
    void enableStatistic(const std::string& statistic_name, const SST::Params& params);

    void serialize_order(SST::Core::Serialization::serializer& ser)
    {
        SST_SER(type);
        SST_SER(params);
        SST_SER(stat_load_level);
        SST_SER(all_stat_config);
        SST_SER(per_stat_configs);
    }
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

    std::map<std::string, std::vector<ConfigPortModule>>
        port_modules; /*!< Map of port names to port modules loaded on that port */
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

    void setConfigGraphPointer(ConfigGraph* graph_ptr);

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

    /**
       Adds a PortModule on the port 'port' of the associated component. Returns the index of the module in the
       component's vector of PortModules for the given port.
    */
    size_t addPortModule(const std::string& port, const std::string& type, const Params& params);

    std::vector<LinkId_t> allLinks() const;

    /**
       Gets all the links to return, then clears links from self and
       all subcomponents.  Used when splitting graphs.
    */
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
        SST_SER(port_modules);
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

} // namespace SST

#endif // SST_CORE_MODEL_CONFIGCOMPONENT_H
