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

#include "sst_config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#include <map>

#include "sst/core/env/envquery.h"
#include "sst/core/env/envconfig.h"

void print_usage(FILE* output) {
	fprintf(output, "sst-config\n");
	fprintf(output, "sst-config --<KEY>\n");
	fprintf(output, "sst-config <GROUP> <KEY>\n");
	fprintf(output, "\n");
	fprintf(output, "<GROUP>    Name of group to which the key belongs\n");
	fprintf(output, "           (e.g. DRAMSim group contains all DRAMSim\n");
	fprintf(output, "           KEY=VALUE settings).\n");
	fprintf(output, "<KEY>      Name of the setting key to find.\n");
	fprintf(output, "           If <GROUP> not specified this is found in\n");
	fprintf(output, "           the \'SSTCore\' default group.\n");
	fprintf(output, "\n");
	fprintf(output, "Example 1:\n");
	fprintf(output, "  sst-config --CXX\n");
	fprintf(output, "           Finds the CXX compiler specified by the core\n");
	fprintf(output, "Example 2:\n");
	fprintf(output, "  sst-config DRAMSim CPPFLAGS\n");
	fprintf(output, "           Finds CPPFLAGS associated with DRAMSim\n");
	fprintf(output, "Example 3:\n");
	fprintf(output, "  sst-config\n");
	fprintf(output, "           Dumps entire configuration found.\n");
	fprintf(output, "\n");
	fprintf(output, "The use of -- for the single <KEY> (Example 1) is\n");
	fprintf(output, "intentional to closely replicate behaviour of the\n");
	fprintf(output, "pkg-config tool used in Linux environments. This\n");
	fprintf(output, "should not be specified when using <GROUP> as well.\n");
	fprintf(output, "\n");
	fprintf(output, "Return: 0 is key found, 1 key/group not found\n");
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
		print_usage(stdout);
	}

	bool dumpEnv = false;

	if(argc == 1) {
		dumpEnv = true;
	} else if(argc == 2) {
		groupName = static_cast<std::string>("SSTCore");
		std::string keyTemp(argv[1]);

		if(keyTemp.size() < 2) {
			fprintf(stderr, "Error: key (%s) is not specified with a group and doesn't start with --\n", keyTemp.c_str());
			print_usage(stderr);
			exit(-1);
		}

		if(keyTemp.substr(0, 2) == "--") {
			key = keyTemp.substr(2);
		} else {
			fprintf(stderr, "Error: key (%s) is not specified with a group and doesn't start with --\n", keyTemp.c_str());
			print_usage(stderr);
			exit(-1);
		}
	} else if (argc == 3) {
		groupName = static_cast<std::string>(argv[1]);
		key       = static_cast<std::string>(argv[2]);
	} else {
		fprintf(stderr, "Error: you specified an incorrect number of parameters\n");
		print_usage(stderr);
	}

	std::vector<std::string> overrideConfigFiles;
	SST::Core::Environment::EnvironmentConfiguration* database =
		SST::Core::Environment::getSSTEnvironmentConfiguration(overrideConfigFiles);
	bool keyFound = false;

	if(dumpEnv) {
		database->print();
		exit(0);
	} else {
		SST::Core::Environment::EnvironmentConfigGroup* group =
			database->getGroupByName(groupName);

		std::set<std::string> groupKeys = group->getKeys();

		for(auto keyItr = groupKeys.begin(); keyItr != groupKeys.end(); keyItr++) {
			if( key == (*keyItr) ) {
				printf("%s\n", group->getValue(key).c_str());
				keyFound = true;
				break;
			}
		}
	}

	delete database;

	// If key is found, we return 0 otherwise 1 indicates not found
	return keyFound ? 0 : 1;
}
