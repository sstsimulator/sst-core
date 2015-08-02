
#include <sst_config.h>
#include <sst/core/configGraphOutput.h>
#include "xmlConfigOutput.h"

using namespace SST::Core;

XMLConfigGraphOutput::XMLConfigGraphOutput(const char* path) :
	ConfigGraphOutput(path) {

}

void XMLConfigGraphOutput::generate(const Config* cfg,
                ConfigGraph* graph) throw(ConfigGraphOutputException) {

	if(NULL == outputFile) {
		throw ConfigGraphOutputException("Output file is not open for writing");
	}

}
