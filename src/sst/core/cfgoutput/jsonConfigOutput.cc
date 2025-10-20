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
#include "sst/core/simulation_impl.h"
#include "sst/core/util/filesystem.h"

#include "nlohmann/json.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

using namespace SST::Core;
namespace json = ::nlohmann;

JSONConfigGraphOutput::JSONConfigGraphOutput(const char* path) :
    ConfigGraphOutput(path)
{
    pathStr.assign(path);
}

void
JSONConfigGraphOutput::outputLinks(ConfigGraph* graph, std::ofstream& ofs)
{
    const auto& linkMap = graph->getLinkMap();
    size_t      count   = 0;

    if ( const_cast<ConfigLinkMap_t&>(linkMap).size() == 0 ) {
        // no links to output
        ofs << "\"links\": null\n";
    }
    else {
        // output all the links
        ofs << "\"links\": [\n";
        for ( const auto& linkItr : linkMap ) {
            auto linkRecord = nlohmann::ordered_json::object();
            count++;

            // general link key:val pairs
            linkRecord["name"]     = linkItr->name;
            linkRecord["noCut"]    = linkItr->no_cut;
            linkRecord["nonlocal"] = linkItr->nonlocal;

            // left link
            linkRecord["left"]["component"] = graph->findComponent(linkItr->component[0])->getFullName();
            linkRecord["left"]["port"]      = linkItr->port[0];
            linkRecord["left"]["latency"]   = linkItr->latency_str(0);

            // right link
            if ( linkItr->nonlocal ) {
                linkRecord["right"]["rank"]   = linkItr->component[1];
                linkRecord["right"]["thread"] = linkItr->latency[1];
            }
            else {
                linkRecord["right"]["component"] = graph->findComponent(linkItr->component[1])->getFullName();
                linkRecord["right"]["port"]      = linkItr->port[1];
                linkRecord["right"]["latency"]   = linkItr->latency_str(1);
            }

            ofs << linkRecord.dump(2);
            if ( count != const_cast<ConfigLinkMap_t&>(linkMap).size() ) {
                ofs << ",\n";
            }
        }
        ofs << "\n]\n";
    }
}

