// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include "sst_config.h"

#include "pythonConfigOutput.h"

#include "sst/core/config.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"

using namespace SST::Core;

PythonConfigGraphOutput::PythonConfigGraphOutput(const char* path) : ConfigGraphOutput(path), graph(nullptr) {}

void
PythonConfigGraphOutput::generateParams(const Params& params)
{
    if ( params.empty() ) return;

    fprintf(outputFile, "{\n");

    bool firstItem = true;
    // for ( auto& key : params.getKeys() ) {
    for ( auto& key : getParamsLocalKeys(params) ) {
        char* esParamName = makeEscapeSafe(key.c_str());
        char* esValue     = makeEscapeSafe(params.find<std::string>(key).c_str());

        if ( isMultiLine(esValue) ) {
            fprintf(outputFile, "%s     \"%s\" : \"\"\"%s\"\"\"", firstItem ? "" : ",\n", esParamName, esValue);
        }
        else {
            fprintf(outputFile, "%s     \"%s\" : \"%s\"", firstItem ? "" : ",\n", esParamName, esValue);
        }

        free(esParamName);
        free(esValue);

        firstItem = false;
    }

    fprintf(outputFile, "\n}");
}

const std::string&
PythonConfigGraphOutput::getLinkObject(LinkId_t id, bool no_cut)
{
    if ( linkMap.find(id) == linkMap.end() ) {
        char tmp[8 + 8 + 1];
        snprintf(tmp, 8 + 8 + 1, "link_id_%08" PRIx32, id);
        fprintf(outputFile, "%s = sst.Link(\"%s\")\n", tmp, tmp);
        if ( no_cut ) fprintf(outputFile, "%s.setNoCut()\n", tmp);
        linkMap[id] = tmp;
    }
    return linkMap[id];
}

void
PythonConfigGraphOutput::generateCommonComponent(const char* objName, const ConfigComponent* comp)
{
    if ( !comp->params.empty() ) {
        // Add local params
        fprintf(outputFile, "%s.addParams(", objName);
        generateParams(comp->params);
        fprintf(outputFile, ")\n");
        // Add global param sets
        for ( auto x : getSubscribedGlobalParamSets(comp->params) ) {
            fprintf(outputFile, "%s.addGlobalParamSet(\"%s\")\n", objName, x.c_str());
        }
    }

    fprintf(outputFile, "%s.setCoordinates(", objName);
    bool first = true;
    for ( double d : comp->coords ) {
        fprintf(outputFile, first ? "%lg" : ", %lg", d);
        first = false;
    }
    fprintf(outputFile, ")\n");

    for ( auto& pair : comp->enabledStatNames ) {
        auto& name       = pair.first;
        auto* si         = comp->findStatistic(pair.second);
        char* esStatName = makeEscapeSafe(name.c_str());

        fprintf(outputFile, "%s.enableStatistics([\"%s\"]", objName, esStatName);

        // Output the Statistic Parameters
        if ( !si->params.empty() ) {
            fprintf(outputFile, ", ");
            generateParams(si->params);
        }
        fprintf(outputFile, ")\n");

        free(esStatName);
    }

    UnitAlgebra tb = Simulation_impl::getTimeLord()->getTimeBase();

    for ( auto linkID : comp->links ) {
        const ConfigLink* link       = getGraph()->getLinkMap()[linkID];
        int               idx        = link->component[0] == comp->id ? 0 : 1;
        SimTime_t         latency    = link->latency[idx];
        auto              tmp        = tb * latency;
        std::string       latencyStr = link->latency_str[idx];
        char*             esPortName = makeEscapeSafe(link->port[idx].c_str());

        const std::string& linkName = getLinkObject(linkID, link->no_cut);
        fprintf(
            outputFile, "%s.addLink(%s, \"%s\", \"%s\")\n", objName, linkName.c_str(), esPortName,
            tmp.toStringBestSI().c_str());

        free(esPortName);
    }

    for ( auto subComp : comp->subComponents ) {
        generateSubComponent(objName, subComp);
    }
}

