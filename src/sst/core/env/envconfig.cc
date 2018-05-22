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
#include "sst/core/env/envconfig.h"

#include <set>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sys/file.h>

//SST::Core::Environment::EnvironmentConfigGroup(std::string gName) {
//	groupName(gName);
//}

std::string SST::Core::Environment::EnvironmentConfigGroup::getName() const {
	return groupName;
}

std::set<std::string> SST::Core::Environment::EnvironmentConfigGroup::getKeys() const {
	std::set<std::string> retKeys;

	for(auto mapItr = params.begin(); mapItr != params.end(); mapItr++) {
		retKeys.insert(mapItr->first);
	}

	return retKeys;
}

std::string SST::Core::Environment::EnvironmentConfigGroup::getValue(std::string key) {
	return (params.find(key) == params.end()) ?
		"" : params[key];
}

void SST::Core::Environment::EnvironmentConfigGroup::setValue(std::string key, std::string value) {
	auto paramsItr = params.find(key);

	if(paramsItr != params.end()) {
		params.erase(paramsItr);
	}

	params.insert(std::pair<std::string, std::string>(key, value));
}

void SST::Core::Environment::EnvironmentConfigGroup::print() {
	printf("# Group: %s ", groupName.c_str());

	int remainingLen = 70 - groupName.size();

	if(remainingLen < 0) {
		remainingLen = 0;
	}

	for( ; remainingLen >= 0; remainingLen--) {
		printf("-");
	}

	printf("\n");

	for(auto paramsItr = params.begin(); paramsItr != params.end(); paramsItr++) {
		printf("%s=%s\n", paramsItr->first.c_str(), paramsItr->second.c_str());
	}
}

void SST::Core::Environment::EnvironmentConfigGroup::writeTo(FILE* outFile) {
	fprintf(outFile, "[%s]\n", groupName.c_str());

	for(auto paramsItr = params.begin(); paramsItr != params.end(); paramsItr++) {
		fprintf(outFile, "%s=%s\n", paramsItr->first.c_str(), paramsItr->second.c_str());
	}
}

SST::Core::Environment::EnvironmentConfiguration::EnvironmentConfiguration() {}

SST::Core::Environment::EnvironmentConfiguration::~EnvironmentConfiguration() {
	// Delete all the groups we have created
	for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
		delete groupItr->second;
	}
}

SST::Core::Environment::EnvironmentConfigGroup* SST::Core::Environment::EnvironmentConfiguration::createGroup(std::string groupName) {
	EnvironmentConfigGroup* newGroup = NULL;

	if(groups.find(groupName) == groups.end()) {
		newGroup = new EnvironmentConfigGroup(groupName);
		groups.insert(std::pair<std::string, EnvironmentConfigGroup*>(groupName, newGroup));
	} else {
		newGroup = groups.find(groupName)->second;
	}

	return newGroup;
}

void SST::Core::Environment::EnvironmentConfiguration::removeGroup(std::string groupName) {
	auto theGroup = groups.find(groupName);

	if(theGroup != groups.end()) {
		groups.erase(theGroup);
	}
}

std::set<std::string> SST::Core::Environment::EnvironmentConfiguration::getGroupNames() {
	std::set<std::string> groupNames;

	for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
		groupNames.insert(groupItr->first);
	}

	return groupNames;
}

SST::Core::Environment::EnvironmentConfigGroup* SST::Core::Environment::EnvironmentConfiguration::getGroupByName(std::string groupName) {
	return createGroup(groupName);
}

void SST::Core::Environment::EnvironmentConfiguration::print() {
	for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
		groupItr->second->print();
	}
}

void SST::Core::Environment::EnvironmentConfiguration::writeTo(std::string filePath) {
	FILE* output = fopen(filePath.c_str(), "w+");

	if(NULL == output) {
		fprintf(stderr, "Unable to open file: %s\n", filePath.c_str());
		exit(-1);
	}

	const int outputFD = fileno(output);

	// Lock the file because we are going to write it out (this should be
	// exclusive since no one else should muck with it)
	flock(outputFD, LOCK_EX);

	for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
		groupItr->second->writeTo(output);
	}

	flock(outputFD, LOCK_UN);
	fclose(output);
}

void SST::Core::Environment::EnvironmentConfiguration::writeTo(FILE* output) {
	for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
		groupItr->second->writeTo(output);
	}
}
