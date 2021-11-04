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

#include "sst/core/model/json/jsonmodel.h"

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include <string>

DISABLE_WARN_STRICT_ALIASING

using namespace SST;
using namespace SST::Core;

SSTJSONModelDefinition::SSTJSONModelDefinition(
    const std::string& script_file, int verbosity, Config* configObj, double start_time) :
    SSTModelDescription(),
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
    configGraphTuple.clear();
    delete output;
}

ComponentId_t
SSTJSONModelDefinition::findComponentIdByName(std::string Name)
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

    for ( CompTuple::const_iterator i = configGraphTuple.begin(); i != configGraphTuple.end(); ++i ) {
        if ( Comp == std::get<SST_MODEL_JSON_CONFIGCOMPONENT_IDX>(*i) ) {
            return std::get<SST_MODEL_JSON_COMPONENTID_IDX>(*i);
        }
    }

    // if we make it here, nothing was found
    output->fatal(CALL_INFO, 1, "Error finding component ID by name (no ID found): %s\n", Name.c_str());
    return Id;
}

void
SSTJSONModelDefinition::recursiveSubcomponent(ConfigComponent* Parent, nlohmann::basic_json<> compArray)
{
    std::string      Name;
    std::string      Type;
    ComponentId_t    Id;
    ConfigComponent* Comp = nullptr;
    int              Slot = 0;

    for ( auto& subArray : compArray["subcomponents"] ) {
        Slot = 0;

        // -- Slot Name
        try {
            Name = subArray["slot_name"];
        }
        catch ( std::out_of_range& e ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering subcomponent slot name from script: %s\n", scriptName.c_str());
        }

        // -- Type
        try {
            Type = subArray["type"];
        }
        catch ( std::out_of_range& e ) {
            output->fatal(CALL_INFO, 1, "Error discovering subcomponent type from script: %s\n", scriptName.c_str());
        }

        // -- Slot
        try {
            Slot = subArray["slot_number"];
        }
        catch ( std::out_of_range& e ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering subcomponent slot number from script: %s\n", scriptName.c_str());
        }

        // add the subcomponent
        Comp = Parent->addSubComponent(Name, Type, Slot);
        Id   = Comp->id;

        configGraphTuple.push_back(std::tuple<std::string, ComponentId_t, ConfigComponent*>(Name, Id, Comp));

        // read all the parameters
        for ( auto& paramArray : subArray["params"].items() ) {
            Comp->addParameter(paramArray.key(), paramArray.value(), false);
        }

        // read all the global parameters
        for ( auto& globalArray : subArray["params_global_sets"].items() ) {
            Comp->addGlobalParamSet(globalArray.value().get<std::string>());
        }

        // recursively build up the subcomponents
        try {
            auto& subsubArray = subArray["subcomponents"];
            if ( subsubArray.size() > 0 ) { recursiveSubcomponent(Comp, subArray); }
        }
        catch ( std::out_of_range& e ) {
            // do nothing, its ok not to have subcomponents
        }
    }
}

