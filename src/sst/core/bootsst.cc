// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/bootshared.h"

#include <cstdlib>

int
main(int argc, char* argv[])
{
    int config_env = 1;
    int print_env  = 0;
    int verbose    = 0;

    for ( int i = 0; i < argc; ++i ) {
        if ( strcmp("--no-env-config", argv[i]) == 0 ) { config_env = 0; }
        else if ( strcmp("--verbose", argv[i]) == 0 ) {
            verbose = 1;
        }
        else if ( strcmp("--print-env", argv[i]) == 0 ) {
            print_env = 1;
        }
    }

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

    if ( config_env ) { boot_sst_configure_env(verbose, argv, argc); }

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
