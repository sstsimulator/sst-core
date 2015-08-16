
#include <sst_config.h>

#include <sst/core/config.h>
#include <sst/core/configGraph.h>
#include <sst/core/configGraphOutput.h>
#include <sst/core/params.h>

#include <map>

#include "JSONConfigOutput.h"

using namespace SST::Core;

JSONConfigGraphOutput::JSONConfigGraphOutput(const char* path) :
	ConfigGraphOutput(path) {

}

void JSONConfigGraphOutput::generate(const Config* cfg,
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
                const ConfigLinkMap_t& linkMap) const {

	fprintf(outputFile, "%s  {\n", indent.c_str());
	fprintf(outputFile, "%s    \"name\" : \"%s\",\n", indent.c_str(), comp.name.c_str());
	fprintf(outputFile, "%s    \"type\" : \"%s\",\n", indent.c_str(), comp.type.c_str());

	auto paramsItr = comp.params.begin();

	if(paramsItr != comp.params.end()) {
		fprintf(outputFile, "%s    \"params\" : [\n", indent.c_str());

		fprintf(outputFile, "%s      { \"name\" : \"%s\", \"value\" : \"%s\" }",
			indent.c_str(), Params::getParamName(paramsItr->first).c_str(),
			paramsItr->second.c_str());

		paramsItr++;

		for(; paramsItr != comp.params.end(); paramsItr++) {
			std::string paramName  = Params::getParamName(paramsItr->first);
			std::string paramValue = paramsItr->second;

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