void
SSTJSONModelDefinition::discoverComponents(json jFile)
{
    std::string      Name;
    std::string      Type;
    ComponentId_t    Id;
    ConfigComponent* Comp   = nullptr;
    uint32_t         rank   = 0;
    uint32_t         thread = 0;

    for ( auto& compArray : jFile["components"] ) {
        // -- Name
        try {
            Name = compArray["name"];
        }
        catch ( std::out_of_range& e ) {
            output->fatal(CALL_INFO, 1, "Error discovering component name from script: %s\n", scriptName.c_str());
        }

        // -- Type
        try {
            Type = compArray["type"];
        }
        catch ( std::out_of_range& e ) {
            output->fatal(CALL_INFO, 1, "Error discovering component type from script: %s\n", scriptName.c_str());
        }

        // Add the component so we have the ComponentID
        Id = graph->addComponent(Name, Type);

        Comp = graph->findComponent(Id);
        configGraphTuple.push_back(std::tuple<std::string, ComponentId_t, ConfigComponent*>(Name, Id, Comp));

        // read all the parameters
        for ( auto& paramArray : compArray["params"].items() ) {
            Comp->addParameter(paramArray.key(), paramArray.value(), false);
        }

        // read all the global parameters
        for ( auto& globalArray : compArray["params_global_sets"].items() ) {
            Comp->addGlobalParamSet(globalArray.value().get<std::string>());
        }

        // read the partition info
        for ( auto& partArray : compArray["partition"].items() ) {
            if ( partArray.key() == "rank" ) { rank = partArray.value(); }
            else if ( partArray.key() == "thread" ) {
                thread = partArray.value();
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
SSTJSONModelDefinition::discoverLinks(json jFile)
{
    std::string   Name;
    std::string   LeftComp;
    std::string   LeftPort;
    std::string   LeftLatency;
    std::string   RightComp;
    std::string   RightPort;
    std::string   RightLatency;
    bool          NoCut = false;
    ComponentId_t LinkID;

    for ( auto& linkArray : jFile["links"] ) {
        // -- Name
        try {
            Name = linkArray["name"];
        }
        catch ( std::out_of_range& e ) {
            output->fatal(CALL_INFO, 1, "Error discovering link name from script: %s\n", scriptName.c_str());
        }

        // -- Left Component
        try {
            LeftComp = linkArray["left"].at("component");
        }
        catch ( std::out_of_range& e ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering left link component for Link=%s from script: %s\n", Name.c_str(),
                scriptName.c_str());
        }

        // -- Left Port
        try {
            LeftPort = linkArray["left"].at("port");
        }
        catch ( std::out_of_range& e ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering left link port for Link=%s from script: %s\n", Name.c_str(),
                scriptName.c_str());
        }

        // -- Left Latency
        try {
            LeftLatency = linkArray["left"].at("latency");
        }
        catch ( std::out_of_range& e ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering left link latency for Link=%s from script: %s\n", Name.c_str(),
                scriptName.c_str());
        }

        // -- NoCut
        try {
            if ( linkArray["left"].find("noCut") != linkArray["left"].end() ) { NoCut = linkArray["left"].at("noCut"); }
            else {
                NoCut = false;
            }
        }
        catch ( std::out_of_range& e ) {
            // ignore the catch, default to false;
            NoCut = false;
        }

        // -- Add the Left Link
        LinkID = findComponentIdByName(LeftComp);
        graph->addLink(LinkID, Name, LeftPort, LeftLatency, NoCut);

        // -- Right Component
        try {
            RightComp = linkArray["right"].at("component");
        }
        catch ( std::out_of_range& e ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering right link component for Link=%s from script: %s\n", Name.c_str(),
                scriptName.c_str());
        }

        // -- Right Port
        try {
            RightPort = linkArray["right"].at("port");
        }
        catch ( std::out_of_range& e ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering right link port for Link=%s from script: %s\n", Name.c_str(),
                scriptName.c_str());
        }

        // -- Left Latency
        try {
            RightLatency = linkArray["right"].at("latency");
        }
        catch ( std::out_of_range& e ) {
            output->fatal(
                CALL_INFO, 1, "Error discovering right link latency for Link=%s from script: %s\n", Name.c_str(),
                scriptName.c_str());
        }

        // -- NoCut
        try {
            if ( linkArray["right"].find("noCut") != linkArray["right"].end() ) {
                NoCut = linkArray["right"].at("noCut");
            }
            else {
                NoCut = false;
            }
        }
        catch ( std::out_of_range& e ) {
            // ignore the catch, default to false;
            NoCut = false;
        }

        // -- Add the Left Link
        LinkID = findComponentIdByName(RightComp);
        graph->addLink(LinkID, Name, RightPort, RightLatency, NoCut);
    }
}

void
SSTJSONModelDefinition::discoverProgramOptions(json jFile)
{
    size_t idx = 0;

    while ( !jFile["program_options"][idx].is_null() ) {
        for ( auto& it : jFile["program_options"][idx].items() ) {
            for ( auto global : programOpts ) {
                if ( it.key() == global ) {
                    config->setConfigEntryFromModel(global, jFile["program_options"][idx][global].get<std::string>());
                }
            }
        }
        idx++;
    }
}

void
SSTJSONModelDefinition::discoverGlobalParams(json jFile)
{
    std::string GlobalName;

    for ( auto& gp : jFile["global_params"].items() ) {
        GlobalName = gp.key();
        for ( auto& param : jFile["global_params"].at(GlobalName).items() ) {
            graph->addGlobalParam(GlobalName, param.key(), param.value().get<std::string>());
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
