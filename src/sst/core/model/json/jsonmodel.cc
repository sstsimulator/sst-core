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
SSTConfigSaxHandler::findComponentIdByName(const std::string& name, bool& success)
{
    ComponentId_t    id   = UNSET_COMPONENT_ID;
    ConfigComponent* comp = nullptr;

    if ( name.length() == 0 ) {
        error_str_ = "Error: Name given to findComponentIdByName is null";
        success    = false;
        return id;
    }

    // first, find the component pointer from the name
    comp = graph_->findComponentByName(name);
    if ( comp == nullptr ) {
        error_str_ = "Error: Unable to locate component by name: " + name;
        success    = false;
        return id;
    }

    success = true;
    return comp->id;
}

const std::map<std::string, std::string>
SSTConfigSaxHandler::getProgramOptions()
{
    return program_options_;
}

bool
SSTConfigSaxHandler::null()
{
    // null passed instead of an object or array
    switch ( current_state_ ) {
    case State::ProgramOptions_key:
    case State::SharedParams_key:
    case State::SharedParams_object_key:
    case State::StatOptions_key:
    case State::StatOptions_Params_key:
    case State::StatGroupsArray_key:
    case State::StatGroup_Output_key:
    case State::StatGroup_Output_Params_key:
    case State::StatGroup_StatArray_key:
    case State::StatGroup_Stat_Params_key:
    case State::CompArray_key:
    case State::Comp_Params_key:
    case State::Comp_StatOptions_key:
    case State::Comp_StatOptions_Params_key:
    case State::Comp_StatArray_key:
    case State::Comp_Stat_Params_key:
    case State::Comp_SharedParamsArray_key:
    case State::Comp_Partition_key:
    case State::PortMod_Params_key:
    case State::PortMod_StatOptions_key:
    case State::PortMod_StatOptions_Params_key:
    case State::PortMod_StatArray_key:
    case State::PortMod_Stat_Params_key:
    case State::PortMod_SharedParamsArray_key:
    case State::SubComp_Params_key:
    case State::SubComp_StatOptions_key:
    case State::SubComp_StatOptions_Params_key:
    case State::SubComp_StatArray_key:
    case State::SubComp_Stat_Params_key:
    case State::SubComp_SharedParamsArray_key:
    case State::LinkArray_key:
    case State::Link_Left_key:
    case State::Link_Right_key:
        current_state_ = ParentState[(int)current_state_];
        break;
    case State::SubCompArray_key:
    case State::PortModArray_key:
        if ( shadow_component_stack_.size() == 1 ) {
            current_state_ = State::Comp;
        }
        else {
            current_state_ = State::SubComp;
        }
        break;
    default:
        error_str_ = "Parsed a null in an unexpected parser state " + StateString[(int)current_state_];
        return false;
    }
    return true;
}

bool
SSTConfigSaxHandler::boolean(bool val)
{
    switch ( current_state_ ) {
    case State::Link:
        if ( current_key_ == "noCut" || current_key_ == "no_cut" ) {
            shadow_link_.nocut = val;
        }
        else if ( current_key_ == "nonlocal" ) {
            shadow_link_.nonlocal = val;
        }
        else {
            error_str_ = "Unexpected key/boolean value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in Link '" + shadow_link_.name + "'";
            return false;
        }
        break;
    case State::Comp_StatOptions:
    case State::SubComp_StatOptions:
    case State::PortMod_StatOptions:
        if ( current_key_ == "enable_all_stats" || current_key_ == "enable_all_statistics" ) {
            if ( val ) {
                shadow_statistic_.name = STATALLFLAG;
            }
        }
        else {
            error_str_ = "Unexpected key/boolean value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in 'statistics' belonging to '" + shadow_component_stack_.front().name +
                         "' or one of its subcomponents";
            return false;
        }
        break;
    case State::Comp_Stat:
    case State::SubComp_Stat:
    case State::PortMod_Stat:
        if ( current_key_ == "shared" ) {
            shadow_statistic_.shared = val;
        }
        else {
            error_str_ = "Unexpected key/boolean value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in 'statistics' object in component tree '" + shadow_component_stack_[0].name;
            return false;
        }
        break;
    default:
        error_str_ = "Parsed a boolean '" + std::to_string(val) + "'in an unexpected parser state " +
                     StateString[(int)current_state_] + ". Last key was: " + current_key_;
        return false;
    }
    return true;
}