void
JSONConfigGraphOutput::outputComponents(const Config* cfg, ConfigGraph* graph, std::ofstream& ofs)
{
    const auto& compMap = graph->getComponentMap();
    size_t      count   = 0;
    if ( const_cast<ConfigComponentMap_t&>(compMap).size() == 0 ) {
        ofs << "\"components\": null,\n";
    }
    else {
        ofs << "\"components\": [\n";
        for ( const auto& compItr : compMap ) {
            count++;
            auto compRecord = nlohmann::ordered_json::object();

            // top-level objects
            compRecord["name"] = compItr->name;
            compRecord["type"] = compItr->type;

            // params
            for ( auto const& paramsItr : compItr->getParamsLocalKeys() ) {
                compRecord["params"][paramsItr] = compItr->params.find<std::string>(paramsItr);
            }

            // params_shared_sets
            if ( compItr->getSubscribedSharedParamSets().size() > 0 ) {
                auto paramSharedSetArray = nlohmann::ordered_json::array();
                for ( auto const& paramsItr : compItr->getSubscribedSharedParamSets() ) {
                    paramSharedSetArray.push_back(paramsItr);
                }
                compRecord["params_shared_sets"] = paramSharedSetArray;
            }

            // subcomponents
            if ( compItr->subComponents.size() > 0 ) {
                auto subCompObjArray = nlohmann::ordered_json::array();
                for ( auto const& scItr : compItr->subComponents ) {
                    auto subCompRecord = nlohmann::ordered_json::object();

                    subCompRecord["slot_name"]   = scItr->name;
                    subCompRecord["slot_number"] = scItr->slot_num;
                    subCompRecord["type"]        = scItr->type;

                    // params
                    for ( auto const& subParamsItr : scItr->getParamsLocalKeys() ) {
                        subCompRecord["params"][subParamsItr] = scItr->params.find<std::string>(subParamsItr);
                    }

                    // params_shared_sets
                    if ( scItr->getSubscribedSharedParamSets().size() > 0 ) {
                        auto scParamSharedSetArray = nlohmann::json::array();
                        for ( auto const& paramsItr : scItr->getSubscribedSharedParamSets() ) {
                            scParamSharedSetArray.push_back(paramsItr);
                        }
                        subCompRecord["params_shared_sets"] = scParamSharedSetArray;
                    }

                    // subsubcomponents
                    if ( scItr->subComponents.size() > 0 ) {
                        auto subSubCompObjArray = nlohmann::ordered_json::array();

                        for ( auto const& subScItr : scItr->subComponents ) {
                            auto subSubCompRecord = nlohmann::ordered_json::object();

                            subSubCompRecord["slot_name"]   = subScItr->name;
                            subSubCompRecord["slot_number"] = subScItr->slot_num;
                            subSubCompRecord["type"]        = subScItr->type;

                            // params
                            for ( auto const& subParamsItr : subScItr->getParamsLocalKeys() ) {
                                subSubCompRecord["params"][subParamsItr] =
                                    subScItr->params.find<std::string>(subParamsItr);
                            }

                            // params_shared_sets
                            if ( subScItr->getSubscribedSharedParamSets().size() > 0 ) {
                                auto scParamSharedSetArray = nlohmann::ordered_json::array();
                                for ( auto const& paramsItr : subScItr->getSubscribedSharedParamSets() ) {
                                    scParamSharedSetArray.push_back(paramsItr);
                                }
                                subSubCompRecord["params_shared_sets"] = scParamSharedSetArray;
                            }

                            // statistics
                            if ( subScItr->enabledStatNames.size() > 0 ) {
                                auto scStatObjArray = nlohmann::ordered_json::array();
                                for ( auto const& pair : subScItr->enabledStatNames ) {
                                    auto statRecord = nlohmann::ordered_json::object();

                                    auto* si = subScItr->findStatistic(pair.second);
                                    if ( si->shared ) {
                                        if ( sharedStatMap.find(si->id) == sharedStatMap.end() ) {
                                            std::string name =
                                                "statObj" + std::to_string(sharedStatMap.size()) + "_" + si->name;
                                            sharedStatMap[si->id].assign(name);
                                            statRecord["name"] = name;
                                        }
                                        else {
                                            std::string name   = sharedStatMap.find(si->id)->second;
                                            statRecord["name"] = name;
                                        }
                                    }
                                    else {
                                        auto const& name   = pair.first;
                                        statRecord["name"] = name;
                                    }

                                    for ( auto const& parmItr : si->params.getKeys() ) {
                                        statRecord["params"][parmItr] = si->params.find<std::string>(parmItr);
                                    }

                                    scStatObjArray.push_back(statRecord);
                                }
                                subSubCompRecord["statistics"] = scStatObjArray;
                            }
                            subSubCompObjArray.push_back(subSubCompRecord);
                        }

                        subCompRecord["subcomponents"] = subSubCompObjArray;
                    }

                    // statistics
                    if ( scItr->enabledStatNames.size() > 0 ) {
                        auto scStatObjArray = nlohmann::ordered_json::array();
                        for ( auto const& pair : scItr->enabledStatNames ) {
                            auto statRecord = nlohmann::ordered_json::object();

                            auto* si = scItr->findStatistic(pair.second);
                            if ( si->shared ) {
                                if ( sharedStatMap.find(si->id) == sharedStatMap.end() ) {
                                    std::string name =
                                        "statObj" + std::to_string(sharedStatMap.size()) + "_" + si->name;
                                    sharedStatMap[si->id].assign(name);
                                    statRecord["name"] = name;
                                }
                                else {
                                    std::string name   = sharedStatMap.find(si->id)->second;
                                    statRecord["name"] = name;
                                }
                            }
                            else {
                                auto const& name   = pair.first;
                                statRecord["name"] = name;
                            }

                            for ( auto const& parmItr : si->params.getKeys() ) {
                                statRecord["params"][parmItr] = si->params.find<std::string>(parmItr);
                            }

                            scStatObjArray.push_back(statRecord);
                        }
                        subCompRecord["statistics"] = scStatObjArray;
                    }
                    subCompObjArray.push_back(subCompRecord);
                }
                compRecord["subcomponents"] = subCompObjArray;
            }

            // statistics
            if ( compItr->enabledStatNames.size() > 0 ) {
                auto statObjArray = nlohmann::ordered_json::array();
                for ( auto const& pair : compItr->enabledStatNames ) {
                    auto statRecord = nlohmann::ordered_json::object();

                    auto* si = compItr->findStatistic(pair.second);
                    if ( si->shared ) {
                        if ( sharedStatMap.find(si->id) == sharedStatMap.end() ) {
                            std::string name = "statObj" + std::to_string(sharedStatMap.size()) + "_" + si->name;
                            sharedStatMap[si->id].assign(name);
                            statRecord["name"] = name;
                        }
                        else {
                            std::string name   = sharedStatMap.find(si->id)->second;
                            statRecord["name"] = name;
                        }
                    }
                    else {
                        auto const& name   = pair.first;
                        statRecord["name"] = name;
                    }

                    for ( auto const& parmItr : si->params.getKeys() ) {
                        statRecord["params"][parmItr] = si->params.find<std::string>(parmItr);
                    }

                    statObjArray.push_back(statRecord);
                }
                compRecord["statistics"] = statObjArray;
            }

            // partition info
            if ( cfg->output_partition() ) {
                compRecord["partition"]["rank"]   = compItr->rank.rank;
                compRecord["partition"]["thread"] = compItr->rank.thread;
            }

            ofs << compRecord.dump(2);
            if ( count == const_cast<ConfigComponentMap_t&>(compMap).size() ) {
                ofs << "\n";
            }
            else {
                ofs << ",\n";
            }
        }
        ofs << "],\n";
    }
}

