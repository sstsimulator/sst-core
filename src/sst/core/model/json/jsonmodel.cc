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

#include "sst_config.h"

#include "sst/core/model/json/jsonmodel.h"

#include <cstdint>
#include <string>

DISABLE_WARN_STRICT_ALIASING

using namespace SST;
using namespace SST::Core;

ComponentId_t
SSTConfigSaxHandler::findComponentIdByName(const std::string& Name, bool& success)
{
    ComponentId_t    Id   = -1;
    ConfigComponent* Comp = nullptr;

    if ( Name.length() == 0 ) {
        errorStr = "Error: Name given to findComponentIdByName is null";
        success  = false;
        return Id;
    }

    // first, find the component pointer from the name
    Comp = graph->findComponentByName(Name);
    if ( Comp == nullptr ) {
        errorStr = "Error finding component ID by name: " + Name;
        success  = false;
        return Id;
    }

    success = true;
    return Comp->id;
}

std::string
SSTConfigSaxHandler::getCurrentPath() const
{
    std::string path;
    for ( size_t i = 0; i < path_stack.size(); ++i ) {
        if ( i > 0 ) path += ".";
        path += path_stack[i];
    }
    return path;
}

const std::map<std::string, std::string>
SSTConfigSaxHandler::getProgramOptions()
{
    return ProgramOptions;
}

void
SSTConfigSaxHandler::processValue(const json& value)
{
    std::string path = getCurrentPath();

    if ( current_state == PROGRAM_OPTIONS ) {
        if ( value.get<std::string>() == "false" ) {
            ProgramOptions[current_key] = "0";
        }
        else if ( value.get<std::string>() == "true" ) {
            ProgramOptions[current_key] = "1";
        }
        else {
            ProgramOptions[current_key] = value.get<std::string>();
        }
    }
    else if ( current_state == STATISTICS_OPTIONS ) {
        if ( current_key == "statisticLoadLevel" && value.is_number_integer() ) {
            graph->setStatisticLoadLevel(value.get<int>());
        }
        else if ( current_key == "statisticOutput" && value.is_string() ) {
            graph->setStatisticOutput(value.get<std::string>());
        }
        else if ( path.find("statistics_options.params") != std::string::npos && value.is_string() ) {
            graph->addStatisticOutputParameter(current_key, value.get<std::string>());
        }
    }
    else if ( current_state == SHARED_PARAMS ) {
        graph->addSharedParam(current_shared_name, current_key, value.get<std::string>());
    }
    else if ( current_state == COMPONENTS ) {
        if ( current_key == "rank" && value.is_number_integer() ) {
            current_comp_rank = value.get<unsigned int>();
        }
        else if ( current_key == "thread" && value.is_number_integer() ) {
            RankInfo Rank(current_comp_rank, value.get<unsigned int>());
            current_component->setRank(Rank);
        }
        else if ( path.find("params_shared_sets") != std::string::npos && value.is_string() ) {
            current_component->addSharedParamSet(value.get<std::string>());
        }
        else if ( current_key == "slot_number" && in_comp_subcomp && value.is_number_integer() ) {
            subcomp_slot = value.get<int>();
        }
        else if ( current_key == "params_shared_sets" && in_comp_subcomp && value.is_string() ) {
            current_sub_component->addSharedParamSet(value.get<std::string>());
        }
    }
    else if ( current_state == LINKS ) {
        if ( current_key == "noCut" && value.is_boolean() ) {
            no_cut = value.get<bool>();
        }
    }
}

bool
SSTConfigSaxHandler::null()
{
    return true;
}

bool
SSTConfigSaxHandler::boolean(bool val)
{
    json j_val = val;
    processValue(j_val);
    return true;
}

bool
SSTConfigSaxHandler::number_integer(json::number_integer_t val)
{
    json j_val = val;
    processValue(j_val);
    return true;
}

bool
SSTConfigSaxHandler::number_unsigned(json::number_unsigned_t val)
{
    json j_val = val;
    processValue(j_val);
    return true;
}

bool
SSTConfigSaxHandler::number_float(json::number_float_t val, [[maybe_unused]] const std::string& str)
{
    json j_val = val;
    processValue(j_val);
    return true;
}

bool
SSTConfigSaxHandler::binary([[maybe_unused]] json::binary_t& val)
{
    return true;
}