bool
SSTConfigSaxHandler::number_integer(json::number_integer_t val)
{
    switch ( current_state_ ) {
    case State::StatOptions:
        if ( current_key_ == "statisticLoadLevel" || current_key_ == "statistic_load_level" ) {
            graph_->setStatisticLoadLevel(val);
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in statistic_options object.";
            return false;
        }
        break;
    case State::Comp_Partition:
        if ( current_key_ == "rank" ) {
            rank_.rank = val;
        }
        else if ( current_key_ == "thread" ) {
            rank_.thread = val;
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in partition object.";
            return false;
        }
        break;
    case State::SubComp:
        if ( current_key_ == "slot_number" ) {
            shadow_component_stack_.back().slot_number = val;
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in 'subcomponents' object in component tree '" + shadow_component_stack_[0].name;
            return false;
        }
        break;
    case State::Comp_StatOptions:
    case State::SubComp_StatOptions:
        if ( current_key_ == "statistic_load_level" ) {
            shadow_component_stack_.back().comp->setStatisticLoadLevel(val);
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in 'statistic_options' object in component tree '" + shadow_component_stack_[0].name;
            return false;
        }
        break;
    case State::PortMod_StatOptions:
        if ( current_key_ == "statistic_load_level" ) {
            shadow_port_module_.stat_load_level = val;
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in 'subcomponents' object in component tree '" + shadow_component_stack_[0].name;
            return false;
        }
        break;
    case State::Link_Right:
        if ( current_key_ == "rank" ) {
            shadow_link_.rank = val;
        }
        else if ( current_key_ == "thread" ) {
            shadow_link_.thread = val;
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in right side of link '" + shadow_link_.name + "'.";
            return false;
        }
        break;
    default:
        error_str_ = "Parsed an integer '" + std::to_string(val) + "' in an unexpected parser state " +
                     StateString[(int)current_state_] + ". Last key was: " + current_key_;
        return false;
    }
    return true;
}

bool
SSTConfigSaxHandler::number_unsigned(json::number_unsigned_t val)
{
    switch ( current_state_ ) {
    case State::StatOptions:
        if ( current_key_ == "statisticLoadLevel" || current_key_ == "statistic_load_level" ) {
            graph_->setStatisticLoadLevel(val);
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in statistic_options object.";
            return false;
        }
        break;
    case State::Comp_Partition:
        if ( current_key_ == "rank" ) {
            rank_.rank = val;
        }
        else if ( current_key_ == "thread" ) {
            rank_.thread = val;
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in partition object.";
            return false;
        }
        break;
    case State::SubComp:
        if ( current_key_ == "slot_number" ) {
            shadow_component_stack_.back().slot_number = val;
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in 'subcomponents' object in component tree '" + shadow_component_stack_[0].name;
            return false;
        }
        break;
    case State::Comp_StatOptions:
    case State::SubComp_StatOptions:
        if ( current_key_ == "statistic_load_level" ) {
            shadow_component_stack_.back().comp->setStatisticLoadLevel(val);
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in 'statistic_options' object in component tree '" + shadow_component_stack_[0].name;
            return false;
        }
        break;
    case State::PortMod_StatOptions:
        if ( current_key_ == "statistic_load_level" ) {
            shadow_port_module_.stat_load_level = val;
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in 'subcomponents' object in component tree '" + shadow_component_stack_[0].name;
            return false;
        }
        break;
    case State::Link_Right:
        if ( current_key_ == "rank" ) {
            shadow_link_.rank = val;
        }
        else if ( current_key_ == "thread" ) {
            shadow_link_.thread = val;
        }
        else {
            error_str_ = "Unexpected key/integer value pair: '" + current_key_ + "'/'" + std::to_string(val) +
                         "' in right side of link '" + shadow_link_.name + "'.";
            return false;
        }
        break;
    default:
        error_str_ = "Parsed an integer '" + std::to_string(val) + "' in an unexpected parser state " +
                     StateString[(int)current_state_] + ". Last key was: " + current_key_;
        return false;
    }
    return true;
}

bool
SSTConfigSaxHandler::number_float([[maybe_unused]] json::number_float_t val, [[maybe_unused]] const std::string& str)
{
    error_str_ = "Parsed a float '" + str + "'in an unexpected parser state " + StateString[(int)current_state_];
    return false;
}

bool
SSTConfigSaxHandler::binary([[maybe_unused]] json::binary_t& val)
{
    error_str_ = "Parsed a binary value in an unexpected parser state " + StateString[(int)current_state_];
    return false;
}