void
JSONConfigGraphOutput::outputStatisticsGroups(ConfigGraph* graph, std::ofstream& ofs)
{
    if ( !graph->getStatGroups().empty() ) {
        ofs << "\"statistics_group\": [\n";
        size_t count = 0;
        for ( auto& grp : graph->getStatGroups() ) {
            count++;
            auto sgRecord = nlohmann::ordered_json::object();
            auto vec      = getParamsLocalKeys(graph->getStatOutput(grp.second.outputID).params);

            // top-level values
            sgRecord["name"] = grp.second.name;

            if ( grp.second.outputFrequency.getValue() != 0 ) {
                sgRecord["frequency"] = grp.second.outputFrequency.toStringBestSI();
            }

            // output
            if ( grp.second.outputID != 0 ) {
                const SST::ConfigStatOutput& out = graph->getStatOutput(grp.second.outputID);
                sgRecord["output"]["type"]       = out.type;
                if ( !out.params.empty() ) {
                    const SST::Params& outParams = out.params;
                    for ( auto const& param : vec ) {
                        sgRecord["output"]["params"][param] = outParams.find<std::string>(param);
                    }
                }
            }

            for ( auto& i : grp.second.statMap ) {
                if ( !i.second.empty() ) {
                    auto statRecord    = nlohmann::ordered_json::object();
                    statRecord["name"] = i.first;
                    for ( auto const& param : i.second.getKeys() ) {
                        statRecord["params"][param] = i.second.find<std::string>(param);
                    }
                    sgRecord["statistics"] = statRecord;
                }
            }

            if ( grp.second.components.size() > 0 ) {
                auto compArray = nlohmann::ordered_json::array();
                for ( SST::ComponentId_t id : grp.second.components ) {
                    const SST::ConfigComponent* comp = graph->findComponent(id);
                    compArray.push_back(comp->name);
                }
                sgRecord["components"] = compArray;
            }

            ofs << sgRecord.dump(2);

            if ( count == graph->getStatGroups().size() ) {
                ofs << "\n],\n";
            }
            else {
                ofs << ",\n";
            }
        }
    }
    else {
        ofs << "\"statistics_group\": null,\n";
    }
}

