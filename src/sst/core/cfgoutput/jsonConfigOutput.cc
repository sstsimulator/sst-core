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
#include "sst/core/nlohmann/json.hpp"

#include <map>
#include <sstream>

using namespace SST::Core;
namespace json = ::nlohmann;

JSONConfigGraphOutput::JSONConfigGraphOutput(const char* path) : ConfigGraphOutput(path) {}

namespace {
struct CompLinkPair {
    SST::ConfigComponent const* comp;
    SST::ConfigLinkMap_t const& linkMap;
};

struct LinkConfPair {
    SST::ConfigLink const& link;
    SST::ConfigComponentMap_t const& compMap;
};

struct StatPair {
    std::pair<std::string, unsigned long> const& statkey;
    SST::ConfigComponent const* comp;
};

struct ParamsPair {
    std::string const& paramItr;
    SST::ConfigStatistic const* si;
};

void
to_json(json::json& j, ParamsPair const& pp) {
    j = json::json { { "name", pp.paramItr }, { "value", pp.si->params.find<std::string>(pp.paramItr) } };
}

void
to_json(json::json& j, StatPair const& sp) {
    auto const& name = sp.statkey.first;
    j = json::json { { "name", name } };

    auto* si = sp.comp->findStatistic(sp.statkey.second);
    for (auto const& parmItr : si->params.getKeys()) {
        j["params"].push_back(ParamsPair { parmItr, si });
    }
}

void
to_json(json::json& j, CompLinkPair const& pair) {
    auto& comp = pair.comp;
    auto& linkMap = pair.linkMap;
    j = json::json { { "name", comp->name }, { "type", comp->type } };

    for (auto const& i : comp->links) {
        auto const& link = linkMap[i];
        const auto port = (link.component[0] == comp->id) ? 0 : 1;
        j["ports"].push_back({ { "name", link.port[port] } });
    }

    for (auto const& scItr : comp->subComponents) {
        j["subcomponents"].push_back(CompLinkPair { scItr, linkMap });
    }

    for (auto const& pair : comp->enabledStatNames) {
        j["statistics"].push_back(StatPair { pair, comp });
    }

    for (auto const& paramsItr : comp->params.getKeys()) {
        j["params"].push_back({ { "name", paramsItr }, { "value", comp->params.find<std::string>(paramsItr) } });
    }
}

void
to_json(json::json& j, LinkConfPair const& pair) {
    auto const& link = pair.link;
    auto const& compMap = pair.compMap;

    // These accesses into compMap are not checked
    j = json::json { { "name", link.name } };
    j["left"] = compMap[link.component[0]]->name;
    j["leftPort"] = link.port[0];
    j["right"] = compMap[link.component[1]]->name;
    j["rightPort"] = link.port[1];
    j["latency"] = link.latency_str[(link.latency[0] <= link.latency[1]) ? 0 : 1];
}

} // namespace

void
JSONConfigGraphOutput::generate(const Config* UNUSED(cfg), ConfigGraph* graph) {
    if (nullptr == outputFile) {
        throw ConfigGraphOutputException("Output file is not open for writing");
    }

    auto compMap = graph->getComponentMap();
    auto linkMap = graph->getLinkMap();

    json::json outputJson;
    for (auto compItr : compMap) {
        outputJson["compenents"].emplace_back(CompLinkPair { compItr, linkMap });
    }

    for (auto linkItr : linkMap) {
        outputJson["links"].push_back(LinkConfPair { linkItr, compMap });
    }

    std::stringstream ss;
    ss << std::setw(2) << outputJson << std::endl;

    // Avoid lifetime issues, I don't think it's a big deal in this case due to
    // lifetime extension, but this temp is the careful way.
    std::string outputString = ss.str();
    fprintf(outputFile, "%s", outputString.c_str());
}
