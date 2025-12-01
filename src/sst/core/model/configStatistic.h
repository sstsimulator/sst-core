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

#ifndef SST_CORE_CONFIGCSTATISTIC_H
#define SST_CORE_CONFIGCSTATISTIC_H

#include "sst/core/params.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

using namespace SST::Statistics;

namespace SST {

class ConfigGraph;
class ConfigLink;

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

} // namespace SST

#endif // SST_CORE_CONFIGCOMPONENT_H
