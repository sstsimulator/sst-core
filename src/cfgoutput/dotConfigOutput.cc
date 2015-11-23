
#include <sst_config.h>
#include <sst/core/configGraphOutput.h>
#include "dotConfigOutput.h"

using namespace SST::Core;

DotConfigGraphOutput::DotConfigGraphOutput(const char* path) :
	ConfigGraphOutput(path) {

}

void DotConfigGraphOutput::generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException) {

	if(NULL == outputFile) {
		throw ConfigGraphOutputException("Output file is not open for writing");
	}

	fprintf(outputFile, "graph \"sst_simulation\" {\n");
	fprintf(outputFile, "\tnode [shape=record] ;\n");

	uint32_t maxRank = 0;
	const auto compMap = graph->getComponentMap();
	const auto linkMap = graph->getLinkMap();

	// Find the maximum rank which is marked for the graph partitioning
	for(auto compItr = compMap.begin(); compItr != compMap.end(); compItr++) {
		maxRank = (compItr->rank.rank > maxRank) ? compItr->rank.rank : maxRank;
	}

	if( maxRank > 0 ) {
		for( uint32_t r = 0; r <= maxRank; r++ ) {
			fprintf(outputFile, "subgraph cluster %u {\n", r);

			for(auto compItr = compMap.begin(); compItr != compMap.end(); compItr++) {
				if( compItr->rank.rank == r ) {
					fprintf(outputFile, "\t\t");
					generateDot( *compItr, linkMap );
				}
			}
		}
	} else {
		for(auto compItr = compMap.begin(); compItr != compMap.end(); compItr++) {
			fprintf(outputFile, "\t");
			generateDot( *compItr, linkMap );
		}
	}

	for(auto linkItr = linkMap.begin(); linkItr != linkMap.end(); linkItr++) {
		fprintf(outputFile, "\t");
		generateDot( *linkItr );
	}

	fprintf(outputFile, "\n}\n");
}

void DotConfigGraphOutput::generateDot(const ConfigComponent& comp,
	const ConfigLinkMap_t& linkMap) const {

	fprintf(outputFile, "%" PRIu64 " [label=\"{%s\\n%s | {",
		(uint64_t) comp.id, comp.name.c_str(), comp.type.c_str());

	for(auto linkItr = linkMap.begin(); linkItr != linkMap.end(); linkItr++) {
		const ConfigLink& link = *linkItr;

		const int port = (link.component[0] == comp.id) ? 0 : 1;
		fprintf(outputFile, " < %s > %s", link.port[port].c_str(),
			link.port[port].c_str());

		if( (linkItr + 1) != linkMap.end() ) {
			fprintf(outputFile, " |");
		}
	}

	fprintf(outputFile, " } }\"];\n");
}

void DotConfigGraphOutput::generateDot(const ConfigLink& link) const {

	fprintf(outputFile, "%" PRIu64 ":\"%s\" -- %" PRIu64
		":%s [label=\"%s\"]; \n",
		(uint64_t) link.component[0], link.port[0].c_str(),
		(uint64_t) link.component[1], link.port[1].c_str(),
		link.name.c_str());
}
