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

#include "sst/core/bootshared.h"
#include "sst/core/configShared.h"

int
main(int argc, char* argv[])
{
    bool config_env = true;

    // Create a ConfigShared object to parse just the options needed
    // for the wrapper.  We will suppress output so that it won't
    // report unknown options which are only parsed by the actual
    // sst-info executable.
    SST::ConfigShared cfg(true, true, true, true);

    // Make a copy of the argv array (shallow)
    char* argv_copy[argc + 1];
    for ( int i = 0; i < argc; ++i ) {
        argv_copy[i] = argv[i];
    }
    argv[argc] = nullptr;

    cfg.parseCmdLine(argc, argv_copy, true);

    if ( cfg.no_env_config() ) config_env = false;


    if ( cfg.verbose() && config_env ) { printf("Launching SST with automatic environment processing enabled...\n"); }

    if ( cfg.print_env() ) {
        int next_index = 0;

        while ( nullptr != environ[next_index] ) {
            const char* next_env = environ[next_index];
            printf("%s\n", next_env);

            next_index++;
        }
    }

    if ( config_env ) { boot_sst_configure_env(cfg.getLibPath()); }

    boot_sst_executable("sstinfo.x", cfg.verbose(), argv, argc);
}
