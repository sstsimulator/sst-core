
#include "sst_config.h"
#include "sst/core/memuse.h"

using namespace SST::Core;

uint64_t SST::Core::maxLocalMemSize() {

	struct rusage sim_ruse;
        getrusage(RUSAGE_SELF, &sim_ruse);

        uint64_t local_max_rss = sim_ruse.ru_maxrss;
        uint64_t global_max_rss = local_max_rss;
#ifdef HAVE_MPI
        boost::mpi::communicator world;
        all_reduce(world, &local_max_rss, 1, &global_max_rss, boost::mpi::maximum<uint64_t>() );
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
#ifdef HAVE_MPI
	boost::mpi::communicator world;
        all_reduce(world, &local_max_rss, 1, &global_max_rss, std::plus<uint64_t>() );
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
#ifdef HAVE_MPI
	boost::mpi::communicator world;
        all_reduce(world, &local_pf, 1, &global_max_pf, boost::mpi::maximum<uint64_t>() );
#endif

	return global_max_pf;
};

uint64_t SST::Core::globalPageFaults() {
	struct rusage sim_ruse;
        getrusage(RUSAGE_SELF, &sim_ruse);

        uint64_t local_pf = sim_ruse.ru_majflt;
        uint64_t global_pf = local_pf;
#ifdef HAVE_MPI
        boost::mpi::communicator world;
        all_reduce(world, &local_pf, 1, &global_pf, std::plus<uint64_t>() );
#endif

	return global_pf;
};
