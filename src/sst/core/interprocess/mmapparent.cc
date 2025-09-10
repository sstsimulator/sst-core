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

/** EXPERIMENTAL: Launch an MPI application. This is a wrapper around MPI_Comm_spawn_multiple
 * that hides all MPI information from the calling process. This is intended to be used
 * by elements that need to launch other processes, such as Ariel. Even if the launched
 * application does not use MPI, this function should be used as fork() should not be used
 * by MPI applications.
 *
 * @param count Number of commands
 * @param array_of_commands Commands to run
 * @param array_of_argv Argv for each command
 * @param array_of_maxprocs The maximum number of procs for each command
 * @param array_of_env A newline-delimited list of environment variables for each command
 * @param array_of_hosts Which host each command will run on
 */
int
SST_MPI_Comm_spawn_multiple2(int count, char *array_of_commands[],
                          char **array_of_argv[], const int array_of_maxprocs[],
                          const char *array_of_env[], char *array_of_hosts[])
{

    // Count the maximum number of ranks that may be launched
    int ranks = 0;
    for (int i = 0; i < count; i++) {
        ranks += array_of_maxprocs[i];
    }

    int* array_of_errcodes = (int*)malloc(sizeof(int) * ranks);

    // Passing environment variables to child processes is implementation-specific.
    // Documentation for MPI_Info options:
    // https://docs.open-mpi.org/en/main/man-openmpi/man3/MPI_Comm_spawn_multiple.3.html#info-arguments
    MPI_Info* array_of_info = (MPI_Info*)malloc(sizeof(MPI_Info) * count);
    for ( int i = 0; i < count; i++ ) {
        MPI_Info_create(&array_of_info[i]);
        MPI_Info_set(array_of_info[i], "host", array_of_hosts[i]);

        // Do not set the child's environment if array_of_env[i] is empty, which seems to crash.
        if ( strcmp("", array_of_env[i]) ) {
            MPI_Info_set(array_of_info[i], "env", array_of_env[i]);
        }
    }

    MPI_Comm intercomm;

    int result = MPI_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, 0, MPI_COMM_SELF, &intercomm, array_of_errcodes);

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
}

int
SST_MPI_Comm_spawn_multiple_new(char** pin_command, const int ranks, const int tracerank, const char* env)
{

    // We have one command for ranks before the traced rank, one
    // for the ranks after it, and one for the traced rank itself
    int cmd_count = 1;
    if ( tracerank > 0 ) {
        cmd_count++;
    }
    if ( tracerank < (ranks - 1) ) {
        cmd_count++;
    }

    // Ariel has already formed the entire string to launch PIN + the app.
    // Find where the PIN arguments end and the app starts. The traced rank will
    // launch with pin and the other ranks will launch normally.
    int app_idx = -1;
    for ( int i = 0; pin_command[i] != NULL; i++ ) {
        if ( strcmp(pin_command[i], "--") == 0 ) {
            app_idx = i + 1;
            break;
        }
    }

    if ( app_idx == -1 ) {
        return 1;
    }

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int  name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    int*    array_of_maxprocs = (int*)malloc(sizeof(int) * cmd_count);
    char**  array_of_commands = (char**)malloc(sizeof(char*) * cmd_count);
    char*** array_of_argv     = (char***)malloc(sizeof(char**) * cmd_count);
    const char**  array_of_env      = (const char**)malloc(sizeof(char*) * cmd_count);
    char**  array_of_hosts    = (char**)malloc(sizeof(char*) * cmd_count);

    if ( cmd_count == 1 ) {
        array_of_maxprocs[0] = 1;
        array_of_commands[0] = pin_command[0];
        array_of_argv[0]     = pin_command + 1;
    }
    else if ( cmd_count == 2 ) {
        if ( tracerank == 0 ) {
            array_of_maxprocs[0] = 1;
            array_of_maxprocs[1] = ranks - 1;

            array_of_commands[0] = pin_command[0];
            array_of_commands[1] = pin_command[app_idx];

            array_of_argv[0] = pin_command + 1;
            array_of_argv[1] = pin_command + app_idx + 1;
        }
        else {
            array_of_maxprocs[0] = ranks - 1;
            array_of_maxprocs[1] = 1;

            array_of_commands[0] = pin_command[app_idx];
            array_of_commands[1] = pin_command[0];

            array_of_argv[0] = pin_command + app_idx + 1;
            array_of_argv[1] = pin_command + 1;
        }
    }
    else if ( cmd_count == 3 ) {
        array_of_maxprocs[0] = tracerank;
        array_of_maxprocs[1] = 1;
        array_of_maxprocs[2] = ranks - tracerank - 1;

        array_of_commands[0] = pin_command[app_idx];
        array_of_commands[1] = pin_command[0];
        array_of_commands[2] = pin_command[app_idx];

        array_of_argv[0] = pin_command + app_idx + 1;
        array_of_argv[1] = pin_command + 1;
        array_of_argv[2] = pin_command + app_idx + 1;
    }

    for (int i = 0; i < cmd_count; i++) {
        array_of_env[i] = env;
        array_of_hosts[i] = processor_name;
    }

	int ret = SST_MPI_Comm_spawn_multiple2(cmd_count, array_of_commands,
                          array_of_argv, array_of_maxprocs,
                          array_of_env, array_of_hosts);

    free(array_of_maxprocs);
    free(array_of_commands);
    free(array_of_argv);
    free(array_of_env);
    free(array_of_hosts);

    return ret;
}

