// -*- c++ -*-

// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/model/json/jsonmodel.h"

#include <string>

DISABLE_WARN_STRICT_ALIASING

using namespace SST;
using namespace SST::Core;

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
    if ( !graph ) { output->fatal(CALL_INFO, 1, "Could not create graph object in JSON loader.\n"); }

    output->verbose(CALL_INFO, 2, 0, "SST loading a JSON model from script: %s\n", script_file.c_str());
}

SSTJSONModelDefinition::~SSTJSONModelDefinition()
{
    delete output;
}

ComponentId_t
SSTJSONModelDefinition::findComponentIdByName(const std::string& Name)
{
    ComponentId_t    Id   = -1;
    ConfigComponent* Comp = nullptr;

    if ( Name.length() == 0 ) {
        output->fatal(CALL_INFO, 1, "Error: Name given to findComponentIdByName is null\n");
        return Id;
    }

    // first, find the component pointer from the name
    Comp = graph->findComponentByName(Name);
    if ( Comp == nullptr ) {
        output->fatal(CALL_INFO, 1, "Error finding component ID by name: %s\n", Name.c_str());
        return Id;
    }

    return Comp->id;
}

void
SSTJSONModelDefinition::recursiveSubcomponent(ConfigComponent* Parent, const nlohmann::basic_json<>& compArray)
{
    std::string      Name;
    std::string      Type;
    ConfigComponent* Comp = nullptr;
    int              Slot = 0;

    auto subs = compArray.find("subcomponents");
    if ( subs == compArray.end() ) return; // No subcomponents
    for ( auto& subArray : compArray["subcomponents"] ) {
        Slot = 0;

        // -- Slot Name
        auto x = subArray.find("slot_name");
        if ( x != subArray.end() ) { Name = x.value(); }
        else {
            output->fatal(
                CALL_INFO, 1, "Error discovering subcomponent slot name from script: %s\n", scriptName.c_str());
        }

        // -- Type
        x = subArray.find("type");
        if ( x != subArray.end() ) { Type = x.value(); }
        else {
            output->fatal(CALL_INFO, 1, "Error discovering subcomponent type from script: %s\n", scriptName.c_str());
        }

        // -- Slot index
        x = subArray.find("slot_number");
        if ( x != subArray.end() ) { Slot = x.value(); }
        else {
            output->fatal(
                CALL_INFO, 1, "Error discovering subcomponent slot number from script: %s\n", scriptName.c_str());
        }

        // add the subcomponent
        Comp = Parent->addSubComponent(Name, Type, Slot);

        // read all the parameters
        if ( subArray.contains("params") ) {
            for ( auto& paramArray : subArray["params"].items() ) {
                Comp->addParameter(paramArray.key(), paramArray.value(), false);
            }
        }

        // read all the global parameters
        if ( subArray.contains("params_global_sets") ) {
            for ( auto& globalArray : subArray["params_global_sets"].items() ) {
                Comp->addGlobalParamSet(globalArray.value().get<std::string>());
            }
        }

        // recursively build up the subcomponents
        if ( subArray.contains("subcomponents") ) {
            auto& subsubArray = subArray["subcomponents"];
            if ( subsubArray.size() > 0 ) { recursiveSubcomponent(Comp, subArray); }
        }
    }
}

void
SSTJSONModelDefinition::discoverComponents(const json& jFile)
{
    std::string      Name;
    std::string      Type;
    ComponentId_t    Id;
    ConfigComponent* Comp   = nullptr;
    uint32_t         rank   = 0;
    uint32_t         thread = 0;

    if ( !jFile.contains("components") ) {
        output->fatal(CALL_INFO, 1, "Error, no \"components\" section in json file: %s\n", scriptName.c_str());
    }

    for ( auto& compArray : jFile["components"] ) {
        // -- Name
        auto x = compArray.find("name");
        if ( x != compArray.end() ) { Name = x.value(); }
        else {
            output->fatal(CALL_INFO, 1, "Error discovering component name from script: %s\n", scriptName.c_str());
        }

        // -- Type
        x = compArray.find("type");
        if ( x != compArray.end() ) { Type = x.value(); }
        else {
            output->fatal(CALL_INFO, 1, "Error discovering component type from script: %s\n", scriptName.c_str());
        }

        // Add the component so we have the ComponentID
        Id = graph->addComponent(Name, Type);

        Comp = graph->findComponent(Id);

        // read all the parameters
        if ( compArray.contains("params") ) {
            for ( auto& paramArray : compArray["params"].items() ) {
                Comp->addParameter(paramArray.key(), paramArray.value(), false);
            }
        }

        // read all the global parameters
        if ( compArray.contains("params_global_sets") ) {
            for ( auto& globalArray : compArray["params_global_sets"].items() ) {
                Comp->addGlobalParamSet(globalArray.value().get<std::string>());
            }
        }

        // read the partition info
        if ( compArray.contains("partition") ) {
            for ( auto& partArray : compArray["partition"].items() ) {
                if ( partArray.key() == "rank" ) { rank = partArray.value(); }
                else if ( partArray.key() == "thread" ) {
                    thread = partArray.value();
                }
            }
        }

        // set the rank information
        RankInfo Rank(rank, thread);
        Comp->setRank(Rank);

        // recursively read the subcomponents
        recursiveSubcomponent(Comp, compArray);

        // reset the variables
        Id   = -1;
        Comp = nullptr;
    }
}

