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

#include "sst/core/model/cfgoutput/jsonConfigOutput.h"

#include "sst/core/config.h"
#include "sst/core/params.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/util/filesystem.h"

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
            linkRecord["name"]     = linkItr->name_;
            linkRecord["noCut"]    = linkItr->no_cut_;
            linkRecord["nonlocal"] = linkItr->nonlocal_;

            // left link
            linkRecord["left"]["component"] = graph->findComponent(linkItr->component_[0])->getFullName();
            linkRecord["left"]["port"]      = linkItr->port_[0];
            linkRecord["left"]["latency"]   = linkItr->latency_str(0);

            // right link
            if ( linkItr->nonlocal_ ) {
                linkRecord["right"]["rank"]   = linkItr->component_[1];
                linkRecord["right"]["thread"] = linkItr->latency_[1];
            }
            else {
                linkRecord["right"]["component"] = graph->findComponent(linkItr->component_[1])->getFullName();
                linkRecord["right"]["port"]      = linkItr->port_[1];
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
    const auto& comp_map = graph->getComponentMap();
    size_t      count    = 0;
    if ( const_cast<ConfigComponentMap_t&>(comp_map).size() == 0 ) {
        ofs << "\"components\": null,\n";
    }
    else {
        ofs << "\"components\": [\n";
        for ( const auto& comp_itr : comp_map ) {
            count++;
            auto comp_record = nlohmann::ordered_json::object();

            // top-level objects
            comp_record["name"] = comp_itr->name;
            comp_record["type"] = comp_itr->type;

            // statistics options
            if ( comp_itr->enabledAllStats ) {
                comp_record["statistics_options"]["enable_all_statistics"] = true;
            }
            if ( comp_itr->statLoadLevel != STATISTICLOADLEVELUNINITIALIZED ) {
                comp_record["statistics_options"]["statistic_load_level"] = comp_itr->statLoadLevel;
            }

            // params
            for ( auto const& params_itr : comp_itr->getParamsLocalKeys() ) {
                comp_record["params"][params_itr] = comp_itr->params.find<std::string>(params_itr);
            }

            // params_shared_sets
            if ( comp_itr->getSubscribedSharedParamSets().size() > 0 ) {
                auto shared_params_array = nlohmann::ordered_json::array();
                for ( auto const& paramsItr : comp_itr->getSubscribedSharedParamSets() ) {
                    shared_params_array.push_back(paramsItr);
                }
                comp_record["params_shared_sets"] = shared_params_array;
            }

            // statistics
            if ( comp_itr->enabledStatNames.size() > 0 ) {
                auto stats_array = nlohmann::ordered_json::array();
                for ( auto const& pair : comp_itr->enabledStatNames ) {
                    auto stat_record = nlohmann::ordered_json::object();

                    auto* stat = comp_itr->findStatistic(pair.second);
                    if ( stat->shared ) {
                        stat_record["shared"]      = true;
                        stat_record["shared_name"] = stat->name;
                    }
                    stat_record["name"] = pair.first;

                    for ( auto const& param_itr : stat->params.getKeys() ) {
                        stat_record["params"][param_itr] = stat->params.find<std::string>(param_itr);
                    }

                    stats_array.push_back(stat_record);
                }
                comp_record["statistics"] = stats_array;
            }


            // subcomponents
            if ( comp_itr->subComponents.size() > 0 ) {
                auto subcomp_array = nlohmann::ordered_json::array();
                for ( auto const& sub_itr : comp_itr->subComponents ) {
                    auto sub_record = outputSubComponent(sub_itr, ofs);
                    subcomp_array.push_back(sub_record);
                }
                comp_record["subcomponents"] = subcomp_array;
            }

            // partition info
            if ( cfg->output_partition() ) {
                comp_record["partition"]["rank"]   = comp_itr->rank.rank;
                comp_record["partition"]["thread"] = comp_itr->rank.thread;
            }

            // port modules
            if ( comp_itr->port_modules.size() > 0 ) {
                auto port_modules_array = nlohmann::ordered_json::array();
                for ( auto const& pm_itr : comp_itr->port_modules ) {
                    for ( auto const& port_vec_itr : pm_itr.second ) {
                        auto pm_record    = nlohmann::ordered_json::object();
                        pm_record["port"] = pm_itr.first;
                        pm_record["type"] = port_vec_itr.type;
                        if ( port_vec_itr.stat_load_level != STATISTICLOADLEVELUNINITIALIZED ) {
                            pm_record["statistics_options"]["statistic_load_level"]  = port_vec_itr.stat_load_level;
                            pm_record["statistics_options"]["enable_all_statistics"] = true;
                        }
                        for ( auto const& param : port_vec_itr.params.getKeys() ) {
                            pm_record["params"][param] = port_vec_itr.params.find<std::string>(param);
                        }

                        if ( !port_vec_itr.per_stat_configs.empty() ) {
                            auto stats_array = nlohmann::ordered_json::array();
                            for ( auto const& stat_cfg : port_vec_itr.per_stat_configs ) {
                                auto stat_record    = nlohmann::ordered_json::object();
                                stat_record["name"] = stat_cfg.first;
                                for ( auto const& param_itr : stat_cfg.second.getKeys() ) {
                                    stat_record["params"][param_itr] = stat_cfg.second.find<std::string>(param_itr);
                                }
                                stats_array.push_back(stat_record);
                            }
                            pm_record["statistics"] = stats_array;
                        }
                        port_modules_array.push_back(pm_record);
                    }
                }
                comp_record["port_modules"] = port_modules_array;
            }

            ofs << comp_record.dump(2);
            if ( count == const_cast<ConfigComponentMap_t&>(comp_map).size() ) {
                ofs << "\n";
            }
            else {
                ofs << ",\n";
            }
        }
        ofs << "],\n";
    }
}

