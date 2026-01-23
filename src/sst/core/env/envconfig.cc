// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/env/envconfig.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <sys/file.h>
#include <utility>
#include <vector>

// SST::Core::Environment::EnvironmentConfigGroup(std::string gName) {
//    groupName(gName);
//}

std::string
SST::Core::Environment::EnvironmentConfigGroup::getName() const
{
    return groupName;
}

std::set<std::string>
SST::Core::Environment::EnvironmentConfigGroup::getKeys() const
{
    std::set<std::string> retKeys;

    for ( const auto& param : params ) {
        retKeys.insert(param.first);
    }

    return retKeys;
}

std::string
SST::Core::Environment::EnvironmentConfigGroup::getValue(const std::string& key)
{
    return (params.find(key) == params.end()) ? "" : params[key];
}

void
SST::Core::Environment::EnvironmentConfigGroup::setValue(const std::string& key, const std::string& value)
{
    auto paramsItr = params.find(key);

    if ( paramsItr != params.end() ) {
        params.erase(paramsItr);
    }

    params.insert(std::pair<std::string, std::string>(key, value));
}

void
SST::Core::Environment::EnvironmentConfigGroup::print()
{
    printf("# Group: %s ", groupName.c_str());

    int remainingLen = 70 - groupName.size();

    if ( remainingLen < 0 ) {
        remainingLen = 0;
    }

    for ( ; remainingLen >= 0; remainingLen-- ) {
        printf("-");
    }

    printf("\n");

    for ( auto& param : params ) {
        printf("%s=%s\n", param.first.c_str(), param.second.c_str());
    }
}

void
SST::Core::Environment::EnvironmentConfigGroup::writeTo(FILE* outFile)
{
    fprintf(outFile, "\n[%s]\n", groupName.c_str());

    for ( auto& param : params ) {
        fprintf(outFile, "%s=%s\n", param.first.c_str(), param.second.c_str());
    }
}

SST::Core::Environment::EnvironmentConfiguration::~EnvironmentConfiguration()
{
    // Delete all the groups we have created
    for ( auto& group : groups ) {
        delete group.second;
    }
}

SST::Core::Environment::EnvironmentConfigGroup*
SST::Core::Environment::EnvironmentConfiguration::createGroup(const std::string& groupName)
{
    EnvironmentConfigGroup* newGroup = nullptr;

    if ( groups.find(groupName) == groups.end() ) {
        newGroup = new EnvironmentConfigGroup(groupName);
        groups.insert(std::pair<std::string, EnvironmentConfigGroup*>(groupName, newGroup));
    }
    else {
        newGroup = groups.find(groupName)->second;
    }

    return newGroup;
}

void
SST::Core::Environment::EnvironmentConfiguration::removeGroup(const std::string& groupName)
{
    auto theGroup = groups.find(groupName);

    if ( theGroup != groups.end() ) {
        groups.erase(theGroup);
    }
}

std::set<std::string>
SST::Core::Environment::EnvironmentConfiguration::getGroupNames()
{
    std::set<std::string> groupNames;

    for ( auto& group : groups ) {
        groupNames.insert(group.first);
    }

    return groupNames;
}

SST::Core::Environment::EnvironmentConfigGroup*
SST::Core::Environment::EnvironmentConfiguration::getGroupByName(const std::string& groupName)
{
    return createGroup(groupName);
}

void
SST::Core::Environment::EnvironmentConfiguration::print()
{
    for ( auto& group : groups ) {
        group.second->print();
    }
}

void
SST::Core::Environment::EnvironmentConfiguration::writeTo(const std::string& filePath)
{
    FILE* output = fopen(filePath.c_str(), "w+");

    if ( nullptr == output ) {
        fprintf(stderr, "Unable to open file: %s\n", filePath.c_str());
        exit(-1);
    }

    const int outputFD = fileno(output);

    // Lock the file because we are going to write it out (this should be
    // exclusive since no one else should muck with it)
    flock(outputFD, LOCK_EX);

    for ( auto& group : groups ) {
        group.second->writeTo(output);
    }

    flock(outputFD, LOCK_UN);
    fclose(output);
}

void
SST::Core::Environment::EnvironmentConfiguration::writeTo(FILE* output)
{
    for ( auto& group : groups ) {
        group.second->writeTo(output);
    }
}
