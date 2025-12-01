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

#ifndef SST_CORE_MODEL_JSON_JSONMODEL_H
#define SST_CORE_MODEL_JSON_JSONMODEL_H

#include "sst/core/component.h"
#include "sst/core/config.h"
#include "sst/core/cputimer.h"
#include "sst/core/factory.h"
#include "sst/core/memuse.h"
#include "sst/core/model/configGraph.h"
#include "sst/core/model/sstmodel.h"
#include "sst/core/output.h"
#include "sst/core/rankInfo.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include "nlohmann/json.hpp"

#include <cstdint>
#include <fstream>
#include <map>
#include <stack>
#include <string>
#include <tuple>
#include <vector>

using namespace SST;
using json = nlohmann::json;

namespace SST::Core {

class SSTConfigSaxHandler
{
public:
    // SAX event handlers
    bool null();
    bool boolean(bool val);
    bool number_integer(json::number_integer_t val);
    bool number_unsigned(json::number_unsigned_t val);
    bool number_float(json::number_float_t val, const std::string& str);
    bool string(std::string& val);
    bool binary(json::binary_t& val);
    bool start_object(std::size_t elements);
    bool end_object();
    bool start_array(std::size_t elements);
    bool end_array();
    bool key(std::string& val);

    bool parse_error(std::size_t position, const std::string& lastToken, const json::exception& ex);
    void setConfigGraph(ConfigGraph* graph);
    const std::map<std::string, std::string> getProgramOptions();

    // error state
    std::size_t errorPos;
    std::string errorStr;

private:
    // Current parsing state for major sections
    enum ParseState {
        ROOT,
        PROGRAM_OPTIONS,
        SHARED_PARAMS,
        STATISTICS_OPTIONS,
        STATISTICS_GROUP,
        COMPONENTS,
        LINKS
    } current_state = ROOT;

    ConfigGraph* graph;

    std::vector<std::string>           path_stack;
    std::stack<json::value_t>          context_stack;
    std::stack<ConfigComponent*>       Parents;
    std::map<std::string, std::string> ProgramOptions;

    std::string current_key;
    std::string current_shared_name;
    std::string current_comp_name;
    std::string current_comp_stat_name;
    std::string current_subcomp_name;
    std::string current_subcomp_type;
    std::string current_grp_stat_name;
    std::string link_name;
    std::string left_comp;
    std::string left_port;
    std::string left_latency;
    std::string right_comp;
    std::string right_port;
    std::string right_latency;
    bool        no_cut   = false;
    bool        nonlocal = false;
    uint32_t    current_comp_rank;
    int         subcomp_slot = -1;
    int         right_rank   = -1;
    int         right_thread = -1;

    // FSM values
    bool foundComponents            = false;
    bool in_shared_params_object    = false;
    bool in_comp_params             = false;
    bool in_comp_stats              = false;
    bool in_comp_stats_params       = false;
    bool in_comp_subcomp            = false;
    bool in_comp_subsubcomp         = false;
    bool in_comp_subcomp_params     = false;
    bool in_grp_stats_output        = false;
    bool in_grp_stats_output_params = false;
    bool in_grp_stats_def           = false;
    bool in_grp_stats_def_params    = false;
    bool in_grp_stats_comps         = false;
    bool in_left_link               = false;
    bool in_right_link              = false;

    int array_depth = 0;

    ComponentId_t    current_comp_id;
    ConfigStatGroup* current_stat_group;
    ConfigComponent* current_component;
    ConfigComponent* current_sub_component;
    Params           current_stat_params;

    // internal parsing methods
    std::string   getCurrentPath() const;
    void          processValue(const json& value);
    ComponentId_t findComponentIdByName(const std::string& Name, bool& success);
};

class SSTJSONModelDefinition : public SSTModelDescription
{
public:
    SST_ELI_REGISTER_MODEL_DESCRIPTION(
          SST::Core::SSTJSONModelDefinition,
          "sst",
          "model.json",
          SST_ELI_ELEMENT_VERSION(1,0,0),
          "JSON model for building SST simulation graphs",
          true)

    SST_ELI_DOCUMENT_MODEL_SUPPORTED_EXTENSIONS(".json")

    SSTJSONModelDefinition(const std::string& script_file, int verbosity, Config* config, double start_time);
    virtual ~SSTJSONModelDefinition();

    ConfigGraph* createConfigGraph() override;

protected:
    std::string   scriptName;
    Output*       output;
    Config*       config;
    ConfigGraph*  graph;
    ComponentId_t nextComponentId;
    double        start_time;
};

} // namespace SST::Core

#endif // SST_CORE_MODEL_JSON_JSONMODEL_H