void
PythonConfigGraphOutput::generateSubComponent(const char* owner, const ConfigComponent* comp)
{
    size_t slen     = strlen(owner) + strlen("_subcomp_") + 1;
    char*  combName = (char*)calloc(1, slen);
    snprintf(combName, slen, "%s%s", owner, "_subcomp_");
    char* pyCompName = makePythonSafeWithPrefix(comp->name.c_str(), combName);
    char* esCompName = makeEscapeSafe(comp->name.c_str());

    fprintf(
        outputFile, "%s = %s.setSubComponent(\"%s\", \"%s\", %d)\n", pyCompName, owner, esCompName, comp->type.c_str(),
        comp->slot_num);

    generateCommonComponent(pyCompName, comp);

    free(pyCompName);
    free(esCompName);
    free(combName);
}

void
PythonConfigGraphOutput::generateComponent(const ConfigComponent* comp, bool output_parition_info)
{
    char* pyCompName = makePythonSafeWithPrefix(comp->name.c_str(), "comp_");
    char* esCompName = makeEscapeSafe(comp->name.c_str());

    fprintf(outputFile, "%s = sst.Component(\"%s\", \"%s\")\n", pyCompName, esCompName, comp->type.c_str());

    if ( output_parition_info ) {
        fprintf(outputFile, "%s.setRank(%d,%d)\n", pyCompName, comp->rank.rank, comp->rank.thread);
    }

    generateCommonComponent(pyCompName, comp);

    free(pyCompName);
    free(esCompName);
}

void
PythonConfigGraphOutput::generateStatGroup(const ConfigGraph* graph, const ConfigStatGroup& grp)
{
    char* pyGroupName = makePythonSafeWithPrefix(grp.name.c_str(), "statGroup_");
    char* esGroupName = makeEscapeSafe(grp.name.c_str());

    fprintf(outputFile, "%s = sst.StatisticGroup(\"%s\")\n", pyGroupName, esGroupName);
    if ( grp.outputFrequency.getValue() != 0 ) {
        fprintf(outputFile, "%s.setFrequency(\"%s\")\n", pyGroupName, grp.outputFrequency.toStringBestSI().c_str());
    }
    if ( grp.outputID != 0 ) {
        const ConfigStatOutput& out = graph->getStatOutput(grp.outputID);
        fprintf(outputFile, "%s.setOutput(sst.StatisticOutput(\"%s\"", pyGroupName, out.type.c_str());
        if ( !out.params.empty() ) {
            fprintf(outputFile, ", ");
            generateParams(out.params);
        }
        fprintf(outputFile, "))\n");
    }

    for ( auto& i : grp.statMap ) {
        fprintf(outputFile, "%s.addStatistic(\"%s\"", pyGroupName, i.first.c_str());
        if ( !i.second.empty() ) {
            fprintf(outputFile, ", ");
            generateParams(i.second);
        }
        fprintf(outputFile, ")\n");
    }

    for ( ComponentId_t id : grp.components ) {
        const ConfigComponent* comp       = graph->findComponent(id);
        char*                  pyCompName = makePythonSafeWithPrefix(comp->name.c_str(), "comp_");
        fprintf(outputFile, "%s.addComponent(%s)\n", pyGroupName, pyCompName);
        free(pyCompName);
    }

    free(esGroupName);
    free(pyGroupName);
}

