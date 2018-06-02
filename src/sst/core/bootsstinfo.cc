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

#include <sst_config.h>
#include <sst/core/bootshared.h>

int main(int argc, char* argv[]) {
	int config_env = 1;
	int verbose = 0;

	for(int i = 0; i < argc; ++i) {
		if(strcmp("--no-env-config", argv[i]) == 0) {
			config_env = 0;
		} else if(strcmp("--verbose", argv[i]) == 0) {
			verbose = 1;
		}
	}

	if(verbose && config_env) {
		printf("Launching SST with automatic environment processing enabled...\n");
	}

	if(config_env) {
		boot_sst_configure_env(verbose, argv, argc);
	}

	boot_sst_executable("sstinfo.x", verbose, argv, argc);
}
