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
    SST::ConfigLink const*  link;
    SST::ConfigGraph const* graph;
};

struct StatPair
{
    std::pair<std::string, unsigned long> const& statkey;
    SST::ConfigComponent const*                  comp;
};

struct StatGroupPair
{
    std::pair<const std::string, SST::ConfigStatGroup> const& group;
    std::vector<std::string>                                  vec;
    SST::ConfigGraph const*                                   graph;
};

struct StatGroupParamPair
{
    const std::string  name;
    const SST::Params& stat;
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

    for ( auto const& paramsItr : comp->getParamsLocalKeys() ) {
        j["params"][paramsItr] = comp->params.find<std::string>(paramsItr);
    }

    for ( auto const& paramsItr : comp->getSubscribedGlobalParamSets() ) {
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

    for ( auto const& paramsItr : comp->getParamsLocalKeys() ) {
        j["params"][paramsItr] = comp->params.find<std::string>(paramsItr);
    }

    for ( auto const& paramsItr : comp->getSubscribedGlobalParamSets() ) {
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
    if ( link->no_cut )
        j = json::ordered_json { { "name", link->name }, { "noCut", true } };
    else
        j = json::ordered_json { { "name", link->name } };

    j["left"]["component"]  = graph->findComponent(link->component[0])->getFullName();
    j["left"]["port"]       = link->port[0];
    j["left"]["latency"]    = link->latency_str[0];
    j["right"]["component"] = graph->findComponent(link->component[1])->getFullName();
    j["right"]["port"]      = link->port[1];
    j["right"]["latency"]   = link->latency_str[1];
}

void
to_json(json::ordered_json& j, StatGroupParamPair const& pair)
{
    auto const& outParams = pair.stat;

    j["name"] = pair.name;

    for ( auto const& param : outParams.getKeys() ) {
        j["params"][param] = outParams.find<std::string>(param);
    }
}

void
to_json(json::ordered_json& j, StatGroupPair const& pair)
{
    auto const& grp   = pair.group.second;
    auto const* graph = pair.graph;
    auto        vec   = pair.vec;

    j["name"] = grp.name;

    if ( grp.outputFrequency.getValue() != 0 ) { j["frequency"] = grp.outputFrequency.toStringBestSI(); }

    if ( grp.outputID != 0 ) {
        const SST::ConfigStatOutput& out = graph->getStatOutput(grp.outputID);
        j["output"]["type"]              = out.type;
        if ( !out.params.empty() ) {
            const SST::Params& outParams = out.params;
            for ( auto const& param : vec ) {
                j["output"]["params"][param] = outParams.find<std::string>(param);
            }
        }
    }

    for ( auto& i : grp.statMap ) {
        if ( !i.second.empty() ) { j["statistics"].emplace_back(StatGroupParamPair { i.first, i.second }); }
    }

    for ( SST::ComponentId_t id : grp.components ) {
        const SST::ConfigComponent* comp = graph->findComponent(id);
        j["components"].emplace_back(comp->name);
    }
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
    outputJson["program_options"]["verbose"]                = std::to_string(cfg->verbose());
    outputJson["program_options"]["stop-at"]                = cfg->stop_at();
    outputJson["program_options"]["print-timing-info"]      = cfg->print_timing() ? "true" : "false";
    // Ignore stopAfter for now
    // outputJson["program_options"]["stopAfter"] = cfg->stopAfterSec();
    outputJson["program_options"]["heartbeat-sim-period"]   = cfg->heartbeat_sim_period();
    outputJson["program_options"]["heartbeat-wall-period"]  = std::to_string(cfg->heartbeat_wall_period());
    outputJson["program_options"]["timebase"]               = cfg->timeBase();
    outputJson["program_options"]["partitioner"]            = cfg->partitioner();
    outputJson["program_options"]["timeVortex"]             = cfg->timeVortex();
    outputJson["program_options"]["interthread-links"]      = cfg->interthread_links() ? "true" : "false";
    outputJson["program_options"]["output-prefix-core"]     = cfg->output_core_prefix();
    outputJson["program_options"]["checkpoint-sim-period"]  = cfg->checkpoint_sim_period();
    outputJson["program_options"]["checkpoint-wall-period"] = std::to_string(cfg->checkpoint_wall_period());

    // Put in the global param sets
    for ( const auto& set : getGlobalParamSetNames() ) {
        for ( const auto& kvp : getGlobalParamSet(set) ) {
            if ( kvp.first != "<set_name>" ) outputJson["global_params"][set][kvp.first] = kvp.second;
        }
    }

    // Global statistics
    if ( 0 != graph->getStatLoadLevel() ) {
        outputJson["statistics_options"]["statisticLoadLevel"] = (uint64_t)graph->getStatLoadLevel();
    }

    if ( !graph->getStatOutput().type.empty() ) {
        outputJson["statistics_options"]["statisticOutput"] = graph->getStatOutput().type.c_str();
        const Params& outParams                             = graph->getStatOutput().params;
        if ( !outParams.empty() ) {
            // generate the parameters
            for ( auto const& paramsItr : getParamsLocalKeys(outParams) ) {
                outputJson["statistics_options"]["params"][paramsItr] = outParams.find<std::string>(paramsItr);
            }
        }
    }

    // Generate the stat groups
    if ( !graph->getStatGroups().empty() ) {
        outputJson["statistics_group"];
        for ( auto& grp : graph->getStatGroups() ) {
            auto vec = getParamsLocalKeys(graph->getStatOutput(grp.second.outputID).params);
            outputJson["statistics_group"].emplace_back(StatGroupPair { grp, vec, graph });
        }
    }

    // no components exist in this rank
    if ( const_cast<ConfigComponentMap_t&>(compMap).size() == 0 ) { outputJson["components"]; }

    for ( const auto& compItr : compMap ) {
        outputJson["components"].emplace_back(CompWrapper { compItr, cfg->output_partition() });
    }

    // no links exist in this rank
    if ( const_cast<ConfigLinkMap_t&>(linkMap).size() == 0 ) { outputJson["links"]; }

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