void
PythonConfigGraphOutput::generate(const Config* cfg, ConfigGraph* graph)
{

    if ( nullptr == outputFile ) { throw ConfigGraphOutputException("Input file is not open for output writing"); }

    this->graph = graph;

    // Generate the header and program options
    fprintf(outputFile, "# Automatically generated by SST\n");
    fprintf(outputFile, "import sst\n\n");
    // We will dump all the program options so we can exactly recreate
    // the run just by running the file.  The exceptions will be to
    // information or configuration outputs.  We don't need to
    // recreate output files when we run it again.  They're added in
    // the order they're defined in the config.cc file.

    fprintf(outputFile, "# Define SST Program Options:\n");
    fprintf(outputFile, "# (These reflect the settings from original run and are not necessary in all files)\n");
    fprintf(outputFile, "sst.setProgramOption(\"verbose\", \"%" PRIu32 "\")\n", cfg->verbose());
    fprintf(outputFile, "sst.setProgramOption(\"stop-at\", \"%s\")\n", cfg->stop_at().c_str());
    fprintf(
        outputFile, "sst.setProgramOption(\"print-timing-info\", \"%s\")\n", cfg->print_timing() ? "true" : "false");
    // Ignore stopAfter for now
    // fprintf(outputFile, "sst.setProgramOption(\"stopAfter\", \"%" PRIu32 "\")\n", cfg->stopAfterSec);
    fprintf(outputFile, "sst.setProgramOption(\"heartbeat-period\", \"%s\")\n", cfg->heartbeatPeriod().c_str());
    fprintf(outputFile, "sst.setProgramOption(\"timebase\", \"%s\")\n", cfg->timeBase().c_str());
    fprintf(outputFile, "sst.setProgramOption(\"partitioner\", \"%s\")\n", cfg->partitioner().c_str());
    fprintf(outputFile, "sst.setProgramOption(\"timeVortex\", \"%s\")\n", cfg->timeVortex().c_str());
    fprintf(
        outputFile, "sst.setProgramOption(\"interthread-links\", \"%s\")\n",
        cfg->interthread_links() ? "true" : "false");
    fprintf(outputFile, "sst.setProgramOption(\"output-prefix-core\", \"%s\")\n", cfg->output_core_prefix().c_str());

    // Output the global params
    fprintf(outputFile, "# Define the global parameter sets:\n");
    std::vector<std::string> global_param_sets = getGlobalParamSetNames();
    for ( auto& x : global_param_sets ) {
        fprintf(outputFile, "sst.addGlobalParams(\"%s\", {\n", x.c_str());
        for ( auto y : getGlobalParamSet(x) ) {
            // If the key is <set_name>, then we can skip since it's
            // just metadata
            if ( y.first != "<set_name>" )
                fprintf(outputFile, "    \"%s\" : \"%s\",\n", y.first.c_str(), y.second.c_str());
        }
        fprintf(outputFile, "})\n");
    }
    fprintf(outputFile, "\n");

    // Output the graph
    fprintf(outputFile, "# Define the SST Components:\n");

    auto compMap = graph->getComponentMap();
    for ( auto& comp_itr : compMap ) {
        generateComponent(comp_itr, cfg->output_partition());
        fprintf(outputFile, "\n");
    }

    // Output general statistics options
    fprintf(outputFile, "# Define SST Statistics Options:\n");

    if ( 0 != graph->getStatLoadLevel() ) {
        fprintf(outputFile, "sst.setStatisticLoadLevel(%" PRIu64 ")\n", (uint64_t)graph->getStatLoadLevel());
    }
    if ( !graph->getStatOutput().type.empty() ) {
        fprintf(outputFile, "sst.setStatisticOutput(\"%s\"", graph->getStatOutput().type.c_str());
        const Params& outParams = graph->getStatOutput().params;
        if ( !outParams.empty() ) {
            fprintf(outputFile, ", ");
            generateParams(outParams);
        }
        fprintf(outputFile, ")\n");
    }

    // Check for statistic groups
    if ( !graph->getStatGroups().empty() ) {
        fprintf(outputFile, "\n# Statistic Groups:\n");
        for ( auto& grp : graph->getStatGroups() ) {
            generateStatGroup(graph, grp.second);
        }
    }

    fprintf(outputFile, "# End of generated output.\n\n");

    this->graph = nullptr;
    linkMap.clear();
}

bool
PythonConfigGraphOutput::isMultiLine(const std::string& check) const
{
    bool isMultiLine = false;

    for ( size_t i = 0; i < check.size(); i++ ) {
        if ( check.at(i) == '\n' || check.at(i) == '\r' || check.at(i) == '\f' ) {
            isMultiLine = true;
            break;
        }
    }

    return isMultiLine;
}

