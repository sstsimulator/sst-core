
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

private:
	std::map<std::string, EnvironmentConfigGroup*> groups;

};

}
}
}
