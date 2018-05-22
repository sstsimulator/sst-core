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

#ifndef _H_SST_CORE_ENV_QUERY_H
#define _H_SST_CORE_ENV_QUERY_H

#include <sst_config.h>

#include <sst/core/env/envconfig.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>

namespace SST {
namespace Core {
namespace Environment {

/**
Reads the next new-line delimited entry in a file and put
into the buffer provided. The user is responsible for ensuring
that the buffer is of an appropriate length.
*/
void configReadLine(FILE* theFile, char* lineBuffer);

/**
Opens a configuration file specified and populates an
EnvironmentConfiguration instance with the contents.
*/
void populateEnvironmentConfig(const std::string& path, EnvironmentConfiguration* cfg,
	bool errorOnNotOpen);

/**
Uses an already open file, reads the contents and populates an instance
of an EnvironmentConfiguration with the contents
*/
void populateEnvironmentConfig(FILE* configFile, EnvironmentConfiguration* cfg,
	bool errorOnNotOpen);

/**
Provides an SST-guaranteed precedence ordering loading of configuration
files. The user can supply an override list of file paths which should
take precedence over the default configuration locations.
*/
EnvironmentConfiguration* getSSTEnvironmentConfiguration(const std::vector<std::string>& overridePaths);

}
}
}

#endif