bool
SSTConfigSaxHandler::string(std::string& val)
{
    json j_val = val;
    processValue(j_val);

    // handle specific string assignment based upon context
    if ( current_state == COMPONENTS ) {
        if ( current_key == "name" && !in_comp_stats ) {
            current_comp_name = val;
        }
        else if ( current_key == "type" && !in_comp_stats && !in_comp_subcomp ) {
            current_comp_id   = graph->addComponent(current_comp_name, val);
            current_component = graph->findComponent(current_comp_id);
        }
        else if ( in_comp_params && !in_comp_subcomp ) {
            current_component->addParameter(current_key, val, false);
        }
        else if ( current_key == "name" && in_comp_stats ) {
            current_comp_stat_name = val;
        }
        else if ( in_comp_stats && in_comp_stats_params ) {
            current_stat_params.insert(current_key, val);
        }
        else if ( current_key == "slot_name" ) {
            current_subcomp_name = val;
        }
        else if ( current_key == "type" && in_comp_subcomp ) {
            current_subcomp_type = val;
            current_sub_component =
                Parents.top()->addSubComponent(current_subcomp_name, current_subcomp_type, subcomp_slot);
        }
        else if ( in_comp_subcomp_params ) {
            current_sub_component->addParameter(current_key, val, false);
        }
    }
    else if ( current_state == LINKS ) {
        if ( current_key == "name" ) {
            link_name = val;
        }
        else if ( in_left_link ) {
            if ( current_key == "component" ) {
                left_comp = val;
            }
            else if ( current_key == "port" ) {
                left_port = val;
            }
            else if ( current_key == "latency" ) {
                left_latency = val;
            }
        }
        else if ( in_right_link ) {
            if ( current_key == "component" ) {
                right_comp = val;
            }
            else if ( current_key == "port" ) {
                right_port = val;
            }
            else if ( current_key == "latency" ) {
                right_latency = val;
            }
        }
    }
    else if ( current_state == STATISTICS_GROUP ) {
        if ( in_grp_stats_output ) {
            if ( current_key == "type" ) {
                auto& statOuts = graph->getStatOutputs();
                statOuts.emplace_back(ConfigStatOutput(val));
                current_stat_group->setOutput(statOuts.size() - 1);
            }
            else if ( in_grp_stats_output_params ) {
                auto& statOuts = graph->getStatOutputs();
                statOuts.back().addParameter(current_key, val);
            }
        }
        else if ( in_grp_stats_def ) {
            if ( current_key == "name" ) {
                current_grp_stat_name = val;
            }
            else if ( in_grp_stats_def_params ) {
                current_stat_params.insert(current_key, val);
            }
        }
        else if ( in_grp_stats_comps ) {
            bool success = false;
            current_stat_group->addComponent(findComponentIdByName(val, success));
            if ( !success ) {
                return false;
            }
        }
        else if ( current_key == "name" ) {
            current_stat_group = graph->getStatGroup(val);
            if ( current_stat_group == nullptr ) {
                errorStr = "Error creating statistics group from: " + val;
                return false;
            }
        }
        else if ( current_key == "frequency" ) {
            if ( !current_stat_group->setFrequency(val) ) {
                errorStr = "Error setting frequency for statistics group: " + val;
                return false;
            }
        }
    }

    return true;
}

bool
SSTConfigSaxHandler::start_object(std::size_t elements)
{
    if ( elements == 0 ) {
        return true;
    }

    context_stack.push(json::value_t::object);

    if ( !path_stack.empty() ) {
        std::string current_section = path_stack[path_stack.size() - 1];

        if ( current_section == "program_options" ) {
            current_state = PROGRAM_OPTIONS;
        }
        else if ( current_section == "shared_params" ) {
            current_state = SHARED_PARAMS;
            if ( !in_shared_params_object && path_stack.size() == 2 ) {
                current_shared_name     = path_stack[1];
                in_shared_params_object = true;
            }
        }
        else if ( current_section == "statistics_options" ) {
            current_state = STATISTICS_OPTIONS;
        }
        else if ( current_state == STATISTICS_GROUP ) {
            if ( current_section == "output" ) {
                in_grp_stats_output = true;
            }
            else if ( current_section == "params" && in_grp_stats_output ) {
                in_grp_stats_output_params = true;
            }
            else if ( current_section == "params" && in_grp_stats_def ) {
                in_grp_stats_def_params = true;
            }
        }
        else if ( current_state == COMPONENTS ) {
            if ( current_section == "params" && !in_comp_stats && !in_comp_subcomp ) {
                in_comp_params = true;
            }
            else if ( current_section == "params" && in_comp_stats ) {
                in_comp_stats_params = true;
            }
            else if ( current_section == "params" && in_comp_subcomp ) {
                in_comp_subcomp_params = true;
            }
        }
        else if ( current_state == LINKS ) {
            if ( current_section == "left" ) {
                in_left_link = true;
            }
            else if ( current_section == "right" ) {
                in_right_link = true;
            }
        }
    }

    return true;
}

