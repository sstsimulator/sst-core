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

#include "dotConfigOutput.h"

#include "sst/core/config.h"
#include "sst/core/configGraphOutput.h"

using namespace SST::Core;

DotConfigGraphOutput::DotConfigGraphOutput(const char* path) : ConfigGraphOutput(path) {}

void
DotConfigGraphOutput::generate(const Config* cfg, ConfigGraph* graph)
{

    if ( nullptr == outputFile ) { throw ConfigGraphOutputException("Output file is not open for writing"); }

    fprintf(outputFile, "graph \"sst_simulation\" {\noverlap=scale;\nsplines=spline;\n");
    const auto compMap = graph->getComponentMap();
    const auto linkMap = graph->getLinkMap();

    // High detail original SST dot graph output
    if ( cfg->dot_verbosity() >= 10 ) {
        fprintf(outputFile, "newrank = true;\n");
        fprintf(outputFile, "node [shape=record];\n");
        // Find the maximum rank which is marked for the graph partitioning
        for ( uint32_t r = 0; r < cfg->num_ranks(); r++ ) {
            fprintf(outputFile, "subgraph cluster_%u {\n", r);
            fprintf(outputFile, "label=\"Rank %u\";\n", r);
            for ( uint32_t t = 0; t < cfg->num_threads(); t++ ) {
                fprintf(outputFile, "subgraph cluster_%u_%u {\n", r, t);
                fprintf(outputFile, "label=\"Thread %u\";\n", t);
                for ( auto compItr : compMap ) {
                    if ( compItr->rank.rank == r && compItr->rank.thread == t ) {
                        generateDot(compItr, linkMap, cfg->dot_verbosity());
                    }
                }
                fprintf(outputFile, "};\n");
            }
            fprintf(outputFile, "};\n");
        }

        // Less detailed, doesn't show MPI ranks
    }
    else {
        fprintf(outputFile, "node [shape=record];\ngraph [style=invis];\n\n");
        for ( auto compItr : compMap ) {
            fprintf(outputFile, "subgraph cluster_%" PRIu64 " {\n", compItr->id);
            generateDot(compItr, linkMap, cfg->dot_verbosity());
            fprintf(outputFile, "}\n\n");
        }
    }

    fprintf(outputFile, "\n");
    for ( auto linkItr : linkMap ) {
        generateDot(linkItr, cfg->dot_verbosity());
    }
    fprintf(outputFile, "\n}\n");
}

void
DotConfigGraphOutput::generateDot(
    const ConfigComponent* comp, const ConfigLinkMap_t& linkMap, const uint32_t dot_verbosity,
    const ConfigComponent* parent = nullptr) const
{

    // Display component type
    if ( parent ) { fprintf(outputFile, "%" PRIu64 " [color=gray,label=\"{<main> ", comp->id); }
    else {
        fprintf(outputFile, "%" PRIu64 " [label=\"{<main> ", comp->id);
    }
    if ( dot_verbosity >= 2 ) { fprintf(outputFile, "%s\\n%s", comp->name.c_str(), comp->type.c_str()); }
    else {
        fprintf(outputFile, "%s", comp->name.c_str());
    }

    // Display ports
    if ( dot_verbosity >= 6 ) {
        int j = comp->links.size();
        if ( j != 0 ) { fprintf(outputFile, " |\n"); }
        for ( LinkId_t i : comp->links ) {
            const ConfigLink* link = linkMap[i];
            const int         port = (link->component[0] == comp->id) ? 0 : 1;
            fprintf(outputFile, "<%s> Port: %s", link->port[port].c_str(), link->port[port].c_str());
            if ( j > 1 ) { fprintf(outputFile, " |\n"); }
            j--;
        }
    }
    fprintf(outputFile, "}\"];\n\n");
    if ( parent ) {
        fprintf(outputFile, "%" PRIu64 ":\"main\" -- %" PRIu64 ":\"main\" [style=dotted];\n\n", comp->id, parent->id);
    }

    // Display subComponents
    if ( dot_verbosity >= 4 ) {
        for ( auto& sc : comp->subComponents ) {
            generateDot(sc, sc->links, dot_verbosity, comp);
        }
    }
}

void
DotConfigGraphOutput::generateDot(const ConfigLink* link, const uint32_t dot_verbosity) const
{

    int minLatIdx = (link->latency[0] <= link->latency[1]) ? 0 : 1;
    // Link name and latency displayed. Connected to specific port on component
    if ( dot_verbosity >= 8 ) {
        fprintf(
            outputFile, "%" PRIu64 ":\"%s\" -- %" PRIu64 ":\"%s\" [label=\"%s\\n%s\"]; \n", link->component[0],
            link->port[0].c_str(), link->component[1], link->port[1].c_str(), link->name.c_str(),
            link->latency_str[minLatIdx].c_str());

        // No link name or latency. Connected to specific port on component
    }
    else if ( dot_verbosity >= 6 ) {
        fprintf(
            outputFile, "%" PRIu64 ":\"%s\" -- %" PRIu64 ":\"%s\"\n", link->component[0], link->port[0].c_str(),
            link->component[1], link->port[1].c_str());

        // No link name or latency. Connected to component NOT port
    }
    else {
        fprintf(outputFile, "%" PRIu64 " -- %" PRIu64 "\n", link->component[0], link->component[1]);
    }
}