bool
SSTConfigSaxHandler::string(std::string& val)
{
    switch ( current_state_ ) {
    case State::ProgramOptions:
        if ( val == "false" ) {
            program_options_[current_key_] = "0";
        }
        else if ( val == "true" ) {
            program_options_[current_key_] = "1";
        }
        else {
            program_options_[current_key_] = val;
        }
        break;
    case State::StatOptions:
        if ( current_key_ == "statisticOutput" || current_key_ == "statistic_output" ) {
            graph_->setStatisticOutput(val);
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in 'statistics_options' object. Expected key 'statistic_output'.";
            return false;
        }
        break;
    case State::StatOptions_Params:
        graph_->addStatisticOutputParameter(current_key_, val);
        break;
    case State::Comp:
        if ( current_key_ == "name" ) {
            shadow_component_stack_.back().name = val;
            if ( shadow_component_stack_.back().comp == nullptr && shadow_component_stack_.back().type != "" ) {
                constructComponent();
                if ( shadow_component_stack_.back().comp == nullptr ) return false;
            }
        }
        else if ( current_key_ == "type" ) {
            shadow_component_stack_.back().type = val;
            if ( shadow_component_stack_.back().comp == nullptr && shadow_component_stack_.back().name != "" ) {
                constructComponent();
                if ( shadow_component_stack_.back().comp == nullptr ) return false;
            }
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in 'component' object with name (if already parsed): '" +
                         shadow_component_stack_.back().name + "'. Expected key 'name' or 'type'.";
            return false;
        }
        break;
    case State::SubComp:
        if ( current_key_ == "slot_name" ) {
            shadow_component_stack_.back().name = val;
        }
        else if ( current_key_ == "type" ) {
            shadow_component_stack_.back().type = val;
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in 'subcomponent' object in component '" + shadow_component_stack_[0].name +
                         "'. Expected key 'slot_name' or 'type'.";
            return false;
        }
        break;
    case State::PortMod:
        if ( current_key_ == "port" ) {
            shadow_port_module_.port = val;
        }
        else if ( current_key_ == "type" ) {
            shadow_port_module_.type = val;
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in 'port_module' object in component '" + shadow_component_stack_[0].name +
                         "'. Expected key 'port' or 'type'.";
            return false;
        }
        break;
    case State::Comp_Params:
    case State::SubComp_Params:
        shadow_component_stack_.back().comp->addParameter(current_key_, val, false);
        break;
    case State::PortMod_Params:
        shadow_port_module_.pm->addParameter(current_key_, val);
        break;
    case State::Comp_SharedParamsArray:
    case State::SubComp_SharedParamsArray:
        shadow_component_stack_.back().comp->addSharedParamSet(val);
        break;
    case State::PortMod_SharedParamsArray:
        shadow_port_module_.shared_param_sets.push_back(val);
        break;
    case State::Comp_Stat:
    case State::SubComp_Stat:
    case State::PortMod_Stat:
        if ( current_key_ == "name" ) {
            shadow_statistic_.name = val;
        }
        else if ( current_key_ == "shared_name" ) {
            shadow_statistic_.shared_name = val;
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in 'statistics' object in component tree '" + shadow_component_stack_[0].name +
                         "'. Expected key 'name' or 'shared_name'.";
            return false;
        }
        break;
    case State::StatGroup_Stat:
        if ( current_key_ == "name" ) {
            shadow_statistic_.name = val;
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in 'statistics_group' object. Expected key 'name'.";
        }
        break;
    case State::Comp_Stat_Params:
    case State::SubComp_Stat_Params:
    case State::PortMod_Stat_Params:
    case State::StatGroup_Stat_Params:
    case State::Comp_StatOptions_Params:
    case State::SubComp_StatOptions_Params:
    case State::PortMod_StatOptions_Params:
        shadow_statistic_.params.insert(current_key_, val);
        break;
    case State::Link:
        if ( current_key_ == "name" ) {
            shadow_link_.name = val;
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in 'link' object. Link name is (may be blank if not parsed yet): '" + shadow_link_.name +
                         "'. Expected key 'name'.";
            return false;
        }
        break;
    case State::Link_Left:
        if ( current_key_ == "component" ) {
            bool success;
            shadow_link_.leftcomp = findComponentIdByName(val, success);
            if ( !success ) {
                error_str_ = "Error: Unable to locate component ID for component '" + val + "' in left side of link '" +
                             shadow_link_.name +
                             "'. Ensure components are declared prior to link instantiation in your input file.";
                return false;
            }
        }
        else if ( current_key_ == "port" ) {
            shadow_link_.leftport = val;
        }
        else if ( current_key_ == "latency" ) {
            shadow_link_.leftlat = val;
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in left side of link named '" + shadow_link_.name +
                         "'. Expected key 'port', 'latency', or 'component'.";
            return false;
        }
        break;
    case State::Link_Right:
        if ( current_key_ == "component" ) {
            bool success;
            shadow_link_.rightcomp = findComponentIdByName(val, success);
            if ( !success ) {
                error_str_ = "Error: Unable to locate component ID for component '" + val +
                             "' in right side of link '" + shadow_link_.name +
                             "'. Ensure components are declared prior to link instantiation in your input file.";
                return false;
            }
        }
        else if ( current_key_ == "port" ) {
            shadow_link_.rightport = val;
        }
        else if ( current_key_ == "latency" ) {
            shadow_link_.rightlat = val;
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in right side of link named '" + shadow_link_.name +
                         "'. Expected key 'port', 'latency', 'component'.";
            return false;
        }
        break;
    case State::StatGroup:
        if ( current_key_ == "name" ) {
            current_stat_group_ = graph_->getStatGroup(val);
            if ( current_stat_group_ == nullptr ) {
                error_str_ = "Error creating statistics group from name: " + val;
                return false;
            }
        }
        else if ( current_key_ == "frequency" ) {
            current_stat_group_->setFrequency(val);
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in statistics_group. Expected key 'name' or 'frequency'.";
            return false;
        }
        break;
    case State::StatGroup_Output:
        if ( current_key_ == "type" ) {
            auto& stat_outputs = graph_->getStatOutputs();
            stat_outputs.emplace_back(ConfigStatOutput(val));
            current_stat_group_->setOutput(stat_outputs.size() - 1);
        }
        else {
            error_str_ = "Error: Unexpected key/string value pair: '" + current_key_ + "'/'" + val +
                         "' in statistics_group output object. Expected key 'type'.";
            return false;
        }
        break;
    case State::StatGroup_Output_Params:
        graph_->getStatOutputs().back().addParameter(current_key_, val);
        break;
    case State::StatGroup_CompArray:
    {
        bool success = false;
        current_stat_group_->addComponent(findComponentIdByName(val, success));
        if ( !success ) {
            error_str_ =
                "Error: Unable to locate component ID for component '" + val +
                "' in statistics group component list for group " + current_stat_group_->name +
                ". Ensure components are declared prior to adding them to a statistic group in your input file.";
            return false;
        }
        break;
    }
    case State::SharedParams_object:
        graph_->addSharedParam(current_shared_name_, current_key_, val);
        break;
    default:
        error_str_ = "Error: Parser encountered a string token in state '" + StateString[(int)current_state_] +
                     "'. This case is not handled and may be an error in the JSON file. Key/value pair is '" +
                     current_key_ + "'/'" + val + "'.";
        return false;
    }
    return true;
}

