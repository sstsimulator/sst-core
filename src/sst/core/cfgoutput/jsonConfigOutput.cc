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
//

#include "sst_config.h"

#include "sst/core/cfgoutput/jsonConfigOutput.h"

#include "sst/core/config.h"
#include "sst/core/configGraph.h"
#include "sst/core/configGraphOutput.h"
#include "sst/core/params.h"

#include "nlohmann/json.hpp"

#include <map>
#include <sstream>

using namespace SST::Core;
namespace json = ::nlohmann;

JSONConfigGraphOutput::JSONConfigGraphOutput(const char* path) : ConfigGraphOutput(path) {}

namespace {
struct CompWrapper
{
    SST::ConfigComponent const* comp;
    bool                        output_parition_info;
};

struct SubCompWrapper
{
    SST::ConfigComponent const* comp;
};

struct LinkConfPair
{
    SST::ConfigLink const&  link;
    SST::ConfigGraph const* graph;
};

struct StatPair
{
    std::pair<std::string, unsigned long> const& statkey;
    SST::ConfigComponent const*                  comp;
};

void
to_json(json::ordered_json& j, StatPair const& sp)
{
    auto const& name = sp.statkey.first;
    j                = json::ordered_json { { "name", name } };

    auto* si = sp.comp->findStatistic(sp.statkey.second);
    for ( auto const& parmItr : si->params.getKeys() ) {
        j["params"][parmItr] = si->params.find<std::string>(parmItr);
    }
}

void
to_json(json::ordered_json& j, SubCompWrapper const& comp_wrapper)
{
    auto& comp = comp_wrapper.comp;
    j = json::ordered_json { { "slot_name", comp->name }, { "slot_number", comp->slot_num }, { "type", comp->type } };

    for ( auto const& paramsItr : comp->params.getLocalKeys() ) {
        j["params"][paramsItr] = comp->params.find<std::string>(paramsItr);
    }

    for ( auto const& paramsItr : comp->params.getSubscribedGlobalParamSets() ) {
        j["params_global_sets"].push_back(paramsItr);
    }

    for ( auto const& scItr : comp->subComponents ) {
        j["subcomponents"].push_back(SubCompWrapper { scItr });
    }

    for ( auto const& pair : comp->enabledStatNames ) {
        j["statistics"].push_back(StatPair { pair, comp });
    }
}

void
to_json(json::ordered_json& j, CompWrapper const& comp_wrapper)
{
    auto& comp = comp_wrapper.comp;
    j          = json::ordered_json { { "name", comp->name }, { "type", comp->type } };

    for ( auto const& paramsItr : comp->params.getLocalKeys() ) {
        j["params"][paramsItr] = comp->params.find<std::string>(paramsItr);
    }

    for ( auto const& paramsItr : comp->params.getSubscribedGlobalParamSets() ) {
        j["params_global_sets"].push_back(paramsItr);
    }

    for ( auto const& scItr : comp->subComponents ) {
        j["subcomponents"].push_back(SubCompWrapper { scItr });
    }

    for ( auto const& pair : comp->enabledStatNames ) {
        j["statistics"].push_back(StatPair { pair, comp });
    }

    if ( comp_wrapper.output_parition_info ) {
        j["partition"]["rank"]   = comp->rank.rank;
        j["partition"]["thread"] = comp->rank.thread;
    }
}

void
to_json(json::ordered_json& j, LinkConfPair const& pair)
{
    auto const& link  = pair.link;
    auto const* graph = pair.graph;

    // These accesses into compMap are not checked
    if ( link.no_cut )
        j = json::ordered_json { { "name", link.name }, { "noCut", true } };
    else
        j = json::ordered_json { { "name", link.name } };

    j["left"]["component"]  = graph->findComponent(link.component[0])->getFullName();
    j["left"]["port"]       = link.port[0];
    j["left"]["latency"]    = link.latency_str[0];
    j["right"]["component"] = graph->findComponent(link.component[1])->getFullName();
    j["right"]["port"]      = link.port[1];
    j["right"]["latency"]   = link.latency_str[1];
}

} // namespace

void
JSONConfigGraphOutput::generate(const Config* cfg, ConfigGraph* graph)
{
    if ( nullptr == outputFile ) { throw ConfigGraphOutputException("Output file is not open for writing"); }

    const auto& compMap = graph->getComponentMap();
    const auto& linkMap = graph->getLinkMap();

    json::ordered_json outputJson;

    // Put in the program options
    outputJson["program_options"]["verbose"]     = std::to_string(cfg->verbose);
    outputJson["program_options"]["stopAtCycle"] = cfg->stopAtCycle;
    if ( cfg->print_timing ) outputJson["program_options"]["print-timing-info"] = "";
    // Ignore stopAfter for now
    // outputJson["program_options"]["stopAfter"] = cfg->stopAfterSec;
    outputJson["program_options"]["heartbeat-period"] = cfg->heartbeatPeriod;
    outputJson["program_options"]["timebase"]         = cfg->timeBase;
    outputJson["program_options"]["partitioner"]      = cfg->partitioner;
    outputJson["program_options"]["timeVortex"]       = cfg->timeVortex;
    if ( cfg->inter_thread_links ) outputJson["program_options"]["inter-thread-links"] = "";
    outputJson["program_options"]["output-prefix-core"] = cfg->output_core_prefix;
    outputJson["program_options"]["model-options"]      = cfg->model_options;

    // Put in the global param sets
    for ( const auto& set : Params::getGlobalParamSetNames() ) {
        for ( const auto& kvp : Params::getGlobalParamSet(set) ) {
            if ( kvp.first != "<set_name>" ) outputJson["global_params"][set][kvp.first] = kvp.second;
        }
    }

    for ( const auto& compItr : compMap ) {
        outputJson["components"].emplace_back(CompWrapper { compItr, cfg->output_partition });
    }

    for ( const auto& linkItr : linkMap ) {
        outputJson["links"].push_back(LinkConfPair { linkItr, graph });
    }

    std::stringstream ss;
    ss << std::setw(2) << outputJson << std::endl;

    // Avoid lifetime issues, I don't think it's a big deal in this case due to
    // lifetime extension, but this temp is the careful way.
    std::string outputString = ss.str();
    fprintf(outputFile, "%s", outputString.c_str());
}
