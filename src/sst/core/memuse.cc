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

#include "sst/core/memuse.h"
#include <sst/core/warnmacros.h>
#include <sys/resource.h>

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif


using namespace SST::Core;

uint64_t SST::Core::maxLocalMemSize() {

	struct rusage sim_ruse;
    getrusage(RUSAGE_SELF, &sim_ruse);

    uint64_t local_max_rss = sim_ruse.ru_maxrss;
    uint64_t global_max_rss = local_max_rss;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Allreduce(&local_max_rss, &global_max_rss, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
#endif

#ifdef SST_COMPILE_MACOSX
	return (global_max_rss / 1024);
#else
	return global_max_rss;
#endif

};

uint64_t SST::Core::maxGlobalMemSize() {
	struct rusage sim_ruse;
    getrusage(RUSAGE_SELF, &sim_ruse);

    uint64_t local_max_rss = sim_ruse.ru_maxrss;
    uint64_t global_max_rss = local_max_rss;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Allreduce(&local_max_rss, &global_max_rss, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
#endif

#ifdef SST_COMPILE_MACOSX
	return (global_max_rss / 1024);
#else
	return global_max_rss;
#endif

};

uint64_t SST::Core::maxLocalPageFaults() {
	struct rusage sim_ruse;
    getrusage(RUSAGE_SELF, &sim_ruse);
    
    uint64_t local_pf = sim_ruse.ru_majflt;
    uint64_t global_max_pf = local_pf;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Allreduce(&local_pf, &global_max_pf, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
#endif
	return global_max_pf;
};

uint64_t SST::Core::globalPageFaults() {
	struct rusage sim_ruse;
    getrusage(RUSAGE_SELF, &sim_ruse);

    uint64_t local_pf = sim_ruse.ru_majflt;
    uint64_t global_pf = local_pf;
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Allreduce(&local_pf, &global_pf, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
#endif

	return global_pf;
};
