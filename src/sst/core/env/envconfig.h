
#ifndef _H_SST_CORE_ENV_CONFIG_H
#define _H_SST_CORE_ENV_CONFIG_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <climits>

namespace SST {
namespace Core {
namespace Environment {

class EnvironmentConfigGroup {

public:
	EnvironmentConfigGroup(const std::string name) :
		groupName(name) {}

	std::string getName() const {
		return groupName;
	}

	std::set<std::string> getKeys() const {
		std::set<std::string> retKeys;

		for(auto mapItr = params.begin(); mapItr != params.end(); mapItr++) {
			retKeys.insert(mapItr->first);
		}

		return retKeys;
	}

	std::string getValue(std::string key) {
		return (params.find(key) == params.end()) ?
			"" : params[key];
	}

	void setValue(std::string key, std::string value) {
		params.insert(std::pair<std::string, std::string>(key, value));
	}

	void print() {
		std::cout << "Group: " << groupName << std::endl;

		for(auto paramsItr = params.begin(); paramsItr != params.end(); paramsItr++) {
			std::cout << paramsItr->first << "=" << paramsItr->second << std::endl;
		}
	}

	void writeTo(FILE* outFile) {
		fprintf(outFile, "[%s]\n", groupName.c_str());

		for(auto paramsItr = params.begin(); paramsItr != params.end(); paramsItr++) {
			fprintf(outFile, "%s=%s\n", paramsItr->first.c_str(), paramsItr->second.c_str());
		}
	}

protected:
	std::string groupName;
	std::map<std::string, std::string> params;

};

class EnvironmentConfiguration {

public:
	EnvironmentConfiguration() {}
	~EnvironmentConfiguration() {
		// Delete all the groups we have created
		for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
			delete groupItr->second;
		}
	}

	EnvironmentConfigGroup* createGroup(std::string groupName) {
		EnvironmentConfigGroup* newGroup = NULL;

		if(groups.find(groupName) == groups.end()) {
			newGroup = new EnvironmentConfigGroup(groupName);
			groups.insert(std::pair<std::string, EnvironmentConfigGroup*>(groupName, newGroup));
		} else {
			newGroup = groups.find(groupName)->second;
		}

		return newGroup;
	}

	void removeGroup(std::string groupName) {
		auto theGroup = groups.find(groupName);

		if(theGroup != groups.end()) {
			groups.erase(theGroup);
		}
	}

	std::set<std::string> getGroupNames() {
		std::set<std::string> groupNames;

		for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
			groupNames.insert(groupItr->first);
		}

		return groupNames;
	}

	EnvironmentConfigGroup* getGroupByName(std::string groupName) {
		return createGroup(groupName);
	}

	void print() {
		for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
			groupItr->second->print();
		}
	}

	void writeTo(std::string filePath) {
		FILE* output = fopen(filePath.c_str(), "wt");

		for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
			groupItr->second->writeTo(output);
		}

		fclose(output);
	}

private:
	std::map<std::string, EnvironmentConfigGroup*> groups;

};

}
}
}

#endif
