
#ifndef _SST_CORE_CONFIG_READER
#define _SST_CORE_CONFIG_READER

#include <string>
#include <map>

namespace SST {
namespace Core {

void configReadLine(FILE* config, char* buffer, const size_t bufferLen);
void populateConfigMapFromFile(FILE* conf_file,
	std::map<std::string, std::string>& confMap);
void populateConfigMap(std::map<std::string, std::string>& confmap);

}
}

#endif