bool
SSTConfigSaxHandler::end_object()
{
    if ( !context_stack.empty() ) {
        context_stack.pop();
    }

    // handle each object completion
    if ( current_state == PROGRAM_OPTIONS ) {
        current_state = ROOT;
    }
    else if ( current_state == STATISTICS_OPTIONS && current_key != "params" ) {
        current_state = ROOT;
    }
    else if ( current_state == SHARED_PARAMS && in_shared_params_object && path_stack.size() == 2 ) {
        in_shared_params_object = false;
    }
    else if ( current_state == STATISTICS_GROUP && in_grp_stats_output && in_grp_stats_output_params ) {
        in_grp_stats_output_params = false;
        current_stat_params.clear();
    }
    else if ( current_state == STATISTICS_GROUP && in_grp_stats_output ) {
        in_grp_stats_output = false;
    }
    else if ( current_state == STATISTICS_GROUP && in_grp_stats_def && in_grp_stats_def_params ) {
        in_grp_stats_def_params = false;
        current_stat_params.clear();
    }
    else if ( current_state == STATISTICS_GROUP && in_grp_stats_def ) {
        current_stat_group->addStatistic(current_grp_stat_name, current_stat_params);
        current_stat_params.clear();
        bool        verified = false;
        std::string reason;
        std::tie(verified, reason) = current_stat_group->verifyStatsAndComponents(graph);
        if ( !verified ) {
            errorStr = "Error verifying statistics and components: " + reason;
            return false;
        }
    }
    else if ( current_state == COMPONENTS && in_comp_params ) {
        in_comp_params = false;
    }
    else if ( current_state == COMPONENTS && in_comp_stats && in_comp_stats_params ) {
        current_component->enableStatistic(current_comp_stat_name, current_stat_params);
        in_comp_stats_params = false;
        current_stat_params.clear();
    }
    else if ( current_state == COMPONENTS && in_comp_stats_params && in_comp_subcomp ) {
        current_sub_component->enableStatistic(current_comp_stat_name, current_stat_params);
        in_comp_stats_params = false;
        current_stat_params.clear();
    }
    else if ( current_state == COMPONENTS && in_comp_subcomp && in_comp_subcomp_params ) {
        in_comp_subcomp_params = false;
    }
    else if ( current_state == LINKS ) {
        if ( in_left_link ) {
            in_left_link = false;
        }
        else if ( in_right_link ) {
            in_right_link = false;
        }
        else {
            LinkId_t link_id = graph->createLink(link_name.c_str(), nullptr);
            if ( no_cut ) graph->setLinkNoCut(link_id);

            // left side
            ComponentId_t CompID  = -1;
            bool          success = false;
            CompID                = findComponentIdByName(left_comp, success);
            if ( !success ) {
                return false;
            }
            graph->addLink(CompID, link_id, left_port.c_str(), left_latency.c_str());

            // right side
            success = false;
            CompID  = findComponentIdByName(right_comp, success);
            if ( !success ) {
                return false;
            }
            graph->addLink(CompID, link_id, right_port.c_str(), right_latency.c_str());
            no_cut = false;
        }
    }

    if ( !path_stack.empty() ) {
        path_stack.pop_back();
    }

    return true;
}

