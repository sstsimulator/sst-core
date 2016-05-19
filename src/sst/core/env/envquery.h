
#include <sst_config.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>

#include "envconfig.h"

namespace SST {
namespace Core {
namespace Environment {

void configReadLine(FILE* theFile, char* lineBuffer) {
	lineBuffer[0] = '\0';

	// Start at index 0, move out each time we get a new character
	int bufferIndex = 0;

	while( ! feof(theFile) ) {
		const int nextChar = fgetc(theFile);

		if(EOF == nextChar) {
			lineBuffer[bufferIndex] = '\0';
			break;
		} else {
			const char nextCharAsChar = (char) nextChar;

			if( '\n' == nextCharAsChar || '\r' == nextCharAsChar ) {
				lineBuffer[bufferIndex] = '\0';
				break;
			} else {
				lineBuffer[bufferIndex] = nextCharAsChar;
			}
		}

		bufferIndex++;
	}
}

void populateEnvironmentConfig(const std::string& path, EnvironmentConfiguration* cfg) {
	FILE* configFile = fopen(path.c_str(), "rt");

	if(NULL == configFile) {
		std::cerr << "SST: Unable to open configuration file \'" <<
			path << "\'" << std::endl;
		exit(-1);
	}

	constexpr size_t maxBufferLength = 4096;
	char* lineBuffer = (char*) malloc(sizeof(char) * maxBufferLength);
	int currentLine = 0;

	EnvironmentConfigGroup* currentGroup = cfg->getGroupByName("default");

	while( ! feof(configFile) ) {
		SST::Core::Environment::configReadLine(configFile, lineBuffer);
		currentLine++;

		if(0 == strlen(lineBuffer)) {
			// Empty line, needs no additional processing
		} else if('#' == lineBuffer[0]) {
			// Comment line, throw this away, we don't need it
		} else if('[' == lineBuffer[0]) {
			if(lineBuffer[strlen(lineBuffer) - 1] != ']') {
				std::cerr << "SST: Error reading configuration file (" << path
					<< "), line number: " << currentLine << ", no matching ]"
					<< std::endl;
				exit(-1);
			}

			// This is a configuration group
			std::string lineStr(lineBuffer);

			// Grab the group by this name
			currentGroup = cfg->getGroupByName(lineStr.substr(1, lineStr.size() - 1));
		} else {
			std::string lineStr(lineBuffer);
			int equalsIndex = 0;

			for(int i = 0; i < strlen(lineBuffer); i++) {
				if('=' == lineBuffer[i]) {
					equalsIndex = i;
					break;
				}
			}

			std::string key = lineStr.substr(0, equalsIndex);
			std::string value = lineStr.substr(equalsIndex + 1);

			currentGroup->setValue(key, value);
		}
	}

	free(lineBuffer);
	fclose(configFile);
}

EnvironmentConfiguration* getSSTEnvironmentConfiguration(const std::vector<std::string>& overridePaths) {
	EnvironmentConfiguration* envConfig = new EnvironmentConfiguration();

	// LOWEST PRIORITY - GLOBAL INSTALL CONFIG
	std::string prefixConfig = SST_INSTALL_PREFIX "/etc/sst/sstsimulator.conf";

	SST::Core::Environment::populateEnvironmentConfig(prefixConfig, envConfig);

	// NEXT - USER HOME CONFIG
	char* homeConfigPath = (char*) malloc( sizeof(char) * PATH_MAX );
	char* userHome = getenv("HOME");

	if(NULL == userHome) {
		sprintf(homeConfigPath, "~/.sst/sstsimulator.conf");
	} else {
		sprintf(homeConfigPath, "%s/.sst/sstsimulator.conf", userHome);
	}

	const std::string homeConfigPathStr(homeConfigPath);
	populateEnvironmentConfig(homeConfigPathStr, envConfig);

	free(homeConfigPath);

	// NEXT - ENVIRONMENT VARIABLE
	const char* envConfigPaths = getenv("SST_CONFIG_FILE_PATH");
	const char* envConfigPathSep = getenv("SST_CONFIG_FILE_PATH_SEPARATOR");
	char* envConfigPathBuffer = (char*) malloc( sizeof(char) * (strlen(envConfigPaths) + 1) );
	strcpy(envConfigPathBuffer, envConfigPaths);

	if( NULL != envConfigPaths ) {
		char* nextToken = strtok(envConfigPathBuffer, envConfigPathSep);

		while( NULL != nextToken ) {
			populateEnvironmentConfig(nextToken, envConfig);
			nextToken = strtok(NULL, envConfigPathSep);
		}
	}

	// NEXT - override paths
	for(std::string nextPath : overridePaths) {
		populateEnvironmentConfig(nextPath, envConfig);
	}

	free(envConfigPathBuffer);
	return envConfig;
}

}
}
}
