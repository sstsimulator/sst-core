// -*- c++ -*-

// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_JSON_JSONMODEL_H
#define SST_CORE_MODEL_JSON_JSONMODEL_H

#include "sst_config.h"

#include "sst/core/component.h"
#include "sst/core/config.h"
#include "sst/core/configGraph.h"
#include "sst/core/cputimer.h"
#include "sst/core/factory.h"
#include "sst/core/memuse.h"
#include "sst/core/model/sstmodel.h"
#include "sst/core/output.h"
#include "sst/core/rankInfo.h"
#include "sst/core/simulation.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include "nlohmann/json.hpp"

#include <fstream>
#include <string>
#include <tuple>
#include <vector>

using namespace SST;
using json = nlohmann::json;

namespace SST {
namespace Core {

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

private:
#define SST_MODEL_JSON_NAME_IDX            0
#define SST_MODEL_JSON_COMPONENTID_IDX     1
#define SST_MODEL_JSON_CONFIGCOMPONENT_IDX 2

    typedef std::vector<std::tuple<std::string, ComponentId_t, ConfigComponent*>> CompTuple;
    CompTuple                                                                     configGraphTuple;

    void          recursiveSubcomponent(ConfigComponent* Parent, const nlohmann::basic_json<>& compArray);
    void          discoverProgramOptions(const json& jFile);
    void          discoverComponents(const json& jFile);
    void          discoverLinks(const json& jFile);
    void          discoverGlobalParams(const json& jFile);
    ComponentId_t findComponentIdByName(const std::string& Name);
};

} // namespace Core
} // namespace SST

#endif // SST_CORE_MODEL_JSON_JSONMODEL_H
