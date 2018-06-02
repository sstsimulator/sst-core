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
#include <sst/core/warnmacros.h>
#include <sst/core/configGraphOutput.h>
#include "xmlConfigOutput.h"

using namespace SST::Core;

XMLConfigGraphOutput::XMLConfigGraphOutput(const char* path) :
	ConfigGraphOutput(path) {

}

void XMLConfigGraphOutput::generate(const Config* UNUSED(cfg),
                ConfigGraph* graph) throw(ConfigGraphOutputException) {

	if(NULL == outputFile) {
		throw ConfigGraphOutputException("Output file is not open for writing");
	}

	fprintf(outputFile, "<?xml version=\"1.0\" ?>\n");
	fprintf(outputFile, "<component id=\"root\" name=\"root\">\n");
	fprintf(outputFile, "   <component id=\"system\" name=\"system\">\n");

	const auto compMap = graph->getComponentMap();
	const auto linkMap = graph->getLinkMap();

	for(auto compItr = compMap.begin(); compItr != compMap.end(); compItr++) {
		generateXML("      ", (*compItr), linkMap);
	}

	for(auto linkItr = linkMap.begin(); linkItr != linkMap.end(); linkItr++) {
		generateXML("      ", (*linkItr), compMap);
	}

	fprintf(outputFile, "   </component>\n");
	fprintf(outputFile, "</component>\n");
}

void XMLConfigGraphOutput::generateXML(const std::string indent, const ConfigComponent& comp,
                const ConfigLinkMap_t& UNUSED(linkMap)) const {

	fprintf(outputFile, "%s<component id=\"system.%s\" name=\"%s\" type=\"%s\">\n",
		indent.c_str(), comp.name.c_str(), comp.name.c_str(), comp.type.c_str());

	// for(auto paramsItr = comp.params.begin(); paramsItr != comp.params.end(); paramsItr++) {
    auto keys = comp.params.getKeys();
	for(auto paramsItr = keys.begin(); paramsItr != keys.end(); paramsItr++) {
		std::string paramName  = *paramsItr;
		std::string paramValue = comp.params.find<std::string>(*paramsItr);

		fprintf(outputFile, "%s%s<param name=\"%s\" value=\"%s\"/>\n",
			indent.c_str(), "   ",
			paramName.c_str(), paramValue.c_str());
	}

	fprintf(outputFile, "%s</component>\n", indent.c_str());
}

void XMLConfigGraphOutput::generateXML(const std::string indent, const ConfigLink& link,
	const ConfigComponentMap_t& compMap) const {

	const ConfigComponent* link_left  = &compMap[link.component[0]];
	const ConfigComponent* link_right = &compMap[link.component[1]];

	fprintf(outputFile, "%s<link id=\"%s\" name=\"%s\"\n%s%sleft=\"%s\" right=\"%s\"\n%s%sleftport=\"%s\" rightport=\"%s\"/>\n",
		indent.c_str(), link.name.c_str(), link.name.c_str(),
		indent.c_str(), "   ",
		link_left->name.c_str(), link_right->name.c_str(),
		indent.c_str(), "   ",
		link.port[0].c_str(), link.port[1].c_str());
}
