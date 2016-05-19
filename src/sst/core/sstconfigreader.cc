// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/sstconfigreader.hpp"

#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstring>

#include <string>

void SST::Core::configReadLine(FILE* config, char* buffer, const size_t bufferLen) {
	buffer[0] = '\0';

	// Start at index zero and move out from there
	int bufferIndex = 0;

	while( ! feof(config) ) {
		const int next = fgetc(config);

		if(EOF == next) {
			buffer[bufferIndex] = '\0';
			break;
		} else {
			const char nextChar = (char) next;

			if( '\n' == nextChar || '\r' == nextChar) {
				buffer[bufferIndex] = '\0';
				break;
			} else {
				buffer[bufferIndex] = nextChar;
			}

			bufferIndex++;
		}
	}
}

void SST::Core::populateConfigMapFromFile(FILE* conf_file,
	std::map<std::string, std::string>& confMap) {

	const size_t bufferLength = 4096;

	char* lineBuffer    = (char*) malloc(sizeof(char) * bufferLength);
	char* variableName  = (char*) malloc(sizeof(char) * bufferLength);
	char* variableValue = (char*) malloc(sizeof(char) * bufferLength);

	while( ! feof(conf_file) ) {
		configReadLine(conf_file, lineBuffer, bufferLength);

		variableName[0] = '\0';
		variableValue[0] = '\0';

		if(strlen(lineBuffer) == 0) {
			// Do nothing and move on
		} else if(lineBuffer[0] == '#') {
			// Comment line, do nothing move on
		} else if(lineBuffer[0] == '[') {
			// Section line, do nothing move on
		} else {
			const int lineLength = strlen(lineBuffer);
			bool foundAssign = false;

			for(int i = 0; i < lineLength; i++) {
				if(lineBuffer[i] == '=') {
					foundAssign = true;
					break;
				}
			}

			int variableIndex = 0;

			if(foundAssign) {
				foundAssign = false;

				for(int i = 0; i < lineLength; i++) {
					if(lineBuffer[variableIndex] != '\n' &&
						lineBuffer[variableIndex] != '\r') {

						if(foundAssign) {
							variableValue[variableIndex] = lineBuffer[i];
							variableIndex++;
						} else {
							if(lineBuffer[i] == '=') {
								variableName[variableIndex] = '\0';
								variableIndex = 0;
								foundAssign = true;
							} else {
								variableName[variableIndex] = lineBuffer[i];
								variableIndex++;
							}
						}
					}
				}

				variableValue[variableIndex] = '\0';

				std::string varNameStr(variableName);
				std::string varValueStr(variableValue);

				if(confMap.find(varNameStr) != confMap.end()) {
					confMap.erase(varNameStr);
				}

				confMap.insert( std::pair<std::string, std::string>(varNameStr, varValueStr) );
			} else {
				// No = in the line, so don't process it
			}
		}
	}

	free(lineBuffer);
}

void SST::Core::populateConfigMap(std::map<std::string, std::string>& confMap) {
	
	char* sstConfFilePath = (char*) malloc(sizeof(char) * PATH_MAX);
	sprintf(sstConfFilePath, "%s/etc/sst/sstsimulator.conf", SST_INSTALL_PREFIX);
		
	FILE* sstConfFile = fopen(sstConfFilePath, "rt");
	
	if( NULL == sstConfFile ) {
		fprintf(stderr, "ERROR: Unable to read from the SST configuration at %s.\n",
			sstConfFilePath);
	} else {
		populateConfigMapFromFile(sstConfFile, confMap);
	  fclose(sstConfFile);
	}
	
	char* userHome = getenv("HOME");
	
	if( NULL == userHome ) {
		sprintf(sstConfFilePath, "~/.sst/sstsimulator.conf");
	} else {
		sprintf(sstConfFilePath, "%s/.sst/sstsimulator.conf", userHome);
	}
	
	FILE* userConfFile = fopen(sstConfFilePath, "rt");
	
	if( NULL == userConfFile ) {
		// Don't worry about this, not every user will have a file.
	} else {
		populateConfigMapFromFile(userConfFile, confMap);
	  fclose(userConfFile);
	}
	free(sstConfFilePath);
}
