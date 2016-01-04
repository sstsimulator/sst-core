#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#include <map>

#include "sst_config.h"

void print_usage() {
	printf("======================================================\n");
	printf("SST %s Core Installation Configuration\n", PACKAGE_VERSION);
	printf("======================================================\n");
	printf("\n");
	printf("SST Install Prefix:   %s\n", SST_INSTALL_PREFIX);
	printf("Install Database:     %s/etc/sst/SST-%s.conf\n", SST_INSTALL_PREFIX, PACKAGE_VERSION);
	exit(1);
}

void readLine(FILE* config, char* buffer) {
	int bufferIndex = 0;

	while( ! feof(config) ) {
		buffer[bufferIndex] = (char) fgetc(config);

		if( buffer[bufferIndex] == '\n' || buffer[bufferIndex] == '\r') {
			buffer[bufferIndex] = '\0';
			break;
		}

		bufferIndex++;
	}
}

void populateConfigMap(FILE* conf_file,
	std::map<std::string, std::string>& confMap) {
	
	char* lineBuffer    = (char*) malloc(sizeof(char) * 4096);
	char* variableName  = (char*) malloc(sizeof(char) * 4096);
	char* variableValue = (char*) malloc(sizeof(char) * 4096);

	while( ! feof(conf_file) ) {
		readLine(conf_file, lineBuffer);

		variableName[0] = '\0';
		variableValue[0] = '\0';

		if(strcmp(lineBuffer, "") == 0) {
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

				confMap.insert( std::pair<std::string, std::string>(varNameStr, varValueStr) );
			} else {
				printf("SST Config: Badly formed line [%s], no assignment found.\n",
					lineBuffer);
			}
		}
	}
	
	free(lineBuffer);
	
}

int main(int argc, char* argv[]) {
	bool found_help = false;

	if(argc > 2) {
		print_usage();
	}

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "--help") == 0 ||
			strcmp(argv[i], "-help") == 0) {
			found_help = true;
			break;
		}
	}

	if(found_help) {
		print_usage();
	}

	std::map<std::string, std::string> configParams;

	char* conf_file_path = (char*) malloc(sizeof(char) * PATH_MAX);
	sprintf(conf_file_path, "%s/etc/sst/SST-%s.conf", SST_INSTALL_PREFIX, PACKAGE_VERSION);

	FILE* sst_conf_file = fopen(conf_file_path, "rt");
	if(NULL == sst_conf_file) {
		fprintf(stderr, "Unable to open the SST configuration database: %s\n", conf_file_path);
		fprintf(stderr, "Have you performed a build and install of SST?\n");
		exit(-1);
	}

	// Populate the main SST installed configuration
	populateConfigMap(sst_conf_file, configParams);

	fclose(sst_conf_file);
	
	sprintf(conf_file_path, "~/.sst/sst-%s.conf", PACKAGE_VERSION);
	FILE* sst_user_conf_file = fopen(conf_file_path, "rt");

	// User configuration file may not exist, so if we can't open that's fine
	if(NULL != sst_user_conf_file) {
		populateConfigMap(sst_user_conf_file, configParams);
		fclose(sst_user_conf_file);
	}
	
	free(conf_file_path);

	if( 1 == argc ) {
		for(auto configItr = configParams.begin(); configItr != configParams.end();
			configItr++) {
			
			printf("%30s %40s\n", configItr->first.c_str(), configItr->second.c_str());	
		}
	} else if( 0 == strcmp(argv[1], "--prefix") ) {
		printf("%s\n", SST_INSTALL_PREFIX);
	} else if( 0 == strcmp(argv[1], "--version") ) {
		printf("%s\n", PACKAGE_VERSION);
	} else if (0 == strcmp(argv[1], "--database") ) {
		printf("%s/etc/sst/SST-%s.conf\n", SST_INSTALL_PREFIX, PACKAGE_VERSION);
	} else {
		if( 0 == strncmp(argv[1], "--", 2) ) {
			char* param = argv[1];
			param++;
			param++;

			std::string paramStr(param);

			auto configParamsVal = configParams.find(paramStr);

			if(configParamsVal == configParams.end()) {
				printf("\n");
			} else {
				printf("%s\n", configParamsVal->second.c_str());
			}
		} else {
			fprintf(stderr, "Unknown parameter to find (%s), must start with --\n", argv[1]);
			exit(-1);
		}
	}

	return 0;
}
