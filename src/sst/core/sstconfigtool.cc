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

#include "sst/core/env/envquery.h"
#include "sst/core/env/envconfig.h"

void print_usage() {
	exit(1);
}

int main(int argc, char* argv[]) {
	bool found_help = false;

	std::string groupName("");
	std::string key("");

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

	if(argc == 2) {
		groupName = static_cast<std::string>("SSTCore");
		std::string keyTemp(argv[1]);

		if(keyTemp.size() < 2) {
			fprintf(stderr, "Error: key (%s) is not specified with a group and doesn't start with --\n", keyTemp.c_str());
			exit(-1);
		}

		if(keyTemp.substr(0, 2) == "--") {
			key = keyTemp.substr(2);
		} else {
			fprintf(stderr, "Error: key (%s) is not specified with a group and doesn't start with --\n", keyTemp.c_str());
			exit(-1);
		}
	} else if (argc == 3) {
		groupName = static_cast<std::string>(argv[1]);
		key       = static_cast<std::string>(argv[2]);
	} else {
		print_usage();
	}

	std::vector<std::string> overrideConfigFiles;
	SST::Core::Environment::EnvironmentConfiguration* database =
		SST::Core::Environment::getSSTEnvironmentConfiguration(overrideConfigFiles);

	populateEnvironmentConfig( SST_INSTALL_PREFIX "/etc/sst/sstsimulator.conf", database,
		true );

	SST::Core::Environment::EnvironmentConfigGroup* group = database->getGroupByName(groupName);

	std::set<std::string> groupKeys = group->getKeys();
	bool keyFound = false;

	for(auto keyItr = groupKeys.begin(); keyItr != groupKeys.end(); keyItr++) {
		if( key == (*keyItr) ) {
			printf("%s\n", group->getValue(key).c_str());
			keyFound = true;
			break;
		}
	}

	delete database;

	// If key is found, we return 0 otherwise 1 indicates not found
	return keyFound ? 0 : 1;
}