nlohmann::ordered_json
JSONConfigGraphOutput::outputSubComponent(ConfigComponent* sub, std::ofstream& ofs)
{
    auto record = nlohmann::ordered_json::object();

    record["slot_name"]   = sub->name;
    record["slot_number"] = sub->slot_num;
    record["type"]        = sub->type;

    // general statistics options
    if ( sub->enabledAllStats ) {
        record["statistics_options"]["enable_all_statistics"] = sub->enabledAllStats;
    }
    if ( sub->statLoadLevel != STATISTICLOADLEVELUNINITIALIZED ) {
        record["statistics_options"]["statistic_load_level"] = sub->statLoadLevel;
    }

    // params
    for ( auto const& param : sub->getParamsLocalKeys() ) {
        record["params"][param] = sub->params.find<std::string>(param);
    }

    // params_shared_sets
    if ( sub->getSubscribedSharedParamSets().size() > 0 ) {
        auto shared_params = nlohmann::json::array();
        for ( auto const& param : sub->getSubscribedSharedParamSets() ) {
            shared_params.push_back(param);
        }
        record["params_shared_sets"] = shared_params;
    }

    // statistics
    if ( sub->enabledStatNames.size() > 0 ) {
        auto stats_array = nlohmann::ordered_json::array();
        for ( auto const& pair : sub->enabledStatNames ) {
            auto  stat_record = nlohmann::ordered_json::object();
            auto* stat        = sub->findStatistic(pair.second);
            if ( stat->shared ) {
                stat_record["shared"]      = true;
                stat_record["shared_name"] = stat->name;
            }
            stat_record["name"] = pair.first;

            // stat params
            for ( auto const& param_itr : stat->params.getKeys() ) {
                stat_record["params"][param_itr] = stat->params.find<std::string>(param_itr);
            }

            stats_array.push_back(stat_record);
        }
        record["statistics"] = stats_array;
    }

    // subsubcomponents (recursive)
    if ( sub->subComponents.size() > 0 ) {
        auto subcomp_array = nlohmann::ordered_json::array();

        for ( auto const& sub_ptr : sub->subComponents ) {
            auto sub_record = outputSubComponent(sub_ptr, ofs);
            subcomp_array.push_back(sub_record);
        }
        record["subcomponents"] = subcomp_array;
    }

    // port modules
    if ( sub->port_modules.size() > 0 ) {
        auto port_modules_array = nlohmann::ordered_json::array();
        for ( auto const& pm_itr : sub->port_modules ) {
            for ( auto const& port_vec_itr : pm_itr.second ) {
                auto pm_record    = nlohmann::ordered_json::object();
                pm_record["port"] = pm_itr.first;
                pm_record["type"] = port_vec_itr.type;
                if ( port_vec_itr.stat_load_level != STATISTICLOADLEVELUNINITIALIZED ) {
                    pm_record["statistics_options"]["statistic_load_level"]  = port_vec_itr.stat_load_level;
                    pm_record["statistics_options"]["enable_all_statistics"] = true;
                }
                for ( auto const& param : port_vec_itr.params.getKeys() ) {
                    pm_record["params"][param] = port_vec_itr.params.find<std::string>(param);
                }

                if ( !port_vec_itr.per_stat_configs.empty() ) {
                    auto stats_array = nlohmann::ordered_json::array();
                    for ( auto const& stat_cfg : port_vec_itr.per_stat_configs ) {
                        auto stat_record    = nlohmann::ordered_json::object();
                        stat_record["name"] = stat_cfg.first;
                        for ( auto const& param_itr : stat_cfg.second.getKeys() ) {
                            stat_record["params"][param_itr] = stat_cfg.second.find<std::string>(param_itr);
                        }
                        stats_array.push_back(stat_record);
                    }
                    pm_record["statistics"] = stats_array;
                }
                port_modules_array.push_back(pm_record);
            }
        }
        record["port_modules"] = port_modules_array;
    }

    return record;
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

            auto statsArray = nlohmann::ordered_json::array();
            for ( auto& i : grp.second.statMap ) {
                if ( !i.second.empty() ) {
                    auto statRecord    = nlohmann::ordered_json::object();
                    statRecord["name"] = i.first;
                    for ( auto const& param : i.second.getKeys() ) {
                        statRecord["params"][param] = i.second.find<std::string>(param);
                    }
                    statsArray.push_back(statRecord);
                }
            }
            sgRecord["statistics"] = statsArray;

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
    const auto& set = getSharedParamSetNames();

    auto param_sets = nlohmann::ordered_json::object();

    if ( set.size() > 0 ) {
        ofs << "\"shared_params\":";

        for ( const auto& s : set ) {
            auto record = nlohmann::ordered_json::object();
            for ( const auto& kvp : getSharedParamSet(s) ) {
                if ( kvp.first != "<set_name>" ) {
                    record[kvp.first] = kvp.second;
                }
            }
            param_sets[s] = record;
        }
        ofs << param_sets.dump(2);
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
}
