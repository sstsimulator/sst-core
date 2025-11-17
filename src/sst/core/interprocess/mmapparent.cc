// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/* This exists only to test mmapparent during sst-core compilation */

#include "sst_config.h"

#include "sst/core/interprocess/mmapparent.h"

#include "sst/core/interprocess/tunneldef.h"
#include "sst/core/sst_mpi.h"

using testtunnel = SST::Core::Interprocess::TunnelDef<int, int>;
template class SST::Core::Interprocess::MMAPParent<testtunnel>;

namespace SST::Core::Interprocess {

int
SST_MPI_Comm_spawn_multiple(int UNUSED_WO_MPI(count), char* UNUSED_WO_MPI(array_of_commands)[],
    char** UNUSED_WO_MPI(array_of_argv)[], const int UNUSED_WO_MPI(array_of_maxprocs)[],
    const char* UNUSED_WO_MPI(array_of_env)[])
{

#ifdef SST_CONFIG_HAVE_MPI
    // Count the maximum number of ranks that may be launched
    int ranks = 0;
    for ( int i = 0; i < count; i++ ) {
        ranks += array_of_maxprocs[i];
    }

    int* array_of_errcodes = (int*)malloc(sizeof(int) * ranks);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int  name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Passing environment variables to child processes is implementation-specific.
    // Documentation for MPI_Info options:
    // https://docs.open-mpi.org/en/main/man-openmpi/man3/MPI_Comm_spawn_multiple.3.html#info-arguments
    MPI_Info* array_of_info = (MPI_Info*)malloc(sizeof(MPI_Info) * count);
    for ( int i = 0; i < count; i++ ) {
        MPI_Info_create(&array_of_info[i]);
        MPI_Info_set(array_of_info[i], "host", processor_name);

        // Do not set the child's environment if array_of_env[i] is empty, which seems to crash.
        if ( strcmp("", array_of_env[i]) ) {
            MPI_Info_set(array_of_info[i], "env", array_of_env[i]);
        }
    }

    MPI_Comm intercomm;

    int result = MPI_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, 0,
        MPI_COMM_SELF, &intercomm, array_of_errcodes);

    // Handle errors here as doing so requires MPI macros and functions
    int errors = 0;
    if ( result != MPI_SUCCESS ) {
        char error_string[MPI_MAX_ERROR_STRING];
        int  length_of_error_string;
        MPI_Error_string(result, error_string, &length_of_error_string);
        fprintf(stderr, "Error in MPI_Comm_spawn_multiple: %s\n", error_string);
        errors++;
    }
    else {
        for ( int i = 0; i < ranks; i++ ) {
            if ( array_of_errcodes[i] != MPI_SUCCESS ) {
                fprintf(stderr, "Error in MPI_Comm_spawn_multiple: process %d failed to start\n", i);
                errors++;
            }
        }
    }

    free(array_of_info);
    free(array_of_errcodes);

    return errors;
#else
    fprintf(stderr, "SST_MPI_Comm_spawn_multiple called but SST-Core was compiled without MPI\n");
    return 1;
#endif
}
} // namespace SST::Core::Interprocess