void
SSTJSONModelDefinition::discoverLinks(const json& jFile)
{
    std::string   Name;
    std::string   Comp[2];
    std::string   Port[2];
    std::string   Latency[2];
    bool          NoCut = false;
    ComponentId_t LinkID;

    if ( !jFile.contains("links") ) {
        output->fatal(CALL_INFO, 1, "Error, no \"links\" section in json file: %s\n", scriptName.c_str());
    }

    for ( auto& linkArray : jFile["links"] ) {
        // -- Name
        auto x = linkArray.find("name");
        if ( x != linkArray.end() ) { Name = x.value(); }
        else {
            output->fatal(CALL_INFO, 1, "Error discovering link name from script: %s\n", scriptName.c_str());
        }

        // -- NoCut
        x = linkArray.find("noCut");
        if ( x != linkArray.end() ) { NoCut = x.value(); }
        else {
            NoCut = false;
        }

        // -- Components
        std::string sides[2] = { "left", "right" };
        for ( int i = 0; i < 2; ++i ) {
            auto side = linkArray.find(sides[i]);
            if ( side == linkArray.end() ) {
                output->fatal(
                    CALL_INFO, 1, "Error discovering %s link component for Link=%s from script: %s\n", sides[i].c_str(),
                    Name.c_str(), scriptName.c_str());
            }

            auto item = side->find("component");
            if ( item != side->end() ) { Comp[i] = item.value(); }
            else {
                output->fatal(
                    CALL_INFO, 1, "Error finding component field of %s link component for Link=%s from script: %s\n",
                    sides[i].c_str(), Name.c_str(), scriptName.c_str());
            }

            // -- Port
            item = side->find("port");
            if ( item != side->end() ) { Port[i] = item.value(); }
            else {
                output->fatal(
                    CALL_INFO, 1, "Error finding port field of %s link component for Link=%s from script: %s\n",
                    sides[i].c_str(), Name.c_str(), scriptName.c_str());
            }

            // -- Latency
            item = side->find("latency");
            if ( item != side->end() ) { Latency[i] = item.value(); }
            else {
                output->fatal(
                    CALL_INFO, 1, "Error finding latency field of %s link component for Link=%s from script: %s\n",
                    sides[i].c_str(), Name.c_str(), scriptName.c_str());
            }

            LinkID = findComponentIdByName(Comp[i]);
            graph->addLink(LinkID, Name, Port[i], Latency[i], NoCut);
        }
    }
}

void
SSTJSONModelDefinition::discoverProgramOptions(const json& jFile)
{
    if ( jFile.contains("program_options") ) {
        for ( auto& option : jFile["program_options"].items() ) {
            setOptionFromModel(option.key(), option.value());
        }
    }
}

void
SSTJSONModelDefinition::discoverGlobalParams(const json& jFile)
{
    std::string GlobalName;

    if ( jFile.contains("global_params") ) {
        for ( auto& gp : jFile["global_params"].items() ) {
            GlobalName = gp.key();
            for ( auto& param : jFile["global_params"].at(GlobalName).items() ) {
                graph->addGlobalParam(GlobalName, param.key(), param.value().get<std::string>());
            }
        }
    }
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

    // create parsed json object
    json jFile = json::parse(ifs);

    // close the file
    ifs.close();

    // discover all the globals
    discoverProgramOptions(jFile);

    // discover the global parameters
    discoverGlobalParams(jFile);

    // discover the components
    discoverComponents(jFile);

    // discover the links
    discoverLinks(jFile);

    // TODO: discover statistics

    return graph;
}
