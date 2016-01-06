#include "sst_config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#include <map>

#include "sstconfigreader.hpp"

void print_usage() {
	printf("======================================================\n");
	printf("SST %s Core Installation Configuration\n", PACKAGE_VERSION);
	printf("======================================================\n");
	printf("\n");
	printf("SST Install Prefix:   %s\n", SST_INSTALL_PREFIX);
	printf("Install Database:     %s/etc/sst/SST-%s.conf\n", SST_INSTALL_PREFIX, PACKAGE_VERSION);
	exit(1);
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
	SST::Core::populateConfigMap(sst_conf_file, configParams);

	fclose(sst_conf_file);

	const char* user_home = getenv("HOME");

	if(NULL == user_home) {
		sprintf(conf_file_path, "~/.sst/SST-%s.conf", PACKAGE_VERSION);
	} else {
		sprintf(conf_file_path, "%s/.sst/SST-%s.conf", user_home, PACKAGE_VERSION);
	}

	FILE* sst_user_conf_file = fopen(conf_file_path, "rt");

	// User configuration file may not exist, so if we can't open that's fine
	if(NULL != sst_user_conf_file) {
		SST::Core::populateConfigMap(sst_user_conf_file, configParams);
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
