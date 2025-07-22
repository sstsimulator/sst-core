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
   int SST_MPI_Comm_spawn_multiple (int count,
         char *array_of_commands[],
         char **array_of_argv[],
         const int array_of_maxprocs[]) {

      MPI_Info *array_of_info = (MPI_Info*)malloc(sizeof(MPI_Info) * count);
      for (int i = 0; i < count; i++) {
         //TODO Check error
         MPI_Info_create(&array_of_info[i]);
      }

      int root = 0;
      MPI_Comm comm = MPI_COMM_SELF;
      MPI_Comm intercomm; // TODO: Store somewhere

      // Each command may spawn multiple ranks
      // Each rank returns a separate error code
      // We need to allocate space for each
      int sum_max_procs = 0;
      for (int i = 0; i < count; i++) {
         sum_max_procs += array_of_maxprocs[i];
      }
      int *array_of_errcodes = (int*)malloc(sizeof(int) * sum_max_procs);

      int result = MPI_Comm_spawn_multiple(count,
             array_of_commands,
             array_of_argv,
             array_of_maxprocs,
             array_of_info,
             root,
             comm,
             &intercomm,
             array_of_errcodes);

      if (result != MPI_SUCCESS) {
         char error_string[MPI_MAX_ERROR_STRING];
         int length_of_error_string;
         MPI_Error_string(result, error_string, &length_of_error_string);

         // Print a meaningful error message
         fprintf(stderr, "Error in MPI_Comm_spawn_multiple: %s\n", error_string);

         // Optionally, you could handle the error further (e.g., abort the program)
         //MPI_Abort(comm, result);
         free(array_of_info);
         free(array_of_errcodes);
         return 1;
      }

      int errors = 0;
      for (int i = 0; i < sum_max_procs; i++) {
         if (array_of_errcodes[i] != MPI_SUCCESS) {
             fprintf(stderr, "Error in MPI_Comm_spawn_multiple: process %d failed to start\n", i);
             errors++;
         }
      }

      free(array_of_info);
      free(array_of_errcodes);

      if (errors > 0) {
         return 1;
      }
      return 0;
   }
}

