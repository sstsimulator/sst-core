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

#include <sst_config.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
#include <sys/file.h>

#include <sst/core/warnmacros.h>
#include "envquery.h"
#include "envconfig.h"

void SST::Core::Environment::configReadLine(FILE* theFile, char* lineBuffer) {
	lineBuffer[0] = '\0';

	// Start at index 0, move out each time we get a new character
	int bufferIndex = 0;

	while( std::feof(theFile) == 0 ) {
		const int next      = std::fgetc(theFile);
		const char nextChar = static_cast<char>(next);

		if(EOF == next) {
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
}

void SST::Core::Environment::populateEnvironmentConfig(FILE* configFile, EnvironmentConfiguration* cfg, bool UNUSED(errorOnNotOpen)) {

	// Get the file descriptor and lock the file using a shared lock so
	// people don't come and change it from under us
	int configFileFD = fileno(configFile);
	flock(configFileFD, LOCK_SH);

	constexpr size_t maxBufferLength = 4096;
	char* lineBuffer = (char*) malloc(sizeof(char) * maxBufferLength);
	for(size_t i = 0; i < maxBufferLength; i++) {
		lineBuffer[i] = '\0';
	}
	int currentLine = 0;

	EnvironmentConfigGroup* currentGroup = cfg->getGroupByName("default");

	while( feof(configFile) == 0 ) {
		SST::Core::Environment::configReadLine(configFile, lineBuffer);
		currentLine++;

		if(0 == strlen(lineBuffer)) {
			// Empty line, needs no additional processing
		} else if('#' == lineBuffer[0]) {
			// Comment line, throw this away, we don't need it
		} else if('[' == lineBuffer[0]) {
			if(lineBuffer[strlen(lineBuffer) - 1] != ']') {
				std::cerr << "SST: Error reading configuration file at line number: "
					<< currentLine << ", no matching ]"
					<< std::endl;
				exit(-1);
			}

			// This is a configuration group
			std::string lineStr(lineBuffer);

			// Grab the group by this name
			currentGroup = cfg->getGroupByName(lineStr.substr(1, lineStr.size() - 2));
		} else {
			std::string lineStr(lineBuffer);
			int equalsIndex = 0;

			for(size_t i = 0; i < strlen(lineBuffer); i++) {
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

	// Unlock since we are done processing it
	flock(configFileFD, LOCK_UN);
	free(lineBuffer);
}

void SST::Core::Environment::populateEnvironmentConfig(const std::string& path, EnvironmentConfiguration* cfg, bool errorOnNotOpen) {
	const char* configFilePath = path.c_str();
	FILE* configFile = fopen(configFilePath, "r");

	if(NULL == configFile) {
		if(errorOnNotOpen) {
			std::cerr << "SST: Unable to open configuration file \'" <<
				path << "\'" << std::endl;
			exit(-1);
		} else {
			return ;
		}
	}

	populateEnvironmentConfig(configFile, cfg, errorOnNotOpen);
}

SST::Core::Environment::EnvironmentConfiguration*
	SST::Core::Environment::getSSTEnvironmentConfiguration(const std::vector<std::string>& overridePaths) {
	EnvironmentConfiguration* envConfig = new EnvironmentConfiguration();

	// LOWEST PRIORITY - GLOBAL INSTALL CONFIG
	std::string prefixConfig = "";

	if( 0 == strcmp(SST_INSTALL_PREFIX, "NONE") ) {
		prefixConfig = "/usr/local/etc/sst/sstsimulator.conf";
	} else {
		prefixConfig = SST_INSTALL_PREFIX "/etc/sst/sstsimulator.conf";
	}

	SST::Core::Environment::populateEnvironmentConfig(prefixConfig, envConfig, true);

	// NEXT - USER HOME CONFIG
	char* homeConfigPath = (char*) malloc( sizeof(char) * PATH_MAX );
	char* userHome = getenv("HOME");

	if(NULL == userHome) {
		sprintf(homeConfigPath, "~/.sst/sstsimulator.conf");
	} else {
		sprintf(homeConfigPath, "%s/.sst/sstsimulator.conf", userHome);
	}

	const std::string homeConfigPathStr(homeConfigPath);
	populateEnvironmentConfig(homeConfigPathStr, envConfig, false);

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
			populateEnvironmentConfig(nextToken, envConfig, true);
			nextToken = strtok(NULL, envConfigPathSep);
		}

		free(envConfigPathBuffer);
	}

	// NEXT - override paths
	for(std::string nextPath : overridePaths) {
		populateEnvironmentConfig(nextPath, envConfig, true);
	}

	return envConfig;
}