bool
SSTConfigSaxHandler::start_array([[maybe_unused]] std::size_t elements)
{
    context_stack.push(json::value_t::array);
    array_depth++;

    if ( current_state == ROOT ) {
        // top-level array elements
        if ( current_key == "components" ) {
            current_state   = COMPONENTS;
            foundComponents = true;
        }
        else if ( current_key == "shared_params" ) {
            current_state = SHARED_PARAMS;
        }
        else if ( current_key == "statistics_group" ) {
            current_state = STATISTICS_GROUP;
            if ( !foundComponents ) {
                errorStr = "Encountered statistics_group before components; components must be loaded before "
                           "statistics_groups";
                return false;
            }
        }
        else if ( current_key == "links" ) {
            current_state = LINKS;
        }
    }
    else if ( current_state == STATISTICS_GROUP ) {
        // array elements within statistics_group
        if ( current_key == "statistics" ) {
            in_grp_stats_def = true;
        }
        else if ( current_key == "components" ) {
            in_grp_stats_comps = true;
        }
    }
    else if ( current_state == COMPONENTS ) {
        // array elements within components
        if ( current_key == "statistics" ) {
            in_comp_stats = true;
        }
        else if ( current_key == "subcomponents" && in_comp_subcomp ) {
            in_comp_subsubcomp = true;
            Parents.push(current_sub_component);
        }
        else if ( current_key == "subcomponents" ) {
            in_comp_subcomp = true;
            Parents.push(current_component);
        }
    }

    return true;
}

bool
SSTConfigSaxHandler::end_array()
{
    if ( current_state == SHARED_PARAMS ) {
        current_state = ROOT;
    }
    else if ( current_state == STATISTICS_GROUP && in_grp_stats_comps ) {
        in_grp_stats_comps = false;
    }
    else if ( current_state == STATISTICS_GROUP && in_grp_stats_def ) {
        in_grp_stats_def = false;
    }
    else if ( current_state == STATISTICS_GROUP ) {
        current_state = ROOT;
    }
    else if ( current_state == COMPONENTS && in_comp_stats ) {
        in_comp_stats = false;
    }
    else if ( current_state == COMPONENTS && in_comp_subsubcomp ) {
        in_comp_subsubcomp = false;
        Parents.pop();
    }
    else if ( current_state == COMPONENTS && in_comp_subcomp ) {
        in_comp_subcomp = false;
        Parents.pop();
    }
    else if ( current_state == COMPONENTS ) {
        current_state = ROOT;
    }
    else if ( current_state == LINKS ) {
        current_state = ROOT;
    }

    if ( !context_stack.empty() ) {
        context_stack.pop();
    }
    array_depth--;
    return true;
}

bool
SSTConfigSaxHandler::key(std::string& val)
{
    current_key = val;
    path_stack.push_back(val);
    return true;
}

void
SSTConfigSaxHandler::setConfigGraph(ConfigGraph* g)
{
    graph = g;
}

bool
SSTConfigSaxHandler::parse_error(std::size_t position, const std::string& last_token, const json::exception& ex)
{
    errorPos = position;
    errorStr = last_token + " : " + ex.what();
    return false;
}

SSTJSONModelDefinition::SSTJSONModelDefinition(
    const std::string& script_file, int verbosity, Config* configObj, double start_time) :
    SSTModelDescription(configObj),
    scriptName(script_file),
    output(nullptr),
    config(configObj),
    graph(nullptr),
    nextComponentId(0),
    start_time(start_time)
{
    output = new Output("SSTJSONModel: ", verbosity, 0, SST::Output::STDOUT);

    graph = new ConfigGraph();
    if ( !graph ) {
        output->fatal(CALL_INFO, 1, "Could not create graph object in JSON loader.\n");
    }

    output->verbose(CALL_INFO, 2, 0, "SST loading a JSON model from script: %s\n", script_file.c_str());
}

SSTJSONModelDefinition::~SSTJSONModelDefinition()
{
    delete output;
}

ConfigGraph*
SSTJSONModelDefinition::createConfigGraph()
{
    // open the file
    std::ifstream ifs(scriptName.c_str());
    if ( !ifs.is_open() ) {
        output->fatal(CALL_INFO, 1, "Error opening JSON model from script: %s\n", scriptName.c_str());
        return nullptr;
    }

    // create the SAX handler
    SSTConfigSaxHandler handler;
    handler.setConfigGraph(graph);
    bool result = json::sax_parse(ifs, &handler);
    if ( !result ) {
        output->fatal(CALL_INFO, 1, "Error parsing json file at position %lu: (%s)\n", handler.errorPos,
            handler.errorStr.c_str());
        return nullptr;
    }

    // set the program options
    auto ProgramOptions = handler.getProgramOptions();
    for ( const auto& [key, value] : ProgramOptions ) {
        setOptionFromModel(key, value);
    }

    // close the file
    ifs.close();

    return graph;
}
