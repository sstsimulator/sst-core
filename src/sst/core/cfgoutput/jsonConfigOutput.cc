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

void JSONConfigGraphOutput::generate(const Config* UNUSED(cfg),
                ConfigGraph* graph) throw(ConfigGraphOutputException) {

	if(NULL == outputFile) {
		throw ConfigGraphOutputException("Output file is not open for writing");
	}

	fprintf(outputFile, "{\n");
	fprintf(outputFile, "     \"components\" : [\n");

	const auto compMap = graph->getComponentMap();
	const auto linkMap = graph->getLinkMap();
	auto compItr = compMap.begin();

	if(compItr != compMap.end()) {
		generateJSON("     ", (*compItr), linkMap);

		compItr++;

		for(; compItr != compMap.end(); compItr++) {
			fprintf(outputFile, ",\n");
			generateJSON("     ", (*compItr), linkMap);
		}
	}

	fprintf(outputFile, "     ],\n");
	fprintf(outputFile, "     \"links\" : [\n");
	fprintf(outputFile, "     ]\n");
	fprintf(outputFile, "}\n");
}

void JSONConfigGraphOutput::generateJSON(const std::string indent, const ConfigComponent& comp,
                const ConfigLinkMap_t& UNUSED(linkMap)) const {

	fprintf(outputFile, "%s  {\n", indent.c_str());
	fprintf(outputFile, "%s    \"name\" : \"%s\",\n", indent.c_str(), comp.name.c_str());
	fprintf(outputFile, "%s    \"type\" : \"%s\",\n", indent.c_str(), comp.type.c_str());

	// auto paramsItr = comp.params.begin();
    auto keys = comp.params.getKeys();
    auto paramsItr = keys.begin();

	// if(paramsItr != comp.params.end()) {
	if(paramsItr != keys.end()) {
		fprintf(outputFile, "%s    \"params\" : [\n", indent.c_str());

		fprintf(outputFile, "%s      { \"name\" : \"%s\", \"value\" : \"%s\" }",
			// indent.c_str(), Params::getParamName(paramsItr->first).c_str(),
			// paramsItr->second.c_str());
			indent.c_str(), paramsItr->c_str(),
                comp.params.find<std::string>(*paramsItr).c_str());

		paramsItr++;

		// for(; paramsItr != comp.params.end(); paramsItr++) {
		for(; paramsItr != keys.end(); paramsItr++) {
			// std::string paramName  = Params::getParamName(paramsItr->first);
			// std::string paramValue = paramsItr->second;
			std::string paramName  = *paramsItr;
			std::string paramValue = comp.params.find<std::string>(*paramsItr);

			fprintf(outputFile, ",\n%s      { \"name\" : \"%s\", \"value\" : \"%s\" }",
				indent.c_str(), paramName.c_str(), paramValue.c_str());
		}

		fprintf(outputFile, "\n");
		fprintf(outputFile, "%s    ]\n", indent.c_str());
	} else {
		fprintf(outputFile, "%s    \"params\" : [ ]\n", indent.c_str());
	}

	fprintf(outputFile, "%s  }", indent.c_str());
}
