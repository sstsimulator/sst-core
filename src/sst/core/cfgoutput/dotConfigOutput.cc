// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include <sst_config.h>
#include <sst/core/configGraphOutput.h>
#include <sst/core/config.h>
#include "dotConfigOutput.h"

using namespace SST::Core;

DotConfigGraphOutput::DotConfigGraphOutput(const char* path) :
	ConfigGraphOutput(path) {

}

void DotConfigGraphOutput::generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException) {

	if ( NULL == outputFile ) {
		throw ConfigGraphOutputException("Output file is not open for writing");
	}

	fprintf(outputFile, "graph \"sst_simulation\" {\n");
	fprintf(outputFile, "\tnewrank = true;\n");
	fprintf(outputFile, "\tnode [shape=record];\n");

	const auto compMap = graph->getComponentMap();
	const auto linkMap = graph->getLinkMap();

	// Find the maximum rank which is marked for the graph partitioning

    for ( uint32_t r = 0; r < cfg->world_size.rank ; r++ ) {
        fprintf(outputFile, "\tsubgraph cluster_%u {\n", r);
        fprintf(outputFile, "\t\tlabel=\"Rank %u\";\n", r);

        for ( uint32_t t = 0 ; t < cfg->world_size.thread ; t++ ) {
            fprintf(outputFile, "\t\tsubgraph cluster_%u_%u {\n", r, t);
            fprintf(outputFile, "\t\t\tlabel=\"Thread %u\";\n", t);

            for ( auto compItr : compMap ) {
                if ( compItr.rank.rank == r && compItr.rank.thread == t ) {
                    fprintf(outputFile, "\t\t\t\t");
                    generateDot( compItr, linkMap );
                }
            }
            fprintf(outputFile, "\t\t};\n");
        }
        fprintf(outputFile, "\t};\n");
    }

    fprintf(outputFile, "\n");
	for ( auto linkItr = linkMap.begin(); linkItr != linkMap.end(); linkItr++ ) {
		fprintf(outputFile, "\t");
		generateDot( *linkItr );
	}

	fprintf(outputFile, "\n}\n");
}

void DotConfigGraphOutput::generateDot(const ConfigComponent& comp,
	const ConfigLinkMap_t& linkMap) const {

	fprintf(outputFile, "%" PRIu64 " [label=\"{%s\\n%s",
		(uint64_t) comp.id, comp.name.c_str(), comp.type.c_str());

    for(LinkId_t i : comp.links) {
        const ConfigLink &link = linkMap[i];
        const int port = (link.component[0] == comp.id) ? 0 : 1;

        fprintf(outputFile, " | < %s > Port: %s", link.port[port].c_str(), link.port[port].c_str());
    }
    fprintf(outputFile, " }\"];\n");
}

void DotConfigGraphOutput::generateDot(const ConfigLink& link) const {

    int minLatIdx = (link.latency[0] <= link.latency[1]) ? 0 : 1;
	fprintf(outputFile, "%" PRIu64 ":\"%s\" -- %" PRIu64 ":\"%s\" [label=\"%s\\n%s\"]; \n",
		(uint64_t) link.component[0], link.port[0].c_str(),
		(uint64_t) link.component[1], link.port[1].c_str(),
		link.name.c_str(),
        link.latency_str[minLatIdx].c_str());
}
