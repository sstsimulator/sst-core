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

#include "sst_config.h"

#include "sst/core/iouse.h"

#include "sst/core/sst_mpi.h"

#include <cstdint>
#include <sys/resource.h>

using namespace SST::Core;

uint64_t
SST::Core::maxInputOperations()
{
    struct rusage sim_ruse;
    getrusage(RUSAGE_SELF, &sim_ruse);

    uint64_t local_max_io_in  = sim_ruse.ru_inblock;
    uint64_t global_max_io_in = local_max_io_in;

#ifdef SST_CONFIG_HAVE_MPI
    MPI_Allreduce(&local_max_io_in, &global_max_io_in, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
#endif

    return global_max_io_in;
}

uint64_t
SST::Core::maxOutputOperations()
{
    struct rusage sim_ruse;
    getrusage(RUSAGE_SELF, &sim_ruse);

    uint64_t local_max_io_out  = sim_ruse.ru_oublock;
    uint64_t global_max_io_out = local_max_io_out;

#ifdef SST_CONFIG_HAVE_MPI
    MPI_Allreduce(&local_max_io_out, &global_max_io_out, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
#endif

    return global_max_io_out;
}