void
JSONConfigGraphOutput::outputStatisticsOptions(ConfigGraph* graph, std::ofstream& ofs)
{
    auto record = nlohmann::ordered_json::object();

    ofs << "\"statistics_options\":";
    if ( 0 != graph->getStatLoadLevel() ) {
        record["statisticLoadLevel"] = (uint64_t)graph->getStatLoadLevel();
    }

    if ( !graph->getStatOutput().type.empty() ) {
        record["statisticOutput"] = graph->getStatOutput().type;
        const Params& outParams   = graph->getStatOutput().params;
        if ( !outParams.empty() ) {
            for ( auto const& paramsItr : getParamsLocalKeys(outParams) ) {
                record["params"][paramsItr] = outParams.find<std::string>(paramsItr);
            }
        }
        ofs << record.dump(2);
    }
    else {
        ofs << record.dump(2);
    }
    ofs << ",\n";
}

void
JSONConfigGraphOutput::outputSharedParams(std::ofstream& ofs)
{
    size_t      count = 0;
    const auto& set   = getSharedParamSetNames();

    if ( set.size() > 0 ) {
        ofs << "\"shared_params\":";

        for ( const auto& s : set ) {
            count++;
            auto record = nlohmann::ordered_json::object();
            ofs << "\"" << s << "\":\n";
            for ( const auto& kvp : getSharedParamSet(s) ) {
                if ( kvp.first != "<set_name>" ) {
                    record[kvp.first] = kvp.second;
                }
            }
            ofs << record.dump(2);
            if ( count == set.size() ) {
                ofs << "\n},\n";
            }
            else {
                ofs << "\n}\n";
            }
        }
        ofs << ",\n";
    }
}

void
JSONConfigGraphOutput::outputProgramOptions(const Config* cfg, std::ofstream& ofs)
{
    auto record = nlohmann::ordered_json::object();

    ofs << "\"program_options\":";
    record["verbose"]                = std::to_string(cfg->verbose());
    record["stop-at"]                = cfg->stop_at();
    record["print-timing-info"]      = std::to_string(cfg->print_timing());
    record["timing-info-json"]       = cfg->timing_json();
    // Ignore stopAfter for now
    // outputJson["program_options"]["stopAfter"] = cfg->stopAfterSec();
    record["heartbeat-sim-period"]   = cfg->heartbeat_sim_period();
    record["heartbeat-wall-period"]  = std::to_string(cfg->heartbeat_wall_period());
    record["timebase"]               = cfg->timeBase();
    record["partitioner"]            = cfg->partitioner();
    record["timeVortex"]             = cfg->timeVortex();
    record["interthread-links"]      = cfg->interthread_links() ? "true" : "false";
    record["output-prefix-core"]     = cfg->output_core_prefix();
    record["checkpoint-sim-period"]  = cfg->checkpoint_sim_period();
    record["checkpoint-wall-period"] = std::to_string(cfg->checkpoint_wall_period());

    ofs << record.dump(2);
    ofs << ",\n";
}

void
JSONConfigGraphOutput::generate(const Config* cfg, ConfigGraph* graph)
{
    if ( nullptr == outputFile ) {
        throw ConfigGraphOutputException("Output file is not open for writing");
    }

    // open an output stream separate from the C-style file pointer
    SST::Util::Filesystem filesystem = Simulation_impl::filesystem;
    std::ofstream         ofs        = filesystem.ofstream(pathStr, std::ofstream::out);
    if ( !ofs.is_open() ) {
        throw ConfigGraphOutputException("Streaming output file is not open for writing");
    }

    ofs << "{\n";

    // output each top-level section
    outputProgramOptions(cfg, ofs);
    outputSharedParams(ofs);
    outputStatisticsOptions(graph, ofs);
    outputComponents(cfg, graph, ofs);
    outputStatisticsGroups(graph, ofs);
    outputLinks(graph, ofs);

    // close the output stream
    ofs << "}\n";
    ofs.flush();
    ofs.close();

    sharedStatMap.clear();
}
