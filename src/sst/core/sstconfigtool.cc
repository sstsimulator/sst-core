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
	printf("Install Database:     %s/etc/sst/sstsimulator.conf\n", SST_INSTALL_PREFIX);
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
	SST::Core::populateConfigMap(configParams);

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
		printf("%s/etc/sst/sstsimulator.conf\n", SST_INSTALL_PREFIX);
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
