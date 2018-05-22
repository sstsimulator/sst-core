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

/***
\class EnvironmentConfigGroup envconfig.h "sst/core/env/envconfig.h"

Specifies a class which contains a group of key=value pairs. A group
is a logical unit of management for collecting configuration parameters.
For instance, these might relate to an specific dependency or element
etc.

Grouping of key=value pairs allows entire lists of parameters to be
removed from the configuration system when needed.

In general the core expects to default to either the "default" group
or the "SSTCore" group, with the configuration populating the "SSTCore"
group with parameters during the initial configure script process.

The configuration and management systems treat the "default" and
"SSTCore" groups with special handling and no guarantee is provided
that these settings can be modified, removed or added to (since they
may be held in configuration files that are not accessible to the
user).

Groups also allow printing (to standard output) or files directly
of all members.

Although a logical interface may be the utilization of a map, groups
do not expose this currently to allow for modification in the
implementation in the future when more complex schemes are expected
to be used. Users of the group class should use the public accessor
methods provided to ensure forwards compatibility with future
SST releases.

*/

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


/***
\class EnvironmentConfiguration envconfig.h "sst/core/env/envconfig.h"

The EnvironmentConfiguration class provides an entire configuration
set for SST, which will include zero or more EnvironmentConfigGroup
instances (essentially zero or more groups). In the usual course of
operations the environment will contains two "special" groups called
"default" (unpopulated but is provided for any ungrouped parameter
values) and "SSTCore" which houses parameters encoded during the
core's configuration process.

EnvironmentConfiguration instances can be loaded from a single file
or via the standard precedence ordered SST loading mechanisms. In the
case of loading from a specific file, the core makes no guarantees
about the number of groups provided (zero or more). The user should
make no assumptions in this case.

If the EnvironmentConfiguration is loaded via the SST precedence
ordered mechanisms, then the "default" and "SSTCore" groups will be
provided.

Although it may seem logical to provide a map structure of group
names to group instances, the design is not implemented this way
to permit future modification in the underlying structures. Users
should use the public methods provided to ensure future compatibility
with SST releases.

*/
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
