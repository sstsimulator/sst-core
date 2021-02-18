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
#include "dotConfigOutput.h"

#include "sst/core/configGraphOutput.h"
#include "sst/core/config.h"

using namespace SST::Core;

DotConfigGraphOutput::DotConfigGraphOutput(const char* path) :
    ConfigGraphOutput(path) {

}

void DotConfigGraphOutput::generate(const Config* cfg, ConfigGraph* graph) {

    if ( nullptr == outputFile ) {
        throw ConfigGraphOutputException("Output file is not open for writing");
    }

    fprintf(outputFile, "graph \"connections\" {\noverlap=scale;\nsplines=spline;\n");
    fprintf(outputFile, "node [shape=record];\ngraph [style=invis];\n\n");
    const auto compMap = graph->getComponentMap();
    const auto linkMap = graph->getLinkMap();
    for ( auto compItr : compMap ) {
        fprintf(outputFile, "subgraph cluster_%" PRIu64 " {\n", compItr.id);
        generateDot( compItr, linkMap );
        fprintf(outputFile, "}\n\n");
    }
    fprintf(outputFile, "\n");
    for ( auto linkItr : linkMap ) {
        generateDot( linkItr );
    }
    fprintf(outputFile, "\n}\n\n");


    fprintf(outputFile, "graph \"sst_simulation\" {\noverlap=scale;\nsplines=spline;\n");
    fprintf(outputFile, "newrank = true;\n");
    fprintf(outputFile, "node [shape=record];\n");
    // Find the maximum rank which is marked for the graph partitioning
    for ( uint32_t r = 0; r < cfg->world_size.rank ; r++ ) {
        fprintf(outputFile, "subgraph cluster_%u {\n", r);
        fprintf(outputFile, "label=\"Rank %u\";\n", r);
        for ( uint32_t t = 0 ; t < cfg->world_size.thread ; t++ ) {
            fprintf(outputFile, "subgraph cluster_%u_%u {\n", r, t);
            fprintf(outputFile, "label=\"Thread %u\";\n", t);
            for ( auto compItr : compMap ) {
                if ( compItr.rank.rank == r && compItr.rank.thread == t ) {
                    generateDot( compItr, linkMap );
                }
            }
            fprintf(outputFile, "};\n");
        }
        fprintf(outputFile, "};\n");
    }
    fprintf(outputFile, "\n");
    for ( auto linkItr = linkMap.begin(); linkItr != linkMap.end(); linkItr++ ) {
        generateDot( *linkItr );
    }
    fprintf(outputFile, "\n}\n");
}


void DotConfigGraphOutput::generateDot(const ConfigComponent& comp, const ConfigLinkMap_t& linkMap) const {
    fprintf(outputFile, "%" PRIu64 " [label=\"{<main> %s\\n%s", comp.id, comp.name.c_str(), comp.type.c_str());
    int j = comp.links.size();
    if(j != 0){
        fprintf(outputFile, " |\n");
    }
    for(LinkId_t i : comp.links) {
        const ConfigLink &link = linkMap[i];
        const int port = (link.component[0] == comp.id) ? 0 : 1;
        fprintf(outputFile, "<%s> Port: %s", link.port[port].c_str(), link.port[port].c_str());
        if(j > 1){
            fprintf(outputFile, " |\n");
        }
        j--;
    }
    fprintf(outputFile, "}\"];\n\n");
    for ( auto &sc : comp.subComponents ) {
        fprintf(outputFile, "%" PRIu64 " [color=gray,label=\"{<main> %s\\n%s", sc.id, sc.name.c_str(), sc.type.c_str());
        j = sc.links.size();
        if(j != 0){
            fprintf(outputFile, " |\n");
        }
        for(LinkId_t i : sc.links) {
            const ConfigLink &link = linkMap[i];
            const int port = (link.component[0] == sc.id) ? 0 : 1;
            fprintf(outputFile, "<%s> Port: %s", link.port[port].c_str(), link.port[port].c_str());
            if(j > 1){
                fprintf(outputFile, " |\n");
            }
            j--;
        }
        fprintf(outputFile, "}\"];\n");
        fprintf(outputFile, "%" PRIu64 ":\"main\" -- %" PRIu64 ":\"main\" [style=dotted];\n\n", comp.id, sc.id);
    }
}


void DotConfigGraphOutput::generateDot(const ConfigLink& link) const {

    int minLatIdx = (link.latency[0] <= link.latency[1]) ? 0 : 1;
    fprintf(outputFile, "%" PRIu64 ":\"%s\" -- %" PRIu64 ":\"%s\" [label=\"%s\\n%s\"]; \n",
        link.component[0], link.port[0].c_str(),
        link.component[1], link.port[1].c_str(),
        link.name.c_str(),
        link.latency_str[minLatIdx].c_str());
}
