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

#include <climits>
#include <map>
#include <cstring>
#include <string>

#include "bootshared.h"
#include <sst/core/warnmacros.h>
#include "sst/core/env/envquery.h"

void update_env_var(const char* name, const int UNUSED(verbose), char* UNUSED(argv[]), const int UNUSED(argc)) {
        char* current_ld_path  = getenv(name);
	char* new_ld_path      = (char*) malloc(sizeof(char) * 32768);
	char* new_ld_path_copy = (char*) malloc(sizeof(char) * 32768);

	for(size_t i = 0; i < 32768; ++i) {
		new_ld_path[i] = '\0';
		new_ld_path_copy[i] = '\0';
	}

	if(NULL != current_ld_path) {
		sprintf(new_ld_path, "%s", current_ld_path);
	}
	std::vector<std::string> configFiles;
	SST::Core::Environment::EnvironmentConfiguration* envConfig = SST::Core::Environment::getSSTEnvironmentConfiguration(configFiles);
	std::set<std::string> groups = envConfig->getGroupNames();

	for(auto groupItr = groups.begin(); groupItr != groups.end(); groupItr++) {
		SST::Core::Environment::EnvironmentConfigGroup* currentGroup = envConfig->getGroupByName(*groupItr);

		std::set<std::string> keys = currentGroup->getKeys();

		for(auto configItr = keys.begin(); configItr != keys.end(); configItr++) {
			const std::string& paramName = *configItr;
			const std::string& value     = currentGroup->getValue(*configItr).c_str();

			if(paramName.size() >= 6) {
				const char* paramNameEnding = paramName.c_str();

				if(strcmp(&paramNameEnding[paramName.size() - 6], "LIBDIR") == 0) {
					strcpy(new_ld_path_copy, new_ld_path);
					sprintf(new_ld_path, "%s:%s", value.c_str(), new_ld_path_copy);
				}
			}
		}
	}

        // Override the exiting LD_LIBRARY_PATH with our updated variable
        setenv(name, new_ld_path, 1);
	free(new_ld_path_copy);

	// Set the SST_ROOT information
#ifdef SST_INSTALL_PREFIX
	// If SST_ROOT not previous set, then we will provide our own
	if(NULL == getenv("SST_ROOT")) {
		setenv("SST_ROOT", SST_INSTALL_PREFIX, 1);
	}
#endif
}

void boot_sst_configure_env(const int verbose, char* argv[], const int argc) {
	update_env_var("LD_LIBRARY_PATH", verbose, argv, argc);
        update_env_var("DYLD_LIBRARY_PATH", verbose, argv, argc);
}

void boot_sst_executable(const char* binary, const int verbose, char* argv[], const int UNUSED(argc)) {
	char* real_binary_path = (char*) malloc(sizeof(char) * PATH_MAX);

	if(strcmp(SST_INSTALL_PREFIX, "NONE") == 0) {
		sprintf(real_binary_path, "/usr/local/libexec/%s", binary);
	} else {
		sprintf(real_binary_path, "%s/libexec/%s", SST_INSTALL_PREFIX, binary);
	}

	if(verbose) {
		char** check_env = environ;
		while(NULL != check_env && NULL != check_env[0]) {
			printf("SST Environment Variable: %s\n", check_env[0]);
			check_env++;
		}
	}

	if(verbose) {
		printf("Launching SST executable (%s)...\n", real_binary_path);
	}
	
	// Flush standard out in case binary crashes
	fflush(stdout);

	int launch_sst = execve(real_binary_path, argv, environ);

	if(launch_sst == -1) {
		switch(errno) {
		case E2BIG:
			fprintf(stderr, "Unable to launch SST, the argument list is too long.\n");
			break;

		case EACCES:
			fprintf(stderr, "Unable to launch SST, part of the path does not have the appropriate read/search access permissions, check you can read the install location or the path is not an executable, did you install correctly?\n");
			break;

		case EFAULT:
			fprintf(stderr, "Unable to launch SST, the executable is corrupted. Please check your installation.\n");
			break;

		case EIO:
			fprintf(stderr, "Unable to launch SST, an error occurred in the I/O system reading the executable.\n");
			break;

		case ENAMETOOLONG:
			fprintf(stderr, "Unable to launch SST, the path to the executable exceeds the operating system maximum. Try installing to a shorter path.\n");
			break;

		case ENOENT:
			fprintf(stderr, "Unable to launch SST, the executable cannot be found. Did you install it correctly?\n");
			break;

		case ENOMEM:
			fprintf(stderr, "Unable to run SST, the program requested more virtual memory than is allowed in the machine limits. You may need to contact the system administrator to have this limit increased.\n");
			break;

		case ENOTDIR:
			fprintf(stderr, "Unable to launch SST, one part of the path to the executable is not a directory. Check the path and install prefix.\n");
			break;

		case ETXTBSY:
			fprintf(stderr, "Unable to launch SST, the executable file is open for writing/reading by another process.\n");
			break;
		}
	}
}