// Passes number of elements in object (-1 if unknown)
// Generally each state transitions to next and prepares/clears any data structures that will be used by the object
bool
SSTConfigSaxHandler::start_object([[maybe_unused]] std::size_t elements)
{
    switch ( current_state_ ) {
    case State::Entry:                          // Begin parsing -> Root
    case State::ProgramOptions_key:             // Parse program options -> ProgramOptions
    case State::SharedParams_key:               // Parse object containing sets of shared params -> SharedParams_object
    case State::SharedParams_object_key:        // Parse shared params set -> SharedParams_object
    case State::StatOptions_key:                // Parse statistic options
    case State::StatOptions_Params_key:         // Parse params -> StatOptions_Params
    case State::StatGroup_Output_key:           // Parse output config -> StatGroup_Output
    case State::StatGroup_Output_Params_key:    // Parse params -> StatGroup_Output_Params
    case State::StatGroup_StatArray:            // Parse statistic -> StatGroup_Stat
    case State::StatGroup_Stat_Params_key:      // Parse params -> STATGRP_OBJ_STATOPT_PARAMS
    case State::Comp_StatOptions_Params_key:    // Parse params
    case State::Comp_Stat_Params_key:           // Parse params
    case State::PortMod_Params_key:             // Parse params
    case State::PortMod_Stat_Params_key:        // Parse params
    case State::PortMod_StatOptions_Params_key: // Parse params
    case State::SubComp_StatOptions_Params_key: // Parse params
    case State::SubComp_Stat_Params_key:        // Parse params
    case State::Link_Left_key:                  // Parse link left
    case State::Link_Right_key:                 // Parse link right
        break;
    case State::StatGroupsArray: // Parse a statistic group -> StatGroup
        current_stat_group_ = nullptr;
        break;
    case State::CompArray:    // Parse a component
    case State::SubCompArray: // Parse a subcomponent
        shadow_component_stack_.emplace_back();
        break;
    case State::Comp_Partition_key: // Parse partition
        rank_.rank   = -1;
        rank_.thread = -1;
        break;
    case State::PortModArray: // Parse port module
        shadow_port_module_.reset();
        break;
    case State::Comp_StatArray:    // Parse statstic object
    case State::SubComp_StatArray: // Parse statstic object
    case State::PortMod_StatArray:
    case State::PortMod_StatOptions_key: // Parse statistic options (params)
        shadow_statistic_.reset();
        break;
    // These component/subcomponent sections require that the component/subcomponent be constructed
    case State::Comp_StatOptions_key:    // Parse statistic options (params)
    case State::SubComp_StatOptions_key: // Parse statistic options (params)
        shadow_statistic_.reset();
        // fall-thru
    case State::Comp_Params_key:    // Parse params
    case State::SubComp_Params_key: // Parse params
        // Assert that component/subcomponent is constructed
        if ( shadow_component_stack_.back().comp == nullptr ) {
            error_str_ =
                "Error: Encountered new (sub)component section before (sub)component was constructed. Ensure "
                "that subcomponent required keys (name, type, slot_name,  etc.) are listed first in the object "
                "before other keys. Component tree name is (if already parsed) '" +
                shadow_component_stack_[0].name + "'.";
            return false;
        }
        break;
    case State::LinkArray: // Parse a link
        shadow_link_.reset();
        break;
    default:
        error_str_ = "Error: Parser encountered a '{' token in state '" + StateString[(int)current_state_] +
                     "'. This case is not handled and may be an error in the JSON file. Last key was '" + current_key_ +
                     "'.";
        return false;
    }
    current_state_ = NextStateObjOrArray[(int)current_state_];
    return true;
}