bool
PythonConfigGraphOutput::isMultiLine(const char* check) const
{
    const int checkLen    = strlen(check);
    bool      isMultiLine = false;

    for ( int i = 0; i < checkLen; i++ ) {
        if ( check[i] == '\n' || check[i] == '\f' || check[i] == '\r' ) {
            isMultiLine = true;
            break;
        }
    }

    return isMultiLine;
}

char*
PythonConfigGraphOutput::makePythonSafeWithPrefix(const std::string& name, const std::string& prefix) const
{

    const auto nameLength = name.size();
    char*      buffer     = nullptr;

    if ( nameLength > 0 && isdigit(name.at(0)) ) {
        if ( name.size() > prefix.size() ) {
            if ( name.substr(0, prefix.size()) == prefix ) {
                size_t slen = name.size() + 3;
                buffer      = (char*)malloc(sizeof(char) * slen);
                snprintf(buffer, slen, "s_%s", name.c_str());
            }
            else {
                size_t slen = name.size() + prefix.size() + 3;
                buffer      = (char*)malloc(sizeof(char) * slen);
                snprintf(buffer, slen, "%ss_%s", prefix.c_str(), name.c_str());
            }
        }
        else {
            size_t slen = name.size() + prefix.size() + 3;
            buffer      = (char*)malloc(sizeof(char) * slen);
            snprintf(buffer, slen, "%ss_%s", prefix.c_str(), name.c_str());
        }
    }
    else {
        if ( name.size() > prefix.size() ) {
            if ( name.substr(0, prefix.size()) == prefix ) {
                size_t slen = name.size() + 3;
                buffer      = (char*)malloc(sizeof(char) * slen);
                snprintf(buffer, slen, "%s", name.c_str());
            }
            else {
                size_t slen = prefix.size() + name.size() + 1;
                buffer      = (char*)malloc(sizeof(char) * slen);
                snprintf(buffer, slen, "%s%s", prefix.c_str(), name.c_str());
            }
        }
        else {
            size_t slen = prefix.size() + name.size() + 1;
            buffer      = (char*)malloc(sizeof(char) * slen);
            snprintf(buffer, slen, "%s%s", prefix.c_str(), name.c_str());
        }
    }

    makeBufferPythonSafe(buffer);
    return buffer;
};

void
PythonConfigGraphOutput::makeBufferPythonSafe(char* buffer) const
{

    const auto length = strlen(buffer);

    for ( size_t i = 0; i < length; i++ ) {
        switch ( buffer[i] ) {
        case ' ':
            buffer[i] = '_';
            break;
        case '.':
            buffer[i] = '_';
            break;
        case ':':
            buffer[i] = '_';
            break;
        case ',':
            buffer[i] = '_';
            break;
        case '-':
            buffer[i] = '_';
            break;
        }
    }
}

bool
PythonConfigGraphOutput::strncmp(const char* a, const char* b, const size_t n) const
{

    bool matched = true;

    for ( size_t i = 0; i < n; i++ ) {
        if ( a[i] != b[i] ) {
            matched = false;
            break;
        }
    }

    return matched;
};

char*
PythonConfigGraphOutput::makeEscapeSafe(const std::string& input) const
{

    std::string escapedInput = "";
    auto        inputLength  = input.size();

    for ( size_t i = 0; i < inputLength; i++ ) {
        const char nextChar = input.at(i);

        switch ( nextChar ) {
        case '\"':
            escapedInput = escapedInput + "\\\"";
            break;
        case '\'':
            escapedInput = escapedInput + "\\\'";
            break;
        case '\n':
            escapedInput = escapedInput + "\\n";
            break;
        default:
            escapedInput.push_back(nextChar);
        }
    }

    size_t slen          = 1 + escapedInput.size();
    char*  escapedBuffer = (char*)malloc(sizeof(char) * slen);
    snprintf(escapedBuffer, slen, "%s", escapedInput.c_str());

    return escapedBuffer;
}
