
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
	EnvironmentConfigGroup(std::string name) : groupName(name) {}
	std::string getName() const;
	std::set<std::string> getKeys() const;
	std::string getValue(std::string key);
	void setValue(std::string key, std::string value);
	void print();
	void writeTo(FILE* outFile);

protected:
	std::string groupName;
	std::map<std::string, std::string> params;

};

class EnvironmentConfiguration {

public:
	EnvironmentConfiguration();
	~EnvironmentConfiguration();

	EnvironmentConfigGroup* createGroup(std::string groupName);
	void removeGroup(std::string groupName);
	std::set<std::string> getGroupNames();
	EnvironmentConfigGroup* getGroupByName(std::string groupName);
	void print();
	void writeTo(std::string filePath);
	void writeTo(FILE* outFile);

private:
	std::map<std::string, EnvironmentConfigGroup*> groups;

};

}
}
}

#endif