bool
SSTConfigSaxHandler::end_object()
{
    switch ( current_state_ ) {
    case State::Root:
    case State::ProgramOptions:
    case State::SharedParams:
    case State::SharedParams_object:
    case State::StatOptions:
    case State::StatOptions_Params:
    case State::StatGroup:
    case State::StatGroup_Output:
    case State::StatGroup_Output_Params:
    case State::StatGroup_Stat_Params:
    case State::Comp_Params:
    case State::SubComp_Params:
    case State::PortMod_Params:
    case State::Comp_Stat_Params:
    case State::SubComp_Stat_Params:
    case State::PortMod_Stat_Params:
    case State::Link_Left:
    case State::Link_Right:
        break;
    case State::Comp_Partition:
        shadow_component_stack_.back().comp->setRank(rank_);
        break;
    case State::StatGroup_Stat:
    {
        // Add stat to group
        current_stat_group_->addStatistic(shadow_statistic_.name, shadow_statistic_.params);
        bool        verified = false;
        std::string reason;
        std::tie(verified, reason) = current_stat_group_->verifyStatsAndComponents(graph_);
        if ( !verified ) {
            error_str_ = "Error verifying statistics and components: " + reason;
            return false;
        }
        break;
    }
    case State::Comp:
        shadow_component_stack_.pop_back();
        current_shared_stats_.clear();
        break;
    case State::Comp_Stat:
    case State::SubComp_Stat:
    case State::Comp_StatOptions:
    case State::SubComp_StatOptions:
        if ( shadow_statistic_.name != "" ) { // Generally name="" if in StatOptions but didn't enable_all_stats
            if ( shadow_statistic_.shared ) {
                auto cs_iter = current_shared_stats_.find(shadow_statistic_.shared_name);
                if ( cs_iter == current_shared_stats_.end() ) {
                    // Have not yet created tis shared statistic object
                    ConfigStatistic* cs = shadow_component_stack_.back().comp->createStatistic();
                    if ( cs == nullptr ) {
                        error_str_ = "Error: Failed to create shared statistic '" + shadow_statistic_.shared_name +
                                     "' on '" + shadow_component_stack_.back().name + "'.";
                        return false;
                    }
                    cs->params.insert(shadow_statistic_.params);
                    cs->shared = true;
                    cs->name   = shadow_statistic_.shared_name;
                    current_shared_stats_.insert(std::make_pair(shadow_statistic_.shared_name, cs));
                    cs_iter = current_shared_stats_.find(shadow_statistic_.shared_name);
                }
                // Shared statistic that maps to existing statistic object
                if ( !shadow_component_stack_.back().comp->reuseStatistic(
                         shadow_statistic_.name, cs_iter->second->id) ) {
                    error_str_ = "Error: Attempting to link statistic '" + shadow_statistic_.name + "' to '" +
                                 shadow_statistic_.shared_name + "' in component '" +
                                 shadow_component_stack_.front().name + "' but unable to complete the linking.";
                    return false;
                }
            }
            else {
                // Regular (unshared) statistic
                shadow_component_stack_.back().comp->enableStatistic(shadow_statistic_.name, shadow_statistic_.params);
            }
        }
        shadow_statistic_.reset();
        break;
    case State::PortMod_Stat:
    case State::PortMod_StatOptions:
        // Portmodules don't do shared stats. An empty name is treated as an not-enabled statistic.
        if ( shadow_statistic_.name == STATALLFLAG ) {
            shadow_port_module_.pm->enableAllStatistics(shadow_statistic_.params);
        }
        else if ( shadow_statistic_.name != "" ) {
            shadow_port_module_.pm->enableStatistic(shadow_statistic_.name, shadow_statistic_.params);
        }
        break;
    case State::PortMod:
    {
        if ( shadow_port_module_.pm == nullptr ) {
            constructPortModule();
        }
        if ( shadow_port_module_.stat_load_level != STATISTICLOADLEVELUNINITIALIZED ) {
            shadow_port_module_.pm->setStatisticLoadLevel(shadow_port_module_.stat_load_level);
        }
        break;
    }
    case State::SubComp:
        if ( shadow_component_stack_.back().comp == nullptr ) {
            constructSubComponent();
            if ( !shadow_component_stack_.back().comp ) return false;
        }
        shadow_component_stack_.pop_back();
        break;
    case State::Link:
    {
        LinkId_t id = graph_->createLink(shadow_link_.name.c_str(), nullptr);
        if ( shadow_link_.nocut ) graph_->setLinkNoCut(id);
        graph_->addLink(shadow_link_.leftcomp, id, shadow_link_.leftport.c_str(), shadow_link_.leftlat.c_str());

        if ( shadow_link_.nonlocal ) {
            graph_->addNonLocalLink(id, shadow_link_.rank, shadow_link_.thread);
        }
        else {
            graph_->addLink(shadow_link_.rightcomp, id, shadow_link_.rightport.c_str(), shadow_link_.rightlat.c_str());
        }
        shadow_link_.reset();
        break;
    }
    default:
        error_str_ = "Error: Parser encountered a '}' token in state '" + StateString[(int)current_state_] +
                     "'. This case is not handled and may be an error in the JSON file. Last key was '" + current_key_ +
                     "'.";
        return false;
    }
    current_state_ = ParentState[(int)current_state_];
    return true;
}

