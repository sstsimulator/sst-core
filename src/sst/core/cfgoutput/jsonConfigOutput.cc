// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include <sst_config.h>

#include <sst/core/warnmacros.h>
#include <sst/core/config.h>
#include <sst/core/configGraph.h>
#include <sst/core/configGraphOutput.h>
#include <sst/core/params.h>
#include <sst/core/cfgoutput/jsonConfigOutput.h>

#include <map>

using namespace SST::Core;

JSONConfigGraphOutput::JSONConfigGraphOutput(const char* path) :
	ConfigGraphOutput(path) {

}

void JSONConfigGraphOutput::generate(const Config* UNUSED(cfg), ConfigGraph* graph) {

	int num = 0;
	if(NULL == outputFile) {
		throw ConfigGraphOutputException("Output file is not open for writing");
	}

	auto compMap = graph->getComponentMap();
	auto linkMap = graph->getLinkMap();
	
	num = compMap.size();
	fprintf(outputFile, "{\n\t\"components\" : [\n");
	for ( auto compItr : compMap ) {
		fprintf(outputFile, "\t\t{\n");
		generateJSON( "\t\t\t", compItr, linkMap );
		fprintf(outputFile, "\t\t}");
		num--; (num > 0) ? fprintf(outputFile, ",\n") : fprintf(outputFile, "\n");
	}
	
	num = linkMap.size();
  fprintf(outputFile, "\t],\n\t\"links\" : [\n");
	for ( auto linkItr : linkMap ) {
		generateJSON( linkItr, compMap );
		fprintf(outputFile, "\t\t}");
		num--; (num > 0) ? fprintf(outputFile, ",\n") : fprintf(outputFile, "\n");
	}
	fprintf(outputFile, "\t]\n}");
}


void JSONConfigGraphOutput::generateJSON(const std::string indent, const ConfigComponent& comp, const ConfigLinkMap_t& linkMap) const {
	
	int num = 0;
	std::string temp = indent + "\"name\" : \"" + comp.name + "\",\n" + indent + "\"type\" : \"" + comp.type + "\",\n";
	fprintf(outputFile, "%s", temp.c_str());
	
	num = comp.links.size();
	if (num > 0){
		fprintf(outputFile,	"%s\"ports\" : [\n", indent.c_str());
		for(LinkId_t i : comp.links) {
			const ConfigLink &link = linkMap[i];
			const int port = (link.component[0] == comp.id) ? 0 : 1;
			fprintf(outputFile, "%s\t{ \"name\" : \"%s\"}", indent.c_str(), link.port[port].c_str());
			num--; (num > 0) ? fprintf(outputFile, ",\n") : fprintf(outputFile, "\n");
		}
		fprintf(outputFile, "%s],\n", indent.c_str());
	}
	
	num = comp.subComponents.size();
	if (num > 0){
		fprintf(outputFile, "%s\"subcomponents\" : [\n", indent.c_str());
		for (auto &scItr : comp.subComponents) {
			fprintf(outputFile, "%s\t{\n", indent.c_str());
			generateJSON( indent + "\t\t", scItr, linkMap);
			fprintf(outputFile, "%s\t}", indent.c_str());
			num--; (num > 0) ? fprintf(outputFile, ",\n") : fprintf(outputFile, "\n");
		}
		fprintf(outputFile, "%s],\n", indent.c_str());
	}
	
	num = comp.enabledStatistics.size();
	if (num > 0){
		fprintf(outputFile, "%s\"statistics\" : [\n", indent.c_str());
		for (auto &statItr : comp.enabledStatistics) {
			temp = indent + "\t{\n" + indent + "\t\t\"name\" : \"" + statItr.name + "\"";
			fprintf(outputFile, "%s", temp.c_str());
			int num2 = statItr.params.size();
			if (num2 > 0){
				fprintf(outputFile, ",\n%s\t\t\"params\" : [\n", indent.c_str());
				for(auto &paramsItr : statItr.params.getKeys()) {
					temp = indent + "\t\t\t{ \"name\" : \"" + paramsItr + "\", \"value\" : \"" + statItr.params.find<std::string>(paramsItr) + "\" }";
					fprintf(outputFile, "%s", temp.c_str());
					num2--; (num2 > 0) ? fprintf(outputFile, ",\n") : fprintf(outputFile, "\n");
				}
				fprintf(outputFile, "%s\t\t]\n", indent.c_str());
			}
			fprintf(outputFile, "\n%s\t}", indent.c_str());
			num--; (num > 0) ? fprintf(outputFile, ",\n") : fprintf(outputFile, "\n");
		}
		fprintf(outputFile, "%s],\n", indent.c_str());
	}
	
	num = comp.params.size();
	fprintf(outputFile, "%s\"params\" : [\n", indent.c_str());
	for(auto &paramsItr : comp.params.getKeys()) {
		temp = indent + "\t{ \"name\" : \"" + paramsItr + "\", \"value\" : \"" + comp.params.find<std::string>(paramsItr) + "\" }";
		fprintf(outputFile, "%s", temp.c_str());
		num--; (num > 0) ? fprintf(outputFile, ",\n") : fprintf(outputFile, "\n");
	}
	fprintf(outputFile, "%s]\n", indent.c_str());
}

void JSONConfigGraphOutput::generateJSON(const ConfigLink& link, const ConfigComponentMap_t& compMap) const {
	int minLatIdx = (link.latency[0] <= link.latency[1]) ? 0 : 1;
	std::string temp = "\t\t{\n\t\t\t\"name\" : \"" + link.name + "\",\n" + 
		"\t\t\t\"left\" : \"" + compMap[link.component[0]].name + "\",\n" + 
		"\t\t\t\"leftPort\" : \"" + link.port[0] + "\",\n" + 
		"\t\t\t\"right\" : \"" + compMap[link.component[1]].name + "\",\n" + 
		"\t\t\t\"rightPort\" : \"" + link.port[1] + "\",\n" + 
		"\t\t\t\"latency\" : \"" + link.latency_str[minLatIdx] + "\"\n";
	fprintf(outputFile, "%s", temp.c_str());
}
