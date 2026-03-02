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

#include "sst/core/config.h"
#include "sst/core/cputimer.h"
#include "sst/core/factory.h"
#include "sst/core/memuse.h"
#include "sst/core/model/configGraph.h"
#include "sst/core/model/json/jsonstates.h"
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
using namespace SST::Core;
using namespace SST::Core::JSONParser;
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
    std::size_t error_pos_;
    std::string error_str_;

private:
    // Helpers for constructing objects
    void constructComponent();
    void constructSubComponent();
    void constructPortModule();

    // Holds information needed to construct a component or subcomponent
    struct JSONShadowComponent
    {
        std::string           name        = ""; // Component or slot name
        std::string           type        = ""; // Type
        unsigned              slot_number = 0;  // Slot number for subcomponents
        SST::ConfigComponent* comp        = nullptr;
    };

    struct JSONShadowStatistic
    {
        std::string name = "";
        Params      params;
        bool        shared      = false;
        std::string shared_name = "";

        void reset()
        {
            name = "";
            params.clear();
            shared      = false;
            shared_name = "";
        }
    };

    struct JSONShadowLink
    {
        std::string        name      = "";
        bool               nocut     = false;
        bool               nonlocal  = false;
        // Left side
        SST::ComponentId_t leftcomp  = -1;
        std::string        leftport  = "";
        std::string        leftlat   = "";
        // Right side if not nonlocal
        SST::ComponentId_t rightcomp = -1;
        std::string        rightport = "";
        std::string        rightlat  = "";
        // Right side if nonlocal
        int                rank      = -1;
        int                thread    = -1;

        void reset()
        {
            name      = "";
            nocut     = false;
            nonlocal  = false;
            leftcomp  = -1;
            leftport  = "";
            leftlat   = "";
            rightcomp = -1;
            rightport = "";
            rightlat  = "";
            rank      = -1;
            thread    = -1;
        }
    };

    struct JSONShadowPortModule
    {
        std::string              port = "";
        std::string              type = "";
        Params                   params;
        uint8_t                  stat_load_level = STATISTICLOADLEVELUNINITIALIZED;
        std::vector<std::string> shared_param_sets;
        ConfigPortModule*        pm = nullptr;

        void reset()
        {
            port = "";
            type = "";
            params.clear();
            stat_load_level = STATISTICLOADLEVELUNINITIALIZED;
            shared_param_sets.clear();
            pm = nullptr;
        }
    };

    ConfigGraph* graph_;

    State                                   current_state_ = State::Entry;
    std::vector<JSONShadowComponent>        shadow_component_stack_;
    JSONShadowStatistic                     shadow_statistic_;
    JSONShadowLink                          shadow_link_;
    JSONShadowPortModule                    shadow_port_module_;
    ConfigStatGroup*                        current_stat_group_;
    std::map<std::string, ConfigStatistic*> current_shared_stats_;

    std::map<std::string, std::string> program_options_;

    std::string current_key_;
    std::string current_shared_name_;
    RankInfo    rank_;

    bool found_components_ = false; // components must be declared before links

    ComponentId_t findComponentIdByName(const std::string& name, bool& success);
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
    std::string  script_name_;
    Output*      output_;
    Config*      config_;
    ConfigGraph* graph_;
    double       start_time_;
};

} // namespace SST::Core

#endif // SST_CORE_MODEL_JSON_JSONMODEL_H