bool
SSTConfigSaxHandler::start_array([[maybe_unused]] std::size_t elements)
{
    switch ( current_state_ ) {
    case State::CompArray_key:
        found_components_ = true;
        break;
    case State::SubCompArray_key:
        if ( shadow_component_stack_.back().comp == nullptr ) {
            error_str_ = "Error: Encountered (sub)component's subcomponents array before the 'type' and 'name' keys in "
                         "component '" +
                         shadow_component_stack_.back().name + "'";
            return false;
        }
        break;
    case State::LinkArray_key:
        if ( !found_components_ ) {
            error_str_ =
                "Error: Encountered 'links' section before 'components' section. Put the 'links' section after "
                "'components' in your input file";
            return false;
        }
        break;
    case State::PortModArray_key:
    case State::Comp_StatArray_key:
    case State::SubComp_StatArray_key:
    case State::PortMod_StatArray_key:
    case State::Comp_SharedParamsArray_key:
    case State::SubComp_SharedParamsArray_key:
    case State::PortMod_SharedParamsArray_key:
    case State::StatGroupsArray_key:
    case State::StatGroup_StatArray_key:
    case State::StatGroup_CompArray_key:
        break;
    default:
        error_str_ = "Error: Parser encountered a '[' token in state '" + StateString[(int)current_state_] +
                     "'. This case is not handled and may be an error in the JSON file. Last key was '" + current_key_ +
                     "'.";
        return false;
    }
    current_state_ = NextStateObjOrArray[(int)current_state_];
    return true;
}

bool
SSTConfigSaxHandler::end_array()
{
    switch ( current_state_ ) {
    case State::CompArray:
    case State::LinkArray:
    case State::Comp_StatArray:
    case State::SubComp_StatArray:
    case State::PortMod_StatArray:
    case State::Comp_SharedParamsArray:
    case State::SubComp_SharedParamsArray:
    case State::PortMod_SharedParamsArray:
    case State::StatGroupsArray:
    case State::StatGroup_StatArray:
    case State::StatGroup_CompArray:
    case State::StatGroup_Output_Params:
    case State::StatGroup_Stat_Params:
    case State::Comp_Params:
    case State::SubComp_Params:
    case State::PortMod_Params:
    case State::Comp_Stat_Params:
    case State::SubComp_Stat_Params:
    case State::PortMod_Stat_Params:
    case State::Comp_Partition:
        current_state_ = ParentState[(int)current_state_];
        break;
    case State::SubCompArray:
    case State::PortModArray:
        if ( shadow_component_stack_.size() == 1 ) {
            current_state_ = State::Comp;
        }
        else {
            current_state_ = State::SubComp;
        }
        break;
    default:
        error_str_ = "Error: Parser encountered a ']' token in state '" + StateString[(int)current_state_] +
                     "'. This case is not handled and may be an error in the JSON file. Last key was '" + current_key_ +
                     "'.";
        return false;
    }
    return true;
}

