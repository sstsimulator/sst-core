// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "bootshared.h"

#include "sst/core/env/envquery.h"
#include "sst/core/warnmacros.h"

#include <climits>
#include <cstring>
#include <map>
#include <string>

void
// update_env_var(const char* name, const int UNUSED(verbose), char* UNUSED(argv[]), const int UNUSED(argc))

// This will add the current load path variable to the end of path and
// set it as the new load path for the actual executable
update_env_var(const char* name, const std::string& path)
{
    char* current_ld_path = getenv(name);
    char* new_ld_path     = (char*)malloc(sizeof(char) * 32768);

    // Put in the passed in path first
    if ( current_ld_path == nullptr ) {
        // No current load path, just put in path
        snprintf(new_ld_path, 32768, "%s", path.c_str());
    }
    else {
        snprintf(new_ld_path, 32768, "%s:%s", path.c_str(), current_ld_path);
    }

    // Override the exiting load path with our updated variable
    setenv(name, new_ld_path, 1);

    // Set the SST_ROOT information
#ifdef SST_INSTALL_PREFIX
    // If SST_ROOT not previous set, then we will provide our own
    if ( nullptr == getenv("SST_ROOT") ) { setenv("SST_ROOT", SST_INSTALL_PREFIX, 1); }
#endif
    free(new_ld_path);
}

void
boot_sst_configure_env(const std::string& path)
{
    update_env_var("LD_LIBRARY_PATH", path);
    update_env_var("DYLD_LIBRARY_PATH", path);
}

void
boot_sst_executable(const char* binary, const int verbose, char* argv[], const int UNUSED(argc))
{
    char* real_binary_path = (char*)malloc(sizeof(char) * PATH_MAX);

    if ( strcmp(SST_INSTALL_PREFIX, "NONE") == 0 ) {
        snprintf(real_binary_path, PATH_MAX, "/usr/local/libexec/%s", binary);
    }
    else {
        snprintf(real_binary_path, PATH_MAX, "%s/libexec/%s", SST_INSTALL_PREFIX, binary);
    }

    if ( verbose ) {
        char** check_env = environ;
        while ( nullptr != check_env && nullptr != check_env[0] ) {
            printf("SST Environment Variable: %s\n", check_env[0]);
            check_env++;
        }
    }

    if ( verbose ) { printf("Launching SST executable (%s)...\n", real_binary_path); }

    // Flush standard out in case binary crashes
    fflush(stdout);

    int launch_sst = execve(real_binary_path, argv, environ);

    if ( launch_sst == -1 ) {
        switch ( errno ) {
        case E2BIG:
            fprintf(stderr, "Unable to launch SST, the argument list is too long.\n");
            break;

        case EACCES:
            fprintf(
                stderr, "Unable to launch SST, part of the path does not have the appropriate read/search access "
                        "permissions, check you can read the install location or the path is not an executable, did "
                        "you install correctly?\n");
            break;

        case EFAULT:
            fprintf(stderr, "Unable to launch SST, the executable is corrupted. Please check your installation.\n");
            break;

        case EIO:
            fprintf(stderr, "Unable to launch SST, an error occurred in the I/O system reading the executable.\n");
            break;

        case ENAMETOOLONG:
            fprintf(
                stderr, "Unable to launch SST, the path to the executable exceeds the operating system maximum. Try "
                        "installing to a shorter path.\n");
            break;

        case ENOENT:
            fprintf(stderr, "Unable to launch SST, the executable cannot be found. Did you install it correctly?\n");
            break;

        case ENOMEM:
            fprintf(
                stderr, "Unable to run SST, the program requested more virtual memory than is allowed in the machine "
                        "limits. You may need to contact the system administrator to have this limit increased.\n");
            break;

        case ENOTDIR:
            fprintf(
                stderr, "Unable to launch SST, one part of the path to the executable is not a directory. Check the "
                        "path and install prefix.\n");
            break;

        case ETXTBSY:
            fprintf(
                stderr, "Unable to launch SST, the executable file is open for writing/reading by another process.\n");
            break;
        }
    }
}
