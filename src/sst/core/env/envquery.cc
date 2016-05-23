
#include <sst_config.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>

#include "envquery.h"
#include "envconfig.h"

void SST::Core::Environment::configReadLine(FILE* theFile, char* lineBuffer) {
	lineBuffer[0] = '\0';

	// Start at index 0, move out each time we get a new character
	int bufferIndex = 0;

	while( std::feof(theFile) == 0 ) {
		const int next      = std::fgetc(theFile);
		const char nextChar = static_cast<char>(next);

		printf("read character: %d %c\n", next, nextChar);

		if(EOF == nextChar) {
			printf("Line ends at index %d\n", bufferIndex);
			lineBuffer[bufferIndex] = '\0';
			break;
		} else {
			if( '\n' == nextChar || '\r' == nextChar ) {
				lineBuffer[bufferIndex] = '\0';
				break;
			} else {
				lineBuffer[bufferIndex] = nextChar;
			}
		}

		bufferIndex++;
	}

	for(auto i = 0; i < 10; i++) {
		printf("Char[%d]=%c\n", i, lineBuffer[i]);
	}

	printf("The line: %s\n", lineBuffer);
}

void SST::Core::Environment::populateEnvironmentConfig(const std::string& path, EnvironmentConfiguration* cfg) {
	printf("Opening [%s]\n", path.c_str());
	FILE* configFile = fopen(path.c_str(), "r");

	if(NULL == configFile) {
		std::cerr << "SST: Unable to open configuration file \'" <<
			path << "\'" << std::endl;
		exit(-1);
	} else {
		printf("Successfully opened file!\n");
	}

	// Start at the beginning
	rewind(configFile);

	if(ferror(configFile)) {
		printf("ERROR ON THE CONFIGURATION FILE\n");
	} else {
		printf("NO ERROR ON THE CONFIG FILE, CONTINUE..,\n");
	}

	if(feof(configFile)) {
		printf("already at the end of the configuration file\n");
	} else {
		printf("NOT at the end of the file yet.\n");
	}

	constexpr size_t maxBufferLength = 4096;
	char* lineBuffer = (char*) malloc(sizeof(char) * maxBufferLength);
	for(auto i = 0; i < maxBufferLength; i++) {
		lineBuffer[i] = '\0';
	}
	int currentLine = 0;

	EnvironmentConfigGroup* currentGroup = cfg->getGroupByName("default");

	while( feof(configFile) == 0 ) {
		printf("LINE BUFFER BEFORE CALL: [%s]\n", lineBuffer);
		SST::Core::Environment::configReadLine(configFile, lineBuffer);
		currentLine++;

		printf("Line %5d = [%s]\n", currentLine, lineBuffer);

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

			std::cout << "Changing to group: " << lineStr << std::endl;

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

SST::Core::Environment::EnvironmentConfiguration*
	SST::Core::Environment::getSSTEnvironmentConfiguration(const std::vector<std::string>& overridePaths) {
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

	// If this isn't specified by the environment, we will default to UNIX ":" to be
	// safe.
	if( NULL == envConfigPathSep ) {
		envConfigPathSep = ":";
	}

	if( NULL != envConfigPaths ) {
		char* envConfigPathBuffer = (char*) malloc( sizeof(char) * (strlen(envConfigPaths) + 1) );
		strcpy(envConfigPathBuffer, envConfigPaths);

		char* nextToken = strtok(envConfigPathBuffer, envConfigPathSep);

		while( NULL != nextToken ) {
			populateEnvironmentConfig(nextToken, envConfig);
			nextToken = strtok(NULL, envConfigPathSep);
		}

		free(envConfigPathBuffer);
	}

	// NEXT - override paths
	for(std::string nextPath : overridePaths) {
		populateEnvironmentConfig(nextPath, envConfig);
	}

	return envConfig;
}