bool
SSTConfigSaxHandler::key(std::string& val)
{
    switch ( current_state_ ) {
    case State::ProgramOptions:
    case State::SharedParams_object:
    case State::StatOptions_Params:
    case State::Link_Left:
    case State::Link_Right:
    case State::StatGroup_Output_Params:
    case State::StatGroup_Stat_Params:
    case State::Comp_Params:
    case State::Comp_StatOptions_Params:
    case State::Comp_Stat_Params:
    case State::Comp_Partition:
    case State::SubComp_Params:
    case State::SubComp_StatOptions_Params:
    case State::SubComp_Stat_Params:
    case State::PortMod_Params:
    case State::PortMod_StatOptions_Params:
    case State::PortMod_Stat_Params:
        break;
    case State::Root:
        if ( val == "program_options" ) {
            current_state_ = State::ProgramOptions_key;
        }
        else if ( val == "shared_params" ) {
            current_state_ = State::SharedParams_key;
        }
        else if ( val == "statistics_options" ) {
            current_state_ = State::StatOptions_key;
        }
        else if ( val == "statistics_group" ) {
            current_state_ = State::StatGroupsArray_key;
        }
        else if ( val == "components" ) {
            current_state_ = State::CompArray_key;
        }
        else if ( val == "links" ) {
            current_state_ = State::LinkArray_key;
        }
        else { // Error: Unexpected key
            error_str_ = "Error: Encountered unexpected key '" + val +
                         "' in state Root. Expected keys are 'program_options', 'shared_params', 'statistics_options', "
                         "'statistics_group', 'components', or 'links'.\n";
            return false;
        }
        break;
    case State::SharedParams:
        current_shared_name_ = val;
        current_state_       = State::SharedParams_object_key;
        break;
    case State::StatOptions:
    case State::Comp_StatOptions:
    case State::SubComp_StatOptions:
    case State::PortMod_StatOptions:
    case State::Comp_Stat:
    case State::SubComp_Stat:
    case State::PortMod_Stat:
    case State::StatGroup_Output:
    case State::StatGroup_Stat:
        if ( val == "params" ) {
            current_state_ = NextStateParams[(int)current_state_];
        }
        break;
    case State::StatGroup:
        if ( !current_stat_group_ && val != "name" ) {
            error_str_ =
                "Error: First key/value pair in a statistics_group object must be the group name. Expected key "
                "'name' and got '" +
                val + "'";
            return false;
        }
        if ( val == "output" ) {
            current_state_ = State::StatGroup_Output_key;
        }
        else if ( val == "statistics" ) {
            current_state_ = State::StatGroup_StatArray_key;
        }
        else if ( val == "components" ) {
            current_state_ = State::StatGroup_CompArray_key;
        }
        break;
    case State::Comp:
    {
        bool assert_existance = true;
        if ( val == "params" ) {
            current_state_ = State::Comp_Params_key;
        }
        else if ( val == "statistics_options" ) {
            current_state_ = State::Comp_StatOptions_key;
        }
        else if ( val == "statistics" ) {
            current_state_ = State::Comp_StatArray_key;
        }
        else if ( val == "subcomponents" ) {
            current_state_ = State::SubCompArray_key;
        }
        else if ( val == "params_shared_sets" ) {
            current_state_ = State::Comp_SharedParamsArray_key;
        }
        else if ( val == "partition" ) {
            current_state_ = State::Comp_Partition_key;
        }
        else if ( val == "port_modules" ) {
            current_state_ = State::PortModArray_key;
        }
        else {
            assert_existance = false;
        }
        if ( assert_existance ) { // To enter next state, component MUST already exist. Component is built as soon as
                                  // type & name are encountered
            if ( shadow_component_stack_.back().comp == nullptr ) {
                error_str_ = "Error: Encountered key '" + val + "' before the 'type' and 'name' keys in component '" +
                             shadow_component_stack_.back().name + "'.";
                return false;
            }
        }
        break;
    }
    case State::PortMod:
    {
        bool assert_existance = true;
        if ( val == "params" ) {
            current_state_ = State::PortMod_Params_key;
        }
        else if ( val == "statistics_options" ) {
            current_state_ = State::PortMod_StatOptions_key;
        }
        else if ( val == "statistics" ) {
            current_state_ = State::PortMod_StatArray_key;
        }
        else if ( val == "params_shared_sets" ) {
            current_state_ = State::PortMod_SharedParamsArray_key;
        }
        else {
            assert_existance = false;
        }
        if ( assert_existance ) {
            if ( shadow_port_module_.pm == nullptr ) {
                constructPortModule();
            }
        }
        break;
    }
    case State::SubComp:
    {
        bool assert_existance = true;
        if ( val == "params" ) {
            current_state_ = State::SubComp_Params_key;
        }
        else if ( val == "statistics_options" ) {
            current_state_ = State::SubComp_StatOptions_key;
        }
        else if ( val == "statistics" ) {
            current_state_ = State::SubComp_StatArray_key;
        }
        else if ( val == "subcomponents" ) {
            current_state_ = State::SubCompArray_key;
        }
        else if ( val == "params_shared_sets" ) {
            current_state_ = State::SubComp_SharedParamsArray_key;
        }
        else if ( val == "port_modules" ) {
            current_state_ = State::PortModArray_key;
        }
        else {
            assert_existance = false;
        }
        if ( assert_existance ) { // To enter next state, component MUST already exist. Component is built as soon as
                                  // type & name are encountered
            if ( shadow_component_stack_.back().comp == nullptr ) {
                constructSubComponent();
                if ( shadow_component_stack_.back().comp == nullptr ) {
                    return false; // error_str_ provided by constructSubComponent function
                }
            }
        }
        break;
    }
    case State::Link:
        if ( val == "left" ) {
            current_state_ = State::Link_Left_key;
        }
        else if ( val == "right" ) {
            current_state_ = State::Link_Right_key;
        }
        break;
    default:
        error_str_ = "Error: Parser encountered a key token in state '" + StateString[(int)current_state_] +
                     "'. This case is not handled and may be an error in the JSON file. Key is '" + val +
                     "' and last valid key was '" + current_key_ + "'.";
        return false;
    }
    current_key_ = val;
    return true;
}

