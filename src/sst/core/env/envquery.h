
#ifndef _H_SST_CORE_ENV_QUERY_H
#define _H_SST_CORE_ENV_QUERY_H

#include <sst_config.h>

#include <sst/core/env/envconfig.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>

namespace SST {
namespace Core {
namespace Environment {

void configReadLine(FILE* theFile, char* lineBuffer);
void populateEnvironmentConfig(const std::string& path, EnvironmentConfiguration* cfg);
EnvironmentConfiguration* getSSTEnvironmentConfiguration(const std::vector<std::string>& overridePaths);

}
}
}

#endif