/** EXPERIMENTAL: Launch an MPI job with one rank traced by Pin.
 *
 * @param pin_command The full command to launch a process using the pintool
 * @param ranks The number of ranks to launch
 * @param tracerank The rank to be traced by Pin
 * @param env A newline-delimited list of environment variables to be set in the child
 */
int
SST_MPI_Comm_spawn_multiple(char** pin_command, const int ranks, const int tracerank, const char* env)
{

    // We have one command for ranks before the traced rank, one
    // for the ranks after it, and one for the traced rank itself
    int cmd_count = 1;
    if ( tracerank > 0 ) {
        cmd_count++;
    }
    if ( tracerank < (ranks - 1) ) {
        cmd_count++;
    }

    // Ariel has already formed the entire string to launch PIN + the app.
    // Find where the PIN arguments end and the app starts. The traced rank will
    // launch with pin and the other ranks will launch normally.
    int app_idx = -1;
    for ( int i = 0; pin_command[i] != NULL; i++ ) {
        if ( strcmp(pin_command[i], "--") == 0 ) {
            app_idx = i + 1;
            break;
        }
    }

    if ( app_idx == -1 ) {
        return 1;
    }

    // Users may specify environment variables in the Ariel parameters. We pass
    // them to the child using MPI_Info. Also set the processor name so all ranks
    // run on the same node as the parent SST process. We may relax this requirement in
    // the future so that only the traced rank runs alongside SST.
    // Documentation for MPI_Info options:
    // https://docs.open-mpi.org/en/main/man-openmpi/man3/MPI_Comm_spawn_multiple.3.html#info-arguments
    MPI_Info* array_of_info = (MPI_Info*)malloc(sizeof(MPI_Info) * cmd_count);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int  name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    for ( int i = 0; i < cmd_count; i++ ) {
        MPI_Info_create(&array_of_info[i]);
        MPI_Info_set(array_of_info[0], "host", processor_name);

        // Do not set the child environment if env is empty, which seems to crash.
        if ( strcmp("", env) ) {
            MPI_Info_set(array_of_info[i], "env", env);
        }
    }

    int*    array_of_maxprocs = (int*)malloc(sizeof(int) * cmd_count);
    char**  array_of_commands = (char**)malloc(sizeof(char*) * cmd_count);
    char*** array_of_argv     = (char***)malloc(sizeof(char**) * cmd_count);

    if ( cmd_count == 1 ) {
        array_of_maxprocs[0] = 1;
        array_of_commands[0] = pin_command[0];
        array_of_argv[0]     = pin_command + 1;
    }
    else if ( cmd_count == 2 ) {
        if ( tracerank == 0 ) {
            array_of_maxprocs[0] = 1;
            array_of_maxprocs[1] = ranks - 1;

            array_of_commands[0] = pin_command[0];
            array_of_commands[1] = pin_command[app_idx];

            array_of_argv[0] = pin_command + 1;
            array_of_argv[1] = pin_command + app_idx + 1;
        }
        else {
            array_of_maxprocs[0] = ranks - 1;
            array_of_maxprocs[1] = 1;

            array_of_commands[0] = pin_command[app_idx];
            array_of_commands[1] = pin_command[0];

            array_of_argv[0] = pin_command + app_idx + 1;
            array_of_argv[1] = pin_command + 1;
        }
    }
    else if ( cmd_count == 3 ) {
        array_of_maxprocs[0] = tracerank;
        array_of_maxprocs[1] = 1;
        array_of_maxprocs[2] = ranks - tracerank - 1;

        array_of_commands[0] = pin_command[app_idx];
        array_of_commands[1] = pin_command[0];
        array_of_commands[2] = pin_command[app_idx];

        array_of_argv[0] = pin_command + app_idx + 1;
        array_of_argv[1] = pin_command + 1;
        array_of_argv[2] = pin_command + app_idx + 1;
    }


    /*
    printf("[SST CORE] Count is %d\n", cmd_count);
    for(int i = 0; i < cmd_count; i++) {
       printf("[SST CORE] Command %d: \n", i);
       printf("  maxprocs: %d\n", array_of_maxprocs[i]);
       printf("  command: %s\n", array_of_commands[i]);
       printf("  argv:\n");
       for(int j = 0; array_of_argv[i][j]!=NULL; j++) {
          printf("    %s ", array_of_argv[i][j]);
       }
       printf("\n");
    }
    */

    MPI_Comm intercomm;
    int*     array_of_errcodes = (int*)malloc(sizeof(int) * ranks);
    int result = MPI_Comm_spawn_multiple(cmd_count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info,
        0,             // root
        MPI_COMM_SELF, // comm
        &intercomm, array_of_errcodes);

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

    free(array_of_maxprocs);
    free(array_of_commands);
    free(array_of_argv);
    free(array_of_info);
    free(array_of_errcodes);

    if ( errors > 0 ) {
        return 1;
    }

    return 0;
}
} // namespace SST::Core::Interprocess
