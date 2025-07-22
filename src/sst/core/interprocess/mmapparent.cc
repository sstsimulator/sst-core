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
#include "sst/core/sst_mpi.h"

#include "sst/core/interprocess/mmapparent.h"

#include "sst/core/interprocess/tunneldef.h"

using testtunnel = SST::Core::Interprocess::TunnelDef<int, int>;
template class SST::Core::Interprocess::MMAPParent<testtunnel>;

namespace SST::Core::Interprocess {
   int SST_MPI_Comm_spawn_multiple (
         char** pin_command,
         const int ranks,
         const int tracerank,
         const char* env) {

      // We have one command for ranks before the tracerank, one
      // for the ranks after it, and one for the trace rank itself
      int count = 1;
      if (tracerank > 0) {
         count++;
      }
      if (tracerank < (ranks-1)) {
         count++;
      }

      // Find where the PIN arguments end and the app starts
      int app_idx = -1;
      for (int i = 0; pin_command[i] != NULL; i++) {
        if (strcmp(pin_command[i], "--") == 0) {
            app_idx = i+1;
            break;
        }
      }

      if (app_idx == -1) {
         return 1;
      }

      // Currently unused.
      // Documentation of options: https://docs.open-mpi.org/en/main/man-openmpi/man3/MPI_Comm_spawn_multiple.3.html#info-arguments
      MPI_Info *array_of_info = (MPI_Info*)malloc(sizeof(MPI_Info) * count);
      for (int i = 0; i < count; i++) {
         MPI_Info_create(&array_of_info[i]);
         //MPI_Info_set(array_of_info[i], "env", env); // TODO Why does this crash?
      }

      // Get the processor name so we can make the traced process
      // run on the same rank the process calling this function.
      char processor_name[MPI_MAX_PROCESSOR_NAME];
      int name_len;
      MPI_Get_processor_name(processor_name, &name_len);

      int    *array_of_maxprocs = (int*)   malloc(sizeof(int) * count);
      char  **array_of_commands = (char**) malloc(sizeof(char*) * count);
      char ***array_of_argv     = (char***)malloc(sizeof(char**) * count);

      if (count == 1) {
         MPI_Info_set(array_of_info[0], "host", processor_name);
         array_of_maxprocs[0] = 1;
         array_of_commands[0] = pin_command[0];
         array_of_argv[0] = pin_command+1;
      } else if (count == 2) {
         if (tracerank == 0) {
            MPI_Info_set(array_of_info[0], "host", processor_name);
            array_of_maxprocs[0] = 1;
            array_of_maxprocs[1] = ranks-1;

            array_of_commands[0] = pin_command[0];
            array_of_commands[1] = pin_command[app_idx];

            array_of_argv[0] = pin_command+1;
            array_of_argv[1] = pin_command+app_idx+1;
         } else {
            MPI_Info_set(array_of_info[1], "host", processor_name);
            array_of_maxprocs[0] = ranks-1;
            array_of_maxprocs[1] = 1;

            array_of_commands[0] = pin_command[app_idx];
            array_of_commands[1] = pin_command[0];

            array_of_argv[0] = pin_command+app_idx+1;
            array_of_argv[1] = pin_command+1;
         }
      } else if (count == 3) {
         MPI_Info_set(array_of_info[1], "host", processor_name);
         array_of_maxprocs[0] = tracerank;
         array_of_maxprocs[1] = 1;
         array_of_maxprocs[2] = ranks - tracerank - 1;

         array_of_commands[0] = pin_command[app_idx];
         array_of_commands[1] = pin_command[0];
         array_of_commands[2] = pin_command[app_idx];

         array_of_argv[0] = pin_command+app_idx+1;
         array_of_argv[1] = pin_command+1;
         array_of_argv[2] = pin_command+app_idx+1;
      }


      /*
      printf("[SST CORE] Count is %d\n", count);
      for(int i = 0; i < count; i++) {
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
      int *array_of_errcodes = (int*)malloc(sizeof(int) * ranks);
      int result = MPI_Comm_spawn_multiple(count,
             array_of_commands,
             array_of_argv,
             array_of_maxprocs,
             array_of_info,
             0,             //root
             MPI_COMM_SELF, //comm
             &intercomm,
             array_of_errcodes);

      int errors = 0;
      if (result != MPI_SUCCESS) {
         char error_string[MPI_MAX_ERROR_STRING];
         int length_of_error_string;
         MPI_Error_string(result, error_string, &length_of_error_string);
         fprintf(stderr, "Error in MPI_Comm_spawn_multiple: %s\n", error_string);
         errors++;
      } else {
         for (int i = 0; i < ranks; i++) {
            if (array_of_errcodes[i] != MPI_SUCCESS) {
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

      if (errors > 0) {
         return 1;
      }

      return 0;
   }
}