// Internal function used to construct a subcomponent
// All calls to this function are guarded or occur in a state that ensures that:
//  (a) shadow_component_stack_.back().comp == nullptr
//  (b) shadow_component_stack_.size() > 1
void
SSTConfigSaxHandler::constructSubComponent()
{
    if ( shadow_component_stack_.back().name == "" || shadow_component_stack_.back().type == "" ) {
        error_str_ = "Error: Unable to construct subcomponent in component '" + shadow_component_stack_[0].name +
                     "'. Missing name or type ('" + shadow_component_stack_.back().name + "', '" +
                     shadow_component_stack_.back().type +
                     "'). The usual cause of this error is failing to place name and type at the beginning of a "
                     "component object.\n";
        return;
    }
    shadow_component_stack_.back().comp =
        shadow_component_stack_[shadow_component_stack_.size() - 2].comp->addSubComponent(
            shadow_component_stack_.back().name, shadow_component_stack_.back().type,
            shadow_component_stack_.back().slot_number);
}

void
SSTConfigSaxHandler::constructPortModule()
{
    if ( shadow_port_module_.port == "" || shadow_port_module_.type == "" ) {
        error_str_ = "Error: Unable to construct port_module in component tree '" + shadow_component_stack_[0].name +
                     "'. Missing port or type ('" + shadow_port_module_.port + "', '" + shadow_port_module_.type +
                     "'). The usual cause of this error is failing to place port and type at the beginning of a "
                     "port_module object.\n";
        return;
    }
    size_t pm_id = (shadow_component_stack_.back().comp)
                       ->addPortModule(shadow_port_module_.port, shadow_port_module_.type, shadow_port_module_.params);
    shadow_port_module_.pm =
        &((shadow_component_stack_.back().comp)->port_modules.find(shadow_port_module_.port)->second[pm_id]);
}

// Internal function used to construct a component
// All calls to this function are guarded or occur in a state that ensures that:
// (a) shadow_component_stack_.back().comp == nullptr
// (b) shadow_component_stack_.size() == 1
void
SSTConfigSaxHandler::constructComponent()
{
    if ( shadow_component_stack_.back().name == "" || shadow_component_stack_.back().type == "" ) {
        error_str_ = "Error: Unable to construct component. Missing name or type ('" +
                     shadow_component_stack_.back().name + "', '" + shadow_component_stack_.back().type +
                     "'). The usual cause of this error is failing to place name and type at the beginning of a "
                     "component object.\n";
        return;
    }
    ComponentId_t id = graph_->addComponent(shadow_component_stack_.back().name, shadow_component_stack_.back().type);
    shadow_component_stack_.back().comp = graph_->findComponent(id);
}

void
SSTConfigSaxHandler::setConfigGraph(ConfigGraph* graph)
{
    graph_ = graph;
}

bool
SSTConfigSaxHandler::parse_error(std::size_t position, const std::string& last_token, const json::exception& ex)
{
    error_pos_ = position;
    error_str_ = last_token + " : " + ex.what();
    return false;
}

SSTJSONModelDefinition::SSTJSONModelDefinition(
    const std::string& script_file, int verbosity, Config* config_obj, double start_time) :
    SSTModelDescription(config_obj),
    script_name_(script_file),
    output_(nullptr),
    config_(config_obj),
    graph_(nullptr),
    start_time_(start_time)
{
    output_ = new Output("SSTJSONModel: ", verbosity, 0, SST::Output::STDOUT);

    graph_ = new ConfigGraph();
    if ( !graph_ ) {
        output_->fatal(CALL_INFO, 1, "Could not create graph object in JSON loader.\n");
    }

    output_->verbose(CALL_INFO, 2, 0, "SST loading a JSON model from script: %s\n", script_file.c_str());
}

SSTJSONModelDefinition::~SSTJSONModelDefinition()
{
    delete output_;
}

ConfigGraph*
SSTJSONModelDefinition::createConfigGraph()
{
    // open the file
    std::ifstream ifs(script_name_.c_str());
    if ( !ifs.is_open() ) {
        output_->fatal(CALL_INFO, 1, "Error opening JSON model from script: %s\n", script_name_.c_str());
        return nullptr;
    }

    // create the SAX handler
    SSTConfigSaxHandler handler;
    handler.setConfigGraph(graph_);
    bool result = json::sax_parse(ifs, &handler);
    if ( !result ) {
        output_->fatal(CALL_INFO, 1, "Error parsing json file at position %lu: (%s)\n", handler.error_pos_,
            handler.error_str_.c_str());
        return nullptr;
    }

    // set the program options
    auto options = handler.getProgramOptions();
    for ( const auto& [key, value] : options ) {
        setOptionFromModel(key, value);
    }

    // close the file
    ifs.close();
    return graph_;
}
