// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/bootshared.h"
#include "sst/core/configShared.h"

#include <cstdlib>
#include <unistd.h> // for opterr

int
main(int argc, char* argv[])
{
    int config_env = 1;
    int print_env  = 0;
    int verbose    = 0;

    // Create a ConfigShred object.  This object won't print any error
    // messages about unknown command line options, that will be
    // deferred to the actual sstsim.x executable.
    SST::ConfigShared cfg(true, true, true, true, true);

    // Make a copy of the argv array (shallow)
    char* argv_copy[argc + 1];
    for ( int i = 0; i < argc; ++i ) {
        argv_copy[i] = argv[i];
    }
    argv[argc] = nullptr;

    // All ranks parse the command line
    cfg.parseCmdLine(argc, argv_copy);

    if ( cfg.no_env_config() ) config_env = 0;
    if ( cfg.verbose() ) verbose = 1;
    if ( cfg.print_env() ) print_env = 1;

    if ( print_env != 1 ) {
        const char* check_print_env   = std::getenv("SST_PRINT_ENV");
        const char* check_display_env = std::getenv("SST_DISPLAY_ENV");

        if ( nullptr != check_display_env ) {
            if ( strcmp("1", check_display_env) == 0 ) { print_env = 1; }
        }

        if ( nullptr != check_print_env ) {
            if ( strcmp("1", check_print_env) == 0 ) { print_env = 1; }
        }
    }

    if ( verbose && config_env ) { printf("Launching SST with automatic environment processing enabled...\n"); }

    if ( config_env ) { boot_sst_configure_env(cfg.getLibPath()); }

    if ( 1 == print_env ) {
        int next_index = 0;

        while ( nullptr != environ[next_index] ) {
            const char* next_env = environ[next_index];
            printf("%s\n", next_env);

            next_index++;
        }
    }

    boot_sst_executable("sstsim.x", verbose, argv, argc);
}
