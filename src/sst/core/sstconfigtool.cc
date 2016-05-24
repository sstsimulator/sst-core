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

	}

	SST::Core::Environment::EnvironmentConfiguration* database =
		new SST::Core::Environment::EnvironmentConfiguration();

	populateEnvironmentConfig( SST_INSTALL_PREFIX "/etc/sst/sstsimulator.conf", database,
		true );

	char* userHome = getenv("HOME");

	if( NULL == userHome ) {
		populateEnvironmentConfig( "~/.sst/sstsimulator.conf", database,
			false );
	} else {
		char* userHomeBuffer = (char*) malloc(sizeof(char) * PATH_MAX);
		sprintf(userHomeBuffer, "%s/.sst/sstsimulator.conf", userHome);

		populateEnvironmentConfig( userHomeBuffer, database, false );
		free(userHomeBuffer);
	}

	database->print();

	return 0;
}
