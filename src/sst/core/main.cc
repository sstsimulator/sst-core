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

#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
// The Python header already defines this and should override one from the
// command line.
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#include <Python.h>
REENABLE_WARNING

#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include "sst/core/activity.h"
#include "sst/core/config.h"
#include "sst/core/configGraph.h"
#include "sst/core/cputimer.h"
#include "sst/core/exit.h"
#include "sst/core/factory.h"
#include "sst/core/iouse.h"
#include "sst/core/link.h"
#include "sst/core/mempool.h"
#include "sst/core/mempoolAccessor.h"
#include "sst/core/memuse.h"
#include "sst/core/model/sstmodel.h"
#include "sst/core/objectComms.h"
#include "sst/core/rankInfo.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/statapi/statengine.h"
#include "sst/core/stringize.h"
#include "sst/core/threadsafe.h"
#include "sst/core/timeLord.h"
#include "sst/core/timeVortex.h"

#include <cinttypes>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sys/resource.h>
#include <time.h>

// Configuration Graph Generation Options
#include "sst/core/cfgoutput/dotConfigOutput.h"
#include "sst/core/cfgoutput/jsonConfigOutput.h"
#include "sst/core/cfgoutput/pythonConfigOutput.h"
#include "sst/core/configGraphOutput.h"
#include "sst/core/eli/elementinfo.h"

using namespace SST::Core;
using namespace SST::Partition;
using namespace std;
using namespace SST;

static SST::Output g_output;


// Functions to force initialization stages of simulation to execute
// one rank at a time.  Put force_rank_sequential_start() before the
// serialized section and force_rank_sequential_stop() after.  These
// calls must be used in matching pairs.  It should also be followed
// by a barrier if there are multiple threads running at the time of
// the call.
static void
force_rank_sequential_start(bool enable, const RankInfo& myRank, const RankInfo& world_size)
{
    if ( !enable || world_size.rank == 1 || myRank.thread != 0 ) return;

#ifdef SST_CONFIG_HAVE_MPI
    // Start off all ranks with a barrier so none enter the serialized
    // region until they are all there
    MPI_Barrier(MPI_COMM_WORLD);

    // Rank 0 will proceed immediately.  All others will wait
    if ( myRank.rank == 0 ) return;

    // Ranks will wait for notice from previous rank before proceeding
    int32_t buf = 0;
    MPI_Recv(&buf, 1, MPI_INT32_T, myRank.rank - 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
#endif
}


// Functions to force initialization stages of simulation to execute
// one rank at a time.  Put force_rank_sequential_start() before the
// serialized section and force_rank_sequential_stop() after.  These
// calls must be used in matching pairs.
static void
force_rank_sequential_stop(bool enable, const RankInfo& myRank, const RankInfo& world_size)
{
    if ( !enable || world_size.rank == 1 || myRank.thread != 0 ) return;

#ifdef SST_CONFIG_HAVE_MPI
    // After I'm through the serialized region, notify the next
    // sender, then barrier.  The last rank does not need to do a
    // send.
    if ( myRank.rank != world_size.rank - 1 ) {
        uint32_t buf = 0;
        MPI_Send(&buf, 1, MPI_INT32_T, myRank.rank + 1, 0, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

static void
SimulationSigHandler(int sig)
{
    Simulation_impl::setSignal(sig);
    if ( sig == SIGINT || sig == SIGTERM ) {
        signal(sig, SIG_DFL); // Restore default handler
    }
}

static void
setupSignals(uint32_t threadRank)
{
    if ( 0 == threadRank ) {
        if ( SIG_ERR == signal(SIGUSR1, SimulationSigHandler) ) {
            g_output.fatal(CALL_INFO, 1, "Installation of SIGUSR1 signal handler failed.\n");
        }
        if ( SIG_ERR == signal(SIGUSR2, SimulationSigHandler) ) {
            g_output.fatal(CALL_INFO, 1, "Installation of SIGUSR2 signal handler failed\n");
        }
        if ( SIG_ERR == signal(SIGINT, SimulationSigHandler) ) {
            g_output.fatal(CALL_INFO, 1, "Installation of SIGINT signal handler failed\n");
        }
        if ( SIG_ERR == signal(SIGALRM, SimulationSigHandler) ) {
            g_output.fatal(CALL_INFO, 1, "Installation of SIGALRM signal handler failed\n");
        }
        if ( SIG_ERR == signal(SIGTERM, SimulationSigHandler) ) {
            g_output.fatal(CALL_INFO, 1, "Installation of SIGTERM signal handler failed\n");
        }

        g_output.verbose(CALL_INFO, 1, 0, "Signal handler registration is completed\n");
    }
    else {
        /* Other threads don't want to receive the signal */
        sigset_t maskset;
        sigfillset(&maskset);
        pthread_sigmask(SIG_BLOCK, &maskset, nullptr);
    }
}

static void
dump_partition(Config& cfg, ConfigGraph* graph, const RankInfo& size)
{

    ///////////////////////////////////////////////////////////////////////
    // If the user asks us to dump the partitioned graph.
    if ( cfg.component_partition_file() != "" ) {
        if ( cfg.verbose() ) {
            g_output.verbose(
                CALL_INFO, 1, 0, "# Dumping partitioned component graph to %s\n",
                cfg.component_partition_file().c_str());
        }

        ofstream              graph_file(cfg.component_partition_file().c_str());
        ConfigComponentMap_t& component_map = graph->getComponentMap();

        for ( uint32_t i = 0; i < size.rank; i++ ) {
            for ( uint32_t t = 0; t < size.thread; t++ ) {
                graph_file << "Rank: " << i << "." << t << " Component List:" << std::endl;

                RankInfo r(i, t);
                for ( ConfigComponentMap_t::const_iterator j = component_map.begin(); j != component_map.end(); ++j ) {
                    auto c = *j;
                    if ( c->rank == r ) {
                        graph_file << "   " << c->name << " (ID=" << c->id << ")" << std::endl;
                        graph_file << "      -> type      " << c->type << std::endl;
                        graph_file << "      -> weight    " << c->weight << std::endl;
                        graph_file << "      -> linkcount " << c->links.size() << std::endl;
                        graph_file << "      -> rank      " << c->rank.rank << std::endl;
                        graph_file << "      -> thread    " << c->rank.thread << std::endl;
                    }
                }
            }
        }

        graph_file.close();

        if ( cfg.verbose() ) { g_output.verbose(CALL_INFO, 2, 0, "# Dump of partition graph is complete.\n"); }
    }
}

static void
do_graph_wireup(ConfigGraph* graph, SST::Simulation_impl* sim, const RankInfo& myRank, SimTime_t min_part)
{

    if ( !graph->containsComponentInRank(myRank) ) {
        g_output.output("WARNING: No components are assigned to rank: %u.%u\n", myRank.rank, myRank.thread);
    }

    sim->performWireUp(*graph, myRank, min_part);
}

// Functions to do shared (static) initialization and notificaion for
// stats engines.  Right now, the StatGroups are per MPI rank and
// everything else in StatEngine is per partition.
static void
do_statengine_static_initialization(ConfigGraph* graph, const RankInfo& myRank)
{
    if ( myRank.thread != 0 ) return;
    StatisticProcessingEngine::static_setup(graph);
}

static void
do_statoutput_start_simulation(const RankInfo& myRank)
{
    if ( myRank.thread != 0 ) return;
    StatisticProcessingEngine::stat_outputs_simulation_start();
}

static void
do_statoutput_end_simulation(const RankInfo& myRank)
{
    if ( myRank.thread != 0 ) return;
    StatisticProcessingEngine::stat_outputs_simulation_end();
}

// Function to initialize the StatEngines in each partition (Simulation_impl object)
static void
do_statengine_initialization(ConfigGraph* graph, SST::Simulation_impl* sim, const RankInfo& UNUSED(myRank))
{
    sim->initializeStatisticEngine(*graph);
}

static void
do_link_preparation(ConfigGraph* graph, SST::Simulation_impl* sim, const RankInfo& myRank, SimTime_t min_part)
{
    sim->prepareLinks(*graph, myRank, min_part);
}

// Returns the extension, or an empty string if there was no extension
static std::string
addRankToFileName(std::string& file_name, int rank)
{
    auto        index = file_name.find_last_of(".");
    std::string base;
    std::string ext;
    // If there is an extension, add it before the extension
    if ( index != std::string::npos ) {
        base = file_name.substr(0, index);
        ext  = file_name.substr(index);
    }
    else {
        base = file_name;
    }
    file_name = base + std::to_string(rank) + ext;
    return ext;
}

static void
doSerialOnlyGraphOutput(SST::Config* cfg, ConfigGraph* graph)
{
    // See if user asked us to dump the config graph in dot graph format
    if ( cfg->output_dot() != "" ) {
        DotConfigGraphOutput out(cfg->output_dot().c_str());
        out.generate(cfg, graph);
    }
}

// This should only be called once in main().  Either before or after
// graph broadcast depending on if parallel_load is turned on or not.
// If on, call it after graph broadcast, if off, call it before.
static void
doParallelCapableGraphOutput(SST::Config* cfg, ConfigGraph* graph, const RankInfo& myRank, const RankInfo& world_size)
{
    // User asked us to dump the config graph to a file in Python
    if ( cfg->output_config_graph() != "" ) {
        // See if we are doing parallel output
        std::string file_name(cfg->output_config_graph());
        if ( cfg->parallel_output() && world_size.rank != 1 ) {
            // Append rank number to base filename
            std::string ext = addRankToFileName(file_name, myRank.rank);
            if ( ext != ".py" ) {
                g_output.fatal(CALL_INFO, 1, "--output-config requires a filename with a .py extension\n");
            }
        }
        PythonConfigGraphOutput out(file_name.c_str());
        out.generate(cfg, graph);
    }

    // User asked us to dump the config graph in JSON format
    if ( cfg->output_json() != "" ) {
        std::string file_name(cfg->output_json());
        if ( cfg->parallel_output() ) {
            // Append rank number to base filename
            std::string ext = addRankToFileName(file_name, myRank.rank);
            if ( ext != ".json" ) {
                g_output.fatal(CALL_INFO, 1, "--output-json requires a filename with a .json extension\n");
            }
        }
        JSONConfigGraphOutput out(file_name.c_str());
        out.generate(cfg, graph);
    }
}

static double
start_graph_creation(
    ConfigGraph*& graph, Config& cfg, Factory* factory, const RankInfo& world_size, const RankInfo& myRank)
{
    // Get a list of all the available SSTModelDescriptions
    std::vector<std::string> models = ELI::InfoDatabase::getRegisteredElementNames<SSTModelDescription>();

    // Create a map of extensions to the model that supports them
    std::map<std::string, std::string> extension_map;
    for ( auto x : models ) {
        // auto extensions = factory->getSimpleInfo<SSTModelDescription, 1, std::vector<std::string>>(x);
        auto extensions = SSTModelDescription::getElementSupportedExtensions(x);
        for ( auto y : extensions ) {
            extension_map[y] = x;
        }
    }


    // Create the model generator
    SSTModelDescription* modelGen = nullptr;

    force_rank_sequential_start(cfg.rank_seq_startup(), myRank, world_size);

    if ( cfg.configFile() != "NONE" ) {
        // Get the file extension by finding the last .
        std::string extension = cfg.configFile().substr(cfg.configFile().find_last_of("."));

        std::string model_name;
        try {
            model_name = extension_map.at(extension);
        }
        catch ( exception& e ) {
            std::cerr << "Unsupported SDL file type: \"" << extension << "\"" << std::endl;
            return -1;
        }

        // If doing parallel load, make sure this model is parallel capable
        if ( cfg.parallel_load() && !SSTModelDescription::isElementParallelCapable(model_name) ) {
            std::cerr << "Model type for extension: \"" << extension << "\" does not support parallel loading."
                      << std::endl;
            return -1;
        }

        if ( myRank.rank == 0 || cfg.parallel_load() ) {
            modelGen = factory->Create<SSTModelDescription>(
                model_name, cfg.configFile(), cfg.verbose(), &cfg, sst_get_cpu_time());
        }
    }


    double start_graph_gen = sst_get_cpu_time();

    // Only rank 0 will populate the graph, unless we are using
    // parallel load.  In this case, all ranks will load the graph
    if ( myRank.rank == 0 || cfg.parallel_load() ) {
        try {
            graph = modelGen->createConfigGraph();
        }
        catch ( std::exception& e ) {
            g_output.fatal(CALL_INFO, -1, "Error encountered during config-graph generation: %s\n", e.what());
        }
    }
    else {
        graph = new ConfigGraph();
    }


    force_rank_sequential_stop(cfg.rank_seq_startup(), myRank, world_size);

#ifdef SST_CONFIG_HAVE_MPI
    // Config is done - broadcast it, unless we are parallel loading
    if ( world_size.rank > 1 && !cfg.parallel_load() ) {
        try {
            Comms::broadcast(cfg, 0);
        }
        catch ( std::exception& e ) {
            g_output.fatal(CALL_INFO, -1, "Error encountered broadcasting configuration object: %s\n", e.what());
        }
    }
#endif

    // Delete the model generator
    if ( modelGen ) {
        delete modelGen;
        modelGen = nullptr;
    }

    return start_graph_gen;
}

static double
start_partitioning(
    Config& cfg, const RankInfo& world_size, const RankInfo& myRank, Factory* factory, ConfigGraph* graph)
{
    ////// Start Partitioning //////
    double start_part = sst_get_cpu_time();

    if ( !cfg.parallel_load() ) {
        // Normal partitioning

        // // If this is a serial job, just use the single partitioner,
        // // but the same code path
        // if ( world_size.rank == 1 && world_size.thread == 1 ) cfg.partitioner_ = "sst.single";

        // Get the partitioner.  Built in partitioners are in the "sst" library.
        SSTPartitioner* partitioner = factory->CreatePartitioner(cfg.partitioner(), world_size, myRank, cfg.verbose());
        try {
            if ( partitioner->requiresConfigGraph() ) { partitioner->performPartition(graph); }
            else {
                PartitionGraph* pgraph;
                if ( myRank.rank == 0 ) { pgraph = graph->getCollapsedPartitionGraph(); }
                else {
                    pgraph = new PartitionGraph();
                }

                if ( myRank.rank == 0 || partitioner->spawnOnAllRanks() ) {
                    partitioner->performPartition(pgraph);

                    if ( myRank.rank == 0 ) graph->annotateRanks(pgraph);
                }

                delete pgraph;
            }
        }
        catch ( std::exception& e ) {
            g_output.fatal(CALL_INFO, -1, "Error encountered during graph partitioning phase: %s\n", e.what());
        }

        delete partitioner;
    }

    // Check the partitioning to make sure it is sane
    if ( myRank.rank == 0 || cfg.parallel_load() ) {
        if ( !graph->checkRanks(world_size) ) {
            g_output.fatal(CALL_INFO, 1, "ERROR: Bad partitioning; partition included unknown ranks.\n");
        }
    }
    return sst_get_cpu_time() - start_part;
}

struct SimThreadInfo_t
{
    RankInfo     myRank;
    RankInfo     world_size;
    Config*      config;
    ConfigGraph* graph;
    SimTime_t    min_part;

    // Time / stats information
    double      build_time;
    double      run_time;
    UnitAlgebra simulated_time;
    uint64_t    max_tv_depth;
    uint64_t    current_tv_depth;
    uint64_t    sync_data_size;
};

static void
start_simulation(uint32_t tid, SimThreadInfo_t& info, Core::ThreadSafe::Barrier& barrier)
{
    // Setup Mempools
    Core::MemPoolAccessor::initializeLocalData(tid);
    info.myRank.thread = tid;

    if ( tid ) {
        /* already did Thread Rank 0 in main() */
        setupSignals(tid);
    }

    ////// Create Simulation Objects //////
    SST::Simulation_impl* sim       = Simulation_impl::createSimulation(info.config, info.myRank, info.world_size);
    double                start_run = 0.0;

    bool restart = info.config->load_from_checkpoint();

    if ( !restart ) {
        double start_build = sst_get_cpu_time();


        barrier.wait();

        sim->processGraphInfo(*info.graph, info.myRank, info.min_part);

        barrier.wait();

        force_rank_sequential_start(info.config->rank_seq_startup(), info.myRank, info.world_size);

        barrier.wait();

        // Perform the wireup.
        if ( tid == 0 ) { do_statengine_static_initialization(info.graph, info.myRank); }
        barrier.wait();

        do_statengine_initialization(info.graph, sim, info.myRank);
        barrier.wait();

        // Prepare the links, which creates the ComponentInfo objects and
        // Link and puts the links in the LinkMap for each ComponentInfo.
#ifdef SST_COMPILE_MACOSX
        // Some versions of clang on mac have an issue with deleting links
        // that were created interleaved between threads, so force
        // serialization of threads during link creation.  This has been
        // confirmed on both Intel and ARM for Xcode 14 and 15.  We should
        // revisit this in the future.  This is easy to see when running
        // sst-benchmark with 1024 components and multiple threads.  At
        // time of adding this code, the difference in delete times was
        // 3-5 minutes versuses less than a second.
        for ( uint32_t i = 0; i < info.world_size.thread; ++i ) {
            if ( i == info.myRank.thread ) { do_link_preparation(info.graph, sim, info.myRank, info.min_part); }
            barrier.wait();
        }
#else
        do_link_preparation(info.graph, sim, info.myRank, info.min_part);
#endif
        barrier.wait();

        // Create all the simulation components
        do_graph_wireup(info.graph, sim, info.myRank, info.min_part);
        barrier.wait();

        if ( tid == 0 ) { delete info.graph; }

        force_rank_sequential_stop(info.config->rank_seq_startup(), info.myRank, info.world_size);

        barrier.wait();

        if ( info.myRank.thread == 0 ) { sim->exchangeLinkInfo(); }

        barrier.wait();

        start_run       = sst_get_cpu_time();
        info.build_time = start_run - start_build;

#ifdef SST_CONFIG_HAVE_MPI
        if ( tid == 0 && info.world_size.rank > 1 ) { MPI_Barrier(MPI_COMM_WORLD); }
#endif

        barrier.wait();

        if ( info.config->runMode() == SimulationRunMode::RUN || info.config->runMode() == SimulationRunMode::BOTH ) {
            if ( info.config->verbose() && 0 == tid ) {
                g_output.verbose(CALL_INFO, 1, 0, "# Starting main event loop\n");

                time_t     the_time = time(nullptr);
                struct tm* now      = localtime(&the_time);

                g_output.verbose(
                    CALL_INFO, 1, 0, "# Start time: %04u/%02u/%02u at: %02u:%02u:%02u\n", (now->tm_year + 1900),
                    (now->tm_mon + 1), now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

                if ( info.config->exit_after() > 0 ) {
                    time_t     stop_time = the_time + info.config->exit_after();
                    struct tm* end       = localtime(&stop_time);
                    g_output.verbose(
                        CALL_INFO, 1, 0, "# Will end by: %04u/%02u/%02u at: %02u:%02u:%02u\n", (end->tm_year + 1900),
                        (end->tm_mon + 1), end->tm_mday, end->tm_hour, end->tm_min, end->tm_sec);

                    /* Set the alarm */
                    alarm(info.config->exit_after());
                }
            }
            // g_output.output("info.config.stopAtCycle = %s\n",info.config->stopAtCycle.c_str());
            sim->setStopAtCycle(info.config);

            if ( tid == 0 && info.world_size.rank > 1 ) {
                // If we are a MPI_parallel job, need to makes sure that all used
                // libraries are loaded on all ranks.
#ifdef SST_CONFIG_HAVE_MPI
                set<string> lib_names;
                set<string> other_lib_names;
                Factory::getFactory()->getLoadedLibraryNames(lib_names);

                // Send my lib_names to the next lowest rank
                if ( info.myRank.rank == (info.world_size.rank - 1) ) {
                    Comms::send(info.myRank.rank - 1, 0, lib_names);
                    lib_names.clear();
                }
                else {
                    Comms::recv(info.myRank.rank + 1, 0, other_lib_names);
                    for ( auto iter = other_lib_names.begin(); iter != other_lib_names.end(); ++iter ) {
                        lib_names.insert(*iter);
                    }
                    if ( info.myRank.rank != 0 ) {
                        Comms::send(info.myRank.rank - 1, 0, lib_names);
                        lib_names.clear();
                    }
                }

                Comms::broadcast(lib_names, 0);
                Factory::getFactory()->loadUnloadedLibraries(lib_names);
#endif
            }
            barrier.wait();

            sim->initialize();
            barrier.wait();

            /* Run Set */
            sim->setup();
            barrier.wait();

            /* Finalize all the stat outputs */
            do_statoutput_start_simulation(info.myRank);
            barrier.wait();

            sim->prepare_for_run();
        }
    }
    else {
        // Finish parsing checkpoint for restart
        sim->restart(info.config);
    }
    /* Run Simulation */
    if ( info.config->runMode() == SimulationRunMode::RUN || info.config->runMode() == SimulationRunMode::BOTH ) {
        sim->run();
        barrier.wait();

        /* Adjust clocks at simulation end to
         * reflect actual simulation end if that
         * differs from detected simulation end
         */
        sim->adjustTimeAtSimEnd();
        barrier.wait();

        sim->complete();
        barrier.wait();

        sim->finish();
        barrier.wait();

        /* Tell stat outputs simulation is done */
        do_statoutput_end_simulation(info.myRank);
        barrier.wait();
    }

    info.simulated_time = sim->getEndSimTime();
    // g_output.output(CALL_INFO,"Simulation time = %s\n",info.simulated_time.toStringBestSI().c_str());

    double end_time = sst_get_cpu_time();
    info.run_time   = end_time - start_run;

    info.max_tv_depth     = sim->getTimeVortexMaxDepth();
    info.current_tv_depth = sim->getTimeVortexCurrentDepth();

    // Print the profiling info.  For threads, we will serialize
    // writing and for ranks we will use different files, unless we
    // are writing to console, in which case we will serialize the
    // output as well.
    FILE*       fp   = nullptr;
    std::string file = info.config->profilingOutput();
    if ( file == "stdout" ) {
        // Output to the console, so we will force both rank and
        // thread output to be sequential
        force_rank_sequential_start(info.world_size.rank > 1, info.myRank, info.world_size);

        for ( uint32_t i = 0; i < info.world_size.thread; ++i ) {
            if ( i == info.myRank.thread ) { sim->printProfilingInfo(stdout); }
            barrier.wait();
        }

        force_rank_sequential_stop(info.world_size.rank > 1, info.myRank, info.world_size);
        barrier.wait();
    }
    else {
        // Output to file
        if ( info.world_size.rank > 1 ) { addRankToFileName(file, info.myRank.rank); }

        // First thread will open a new file
        std::string mode;
        // Thread 0 will open a new file, all others will append
        if ( info.myRank.thread == 0 )
            mode = "w";
        else
            mode = "a";

        for ( uint32_t i = 0; i < info.world_size.thread; ++i ) {
            if ( i == info.myRank.thread ) {
                fp = fopen(file.c_str(), mode.c_str());
                sim->printProfilingInfo(fp);
                fclose(fp);
            }
            barrier.wait();
        }
    }

    // Put in info about sync memory usage
    info.sync_data_size = sim->getSyncQueueDataSize();

    delete sim;
}

int
main(int argc, char* argv[])
{
#ifdef SST_CONFIG_HAVE_MPI
    // Initialize MPI
    MPI_Init(&argc, &argv);

    int myrank = 0;
    int mysize = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &mysize);

    RankInfo world_size(mysize, 1);
    RankInfo myRank(myrank, 0);
#else
    int myrank = 0;
    RankInfo world_size(1, 1);
    RankInfo myRank(0, 0);
#endif
    Config cfg(world_size.rank, myrank == 0);

    // All ranks parse the command line
    auto ret_value = cfg.parseCmdLine(argc, argv);
    if ( ret_value == -1 ) {
        // Error in command line arguments
        return -1;
    }
    else if ( ret_value == 1 ) {
        // Just asked for info, clean exit
        return 0;
    }

    // Check to see if we are doing a restart from a checkpoint
    bool restart = cfg.load_from_checkpoint();

    // If restarting, update config from checkpoint
    uint32_t cpt_num_threads, cpt_num_ranks;
    if ( restart ) {
        size_t size;
        char*  buffer;

        SST::Core::Serialization::serializer ser;
        ser.enable_pointer_tracking();
        std::ifstream fs(cfg.configFile(), std::ios::binary);
        if ( !fs.is_open() ) {
            if ( fs.bad() ) {
                fprintf(stderr, "Unable to open checkpoint file [%s]: badbit set\n", cfg.configFile().c_str());
                return -1;
            }
            if ( fs.fail() ) {
                fprintf(stderr, "Unable to open checkpoint file [%s]: %s\n", cfg.configFile().c_str(), strerror(errno));
                return -1;
            }
            fprintf(stderr, "Unable to open checkpoint file [%s]: unknown error\n", cfg.configFile().c_str());
            return -1;
        }

        fs.read(reinterpret_cast<char*>(&size), sizeof(size));
        buffer = new char[size];
        fs.read(buffer, size);

        std::string cpt_lib_path, cpt_timebase, cpt_output_directory;
        std::string cpt_output_core_prefix, cpt_debug_file, cpt_prefix;
        int         cpt_output_verbose = 0;

        ser.start_unpacking(buffer, size);
        ser& cpt_num_ranks;
        ser& cpt_num_threads;
        ser& cpt_lib_path;
        ser& cpt_timebase;
        ser& cpt_output_directory;
        ser& cpt_output_core_prefix;
        ser& cpt_output_verbose;
        ser& cpt_debug_file;
        ser& cpt_prefix;

        fs.close();
        delete[] buffer;

        // Error check that ranks & threads match after output is created
        cfg.libpath_  = cpt_lib_path;
        cfg.timeBase_ = cpt_timebase;
        if ( !cfg.wasOptionSetOnCmdLine("output-directory") ) cfg.output_directory_ = cpt_output_directory;
        if ( !cfg.wasOptionSetOnCmdLine("output-prefix-core") ) cfg.output_core_prefix_ = cpt_output_core_prefix;
        if ( !cfg.wasOptionSetOnCmdLine("verbose") ) cfg.verbose_ = cpt_output_verbose;
        if ( !cfg.wasOptionSetOnCmdLine("debug-file") ) cfg.debugFile_ = cpt_debug_file;
        if ( !cfg.wasOptionSetOnCmdLine("checkpoint-prefix") ) cfg.checkpoint_prefix_ = cpt_prefix;

        ////// Initialize global data //////

        // These are initialized after graph creation in the non-restart path
        world_size.thread = cfg.num_threads();
        Output::setFileName(cfg.debugFile() != "/dev/null" ? cfg.debugFile() : "sst_output");
        Output::setWorldSize(world_size.rank, world_size.thread, myrank);
        g_output = Output::setDefaultObject(cfg.output_core_prefix(), cfg.verbose(), 0, Output::STDOUT);
        Simulation_impl::getTimeLord()->init(cfg.timeBase());
    }

    // Check to see if the config file exists
    cfg.checkConfigFile();

    // If we are doing a parallel load with a file per rank, add the
    // rank number to the file name before the extension
    if ( cfg.parallel_load() && cfg.parallel_load_mode_multi() && world_size.rank != 1 ) {
        addRankToFileName(cfg.configFile_, myRank.rank);
    }

    // Create the factory.  This may be needed to load an external model definition
    Factory* factory = new Factory(cfg.getLibPath());

    if ( restart && (cfg.num_ranks() != cpt_num_ranks || cfg.num_threads() != cpt_num_threads) ) {
        g_output.fatal(
            CALL_INFO, 1,
            "Rank or thread counts do not match checkpoint. "
            "Checkpoint requires %" PRIu32 " ranks and %" PRIu32 " threads\n",
            cpt_num_ranks, cpt_num_threads);
    }

    ////// Start ConfigGraph Creation //////

    double       start    = sst_get_cpu_time();
    ConfigGraph* graph    = nullptr;
    SimTime_t    min_part = 0xffffffffffffffffl;

    // If we aren't restarting, need to create the graph
    if ( !restart ) {
        // Get the memory before we create the graph
        const uint64_t pre_graph_create_rss = maxGlobalMemSize();

        double start_graph_gen = start_graph_creation(graph, cfg, factory, world_size, myRank);

        ////// Initialize global data //////
        // Config is updated from SDL, initialize globals

        // Set the number of threads
        world_size.thread = cfg.num_threads();

        // Create global output object
        Output::setFileName(cfg.debugFile() != "/dev/null" ? cfg.debugFile() : "sst_output");
        Output::setWorldSize(world_size.rank, world_size.thread, myrank);
        g_output = Output::setDefaultObject(cfg.output_core_prefix(), cfg.verbose(), 0, Output::STDOUT);

        g_output.verbose(
            CALL_INFO, 1, 0, "#main() My rank is (%u.%u), on %u/%u nodes/threads\n", myRank.rank, myRank.thread,
            world_size.rank, world_size.thread);

        // TimeLord must be initialized prior to postCreationCleanup() call
        Simulation_impl::getTimeLord()->init(cfg.timeBase());

        // Cleanup after graph creation
        if ( myRank.rank == 0 || cfg.parallel_load() ) {

            graph->postCreationCleanup();

            // Check config graph to see if there are structural errors.
            if ( graph->checkForStructuralErrors() ) {
                g_output.fatal(CALL_INFO, 1, "Structure errors found in the ConfigGraph.\n");
            }
        }
        double graph_gen_time = sst_get_cpu_time() - start_graph_gen;

        // If verbose level is high enough, compute the total number
        // components in the simulation.  NOTE: if parallel-load is
        // enabled, then the parittioning won't actually happen and all
        // ranks already have their parts of the graph.
        uint64_t comp_count = 0;
        if ( cfg.verbose() >= 1 ) {
            if ( !cfg.parallel_load() && myRank.rank == 0 ) { comp_count = graph->getNumComponents(); }
#ifdef SST_CONFIG_HAVE_MPI
            else if ( cfg.parallel_load() ) {
                uint64_t my_count = graph->getNumComponentsInMPIRank(myRank.rank);
                MPI_Allreduce(&my_count, &comp_count, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
            }
#endif
        }

        if ( myRank.rank == 0 ) {
            g_output.verbose(CALL_INFO, 1, 0, "# ------------------------------------------------------------\n");
            g_output.verbose(CALL_INFO, 1, 0, "# Graph construction took %f seconds.\n", graph_gen_time);
            g_output.verbose(CALL_INFO, 1, 0, "# Graph contains %" PRIu64 " components\n", comp_count);
        }

        ////// End ConfigGraph Creation //////

#ifdef SST_CONFIG_HAVE_MPI
        // If we did a parallel load, check to make sure that all the
        // ranks have the same thread count set (the python can change the
        // thread count if not specified on the command line
        if ( cfg.parallel_load() ) {
            uint32_t max_thread_count = 0;
            uint32_t my_thread_count  = cfg.num_threads();
            MPI_Allreduce(&my_thread_count, &max_thread_count, 1, MPI_UINT32_T, MPI_MAX, MPI_COMM_WORLD);
            if ( my_thread_count != max_thread_count ) {
                g_output.fatal(
                    CALL_INFO, 1, "Thread counts do no match across ranks for configuration using parallel loading\n");
            }
        }
#endif

        // If this is a serial job, just use the single partitioner,
        // but the same code path
        if ( world_size.rank == 1 && world_size.thread == 1 ) cfg.partitioner_ = "sst.single";

        // Run the partitioner
        double partitioning_time = start_partitioning(cfg, world_size, myRank, factory, graph);

        const uint64_t post_graph_create_rss = maxGlobalMemSize();

        if ( myRank.rank == 0 ) {
            if ( !cfg.parallel_load() )
                g_output.verbose(CALL_INFO, 1, 0, "# Graph partitioning took %lg seconds.\n", partitioning_time);
            g_output.verbose(
                CALL_INFO, 1, 0, "# Graph construction and partition raised RSS by %" PRIu64 " KB\n",
                (post_graph_create_rss - pre_graph_create_rss));
            g_output.verbose(CALL_INFO, 1, 0, "# ------------------------------------------------------------\n");

            // Output the partition information if user requests it
            dump_partition(cfg, graph, world_size);
        }
        ////// End Partitioning //////

        ////// Calculate Minimum Partitioning //////
        SimTime_t local_min_part = 0xffffffffffffffffl;
        if ( world_size.rank > 1 ) {
            // Check the graph for the minimum latency crossing a partition boundary
            if ( myRank.rank == 0 || cfg.parallel_load() ) {
                ConfigComponentMap_t& comps = graph->getComponentMap();
                ConfigLinkMap_t&      links = graph->getLinkMap();
                // Find the minimum latency across a partition
                for ( ConfigLinkMap_t::iterator iter = links.begin(); iter != links.end(); ++iter ) {
                    ConfigLink* clink = *iter;
                    RankInfo    rank[2];
                    rank[0] = comps[COMPONENT_ID_MASK(clink->component[0])]->rank;
                    rank[1] = comps[COMPONENT_ID_MASK(clink->component[1])]->rank;
                    if ( rank[0].rank == rank[1].rank ) continue;
                    if ( clink->getMinLatency() < local_min_part ) { local_min_part = clink->getMinLatency(); }
                }
            }
#ifdef SST_CONFIG_HAVE_MPI

            // Fix for case that probably doesn't matter in practice, but
            // does come up during some specific testing.  If there are no
            // links that cross the boundary and we're a multi-rank job,
            // we need to put in a sync interval to look for the exit
            // conditions being met.
            // if ( min_part == MAX_SIMTIME_T ) {
            //     // std::cout << "No links cross rank boundary" << std::endl;
            //     min_part = Simulation_impl::getTimeLord()->getSimCycles("1us","");
            // }

            MPI_Allreduce(&local_min_part, &min_part, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD);
            // Comms::broadcast(min_part, 0);
#endif
        }
        ////// End Calculate Minimum Partitioning //////

        ////// Write out the graph, if requested //////
        if ( myRank.rank == 0 ) {
            doSerialOnlyGraphOutput(&cfg, graph);

            if ( !cfg.parallel_output() ) { doParallelCapableGraphOutput(&cfg, graph, myRank, world_size); }
        }

        ////// Broadcast Graph //////
#ifdef SST_CONFIG_HAVE_MPI
        if ( world_size.rank > 1 && !cfg.parallel_load() ) {
            try {
                Comms::broadcast(Params::keyMap, 0);
                Comms::broadcast(Params::keyMapReverse, 0);
                Comms::broadcast(Params::nextKeyID, 0);
                Comms::broadcast(Params::global_params, 0);

                std::set<uint32_t> my_ranks;
                std::set<uint32_t> your_ranks;

                if ( 0 == myRank.rank ) {
                    // Split the rank space in half
                    for ( uint32_t i = 0; i < world_size.rank / 2; i++ ) {
                        my_ranks.insert(i);
                    }

                    for ( uint32_t i = world_size.rank / 2; i < world_size.rank; i++ ) {
                        your_ranks.insert(i);
                    }

                    // Need to send the your_ranks set and the proper
                    // subgraph for further distribution
                    // ConfigGraph* your_graph = graph->getSubGraph(your_ranks);
                    ConfigGraph* your_graph = graph->splitGraph(my_ranks, your_ranks);
                    int          dest       = *your_ranks.begin();
                    Comms::send(dest, 0, your_ranks);
                    Comms::send(dest, 0, *your_graph);
                    your_ranks.clear();
                    delete your_graph;
                }
                else {
                    Comms::recv(MPI_ANY_SOURCE, 0, my_ranks);
                    Comms::recv(MPI_ANY_SOURCE, 0, *graph);
                }

                while ( my_ranks.size() != 1 ) {
                    // This means I have more data to pass on to other ranks
                    std::set<uint32_t>::iterator mid = my_ranks.begin();
                    for ( unsigned int i = 0; i < my_ranks.size() / 2; i++ ) {
                        ++mid;
                    }

                    your_ranks.insert(mid, my_ranks.end());
                    my_ranks.erase(mid, my_ranks.end());

                    // // ConfigGraph* your_graph = graph->getSubGraph(your_ranks);
                    ConfigGraph* your_graph = graph->splitGraph(my_ranks, your_ranks);

                    uint32_t dest = *your_ranks.begin();

                    Comms::send(dest, 0, your_ranks);
                    Comms::send(dest, 0, *your_graph);
                    your_ranks.clear();
                    delete your_graph;
                }
            }
            catch ( std::exception& e ) {
                g_output.fatal(CALL_INFO, -1, "Error encountered during graph broadcast: %s\n", e.what());
            }
        }
#endif

        ////// End Broadcast Graph //////
        if ( cfg.parallel_output() ) { doParallelCapableGraphOutput(&cfg, graph, myRank, world_size); }
    } // end if ( !restart )

    if ( cfg.enable_sig_handling() ) {
        g_output.verbose(CALL_INFO, 1, 0, "Signal handlers will be registered for USR1, USR2, INT and TERM...\n");
        setupSignals(0);
    }
    else {
        // Print out to say disabled?
        g_output.verbose(CALL_INFO, 1, 0, "Signal handlers are disabled by user input\n");
    }


    ////// Create Simulation //////
    Core::ThreadSafe::Barrier mainBarrier(world_size.thread);

    Simulation_impl::factory    = factory;
    Simulation_impl::sim_output = g_output;
    Simulation_impl::resizeBarriers(world_size.thread);
#ifdef USE_MEMPOOL
    MemPoolAccessor::initializeGlobalData(world_size.thread, cfg.cache_align_mempools());
#endif

    std::vector<std::thread>     threads(world_size.thread);
    std::vector<SimThreadInfo_t> threadInfo(world_size.thread);
    for ( uint32_t i = 0; i < world_size.thread; i++ ) {
        threadInfo[i].myRank        = myRank;
        threadInfo[i].myRank.thread = i;
        threadInfo[i].world_size    = world_size;
        threadInfo[i].config        = &cfg;
        threadInfo[i].graph         = graph;
        threadInfo[i].min_part      = min_part;
    }

    double end_serial_build = sst_get_cpu_time();

    try {
        Output::setThreadID(std::this_thread::get_id(), 0);
        for ( uint32_t i = 1; i < world_size.thread; i++ ) {
            threads[i] = std::thread(start_simulation, i, std::ref(threadInfo[i]), std::ref(mainBarrier));
            Output::setThreadID(threads[i].get_id(), i);
        }

        start_simulation(0, threadInfo[0], mainBarrier);
        for ( uint32_t i = 1; i < world_size.thread; i++ ) {
            threads[i].join();
        }

        Simulation_impl::shutdown();
    }
    catch ( std::exception& e ) {
        g_output.fatal(CALL_INFO, -1, "Error encountered during simulation: %s\n", e.what());
    }

    double total_end_time = sst_get_cpu_time();

    for ( uint32_t i = 1; i < world_size.thread; i++ ) {
        threadInfo[0].simulated_time = std::max(threadInfo[0].simulated_time, threadInfo[i].simulated_time);
        threadInfo[0].run_time       = std::max(threadInfo[0].run_time, threadInfo[i].run_time);
        threadInfo[0].build_time     = std::max(threadInfo[0].build_time, threadInfo[i].build_time);

        threadInfo[0].max_tv_depth = std::max(threadInfo[0].max_tv_depth, threadInfo[i].max_tv_depth);
        threadInfo[0].current_tv_depth += threadInfo[i].current_tv_depth;
        threadInfo[0].sync_data_size += threadInfo[i].sync_data_size;
    }

    double build_time = (end_serial_build - start) + threadInfo[0].build_time;
    double run_time   = threadInfo[0].run_time;
    double total_time = total_end_time - start;

    double max_run_time = 0, max_build_time = 0, max_total_time = 0;

    uint64_t local_max_tv_depth      = threadInfo[0].max_tv_depth;
    uint64_t global_max_tv_depth     = 0;
    uint64_t local_current_tv_depth  = threadInfo[0].current_tv_depth;
    uint64_t global_current_tv_depth = 0;

    uint64_t global_max_sync_data_size = 0, global_sync_data_size = 0;

    int64_t mempool_size = 0, max_mempool_size = 0, global_mempool_size = 0;
    int64_t active_activities = 0, global_active_activities = 0;
    Core::MemPoolAccessor::getMemPoolUsage(mempool_size, active_activities);

#ifdef SST_CONFIG_HAVE_MPI
    uint64_t local_sync_data_size = threadInfo[0].sync_data_size;

    MPI_Allreduce(&run_time, &max_run_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&build_time, &max_build_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&total_time, &max_total_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local_max_tv_depth, &global_max_tv_depth, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local_current_tv_depth, &global_current_tv_depth, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&local_sync_data_size, &global_max_sync_data_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local_sync_data_size, &global_sync_data_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&mempool_size, &max_mempool_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&mempool_size, &global_mempool_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&active_activities, &global_active_activities, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
#else
    max_build_time = build_time;
    max_run_time = run_time;
    max_total_time = total_time;
    global_max_tv_depth = local_max_tv_depth;
    global_current_tv_depth = local_current_tv_depth;
    global_max_sync_data_size = 0;
    global_max_sync_data_size = 0;
    max_mempool_size = mempool_size;
    global_mempool_size = mempool_size;
    global_active_activities = active_activities;
#endif

    const uint64_t local_max_rss     = maxLocalMemSize();
    const uint64_t global_max_rss    = maxGlobalMemSize();
    const uint64_t local_max_pf      = maxLocalPageFaults();
    const uint64_t global_pf         = globalPageFaults();
    const uint64_t global_max_io_in  = maxInputOperations();
    const uint64_t global_max_io_out = maxOutputOperations();

    if ( myRank.rank == 0 && (cfg.verbose() || cfg.print_timing()) ) {
        std::string ua_buffer;
        ua_buffer = format_string("%" PRIu64 "KB", local_max_rss);
        UnitAlgebra max_rss_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "KB", global_max_rss);
        UnitAlgebra global_rss_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "B", global_max_sync_data_size);
        UnitAlgebra global_max_sync_data_size_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "B", global_sync_data_size);
        UnitAlgebra global_sync_data_size_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "B", max_mempool_size);
        UnitAlgebra max_mempool_size_ua(ua_buffer);

        ua_buffer = format_string("%" PRIu64 "B", global_mempool_size);
        UnitAlgebra global_mempool_size_ua(ua_buffer);

        g_output.output("\n");
        g_output.output("\n");
        g_output.output("------------------------------------------------------------\n");
        g_output.output("Simulation Timing Information (Wall Clock Times):\n");
        g_output.output("  Build time:                      %f seconds\n", max_build_time);
        g_output.output("  Run loop time:                   %f seconds\n", max_run_time);
        g_output.output("  Total time:                      %f seconds\n", max_total_time);
        g_output.output("\n");
        g_output.output(
            "Simulated time:                    %s\n", threadInfo[0].simulated_time.toStringBestSI().c_str());
        g_output.output("\n");
        g_output.output("Simulation Resource Information:\n");
        g_output.output("  Max Resident Set Size:           %s\n", max_rss_ua.toStringBestSI().c_str());
        g_output.output("  Approx. Global Max RSS Size:     %s\n", global_rss_ua.toStringBestSI().c_str());
        g_output.output("  Max Local Page Faults:           %" PRIu64 " faults\n", local_max_pf);
        g_output.output("  Global Page Faults:              %" PRIu64 " faults\n", global_pf);
        g_output.output("  Max Output Blocks:               %" PRIu64 " blocks\n", global_max_io_out);
        g_output.output("  Max Input Blocks:                %" PRIu64 " blocks\n", global_max_io_in);
        g_output.output("  Max mempool usage:               %s\n", max_mempool_size_ua.toStringBestSI().c_str());
        g_output.output("  Global mempool usage:            %s\n", global_mempool_size_ua.toStringBestSI().c_str());
        g_output.output("  Global active activities:        %" PRIu64 " activities\n", global_active_activities);
        g_output.output("  Current global TimeVortex depth: %" PRIu64 " entries\n", global_current_tv_depth);
        g_output.output("  Max TimeVortex depth:            %" PRIu64 " entries\n", global_max_tv_depth);
        g_output.output(
            "  Max Sync data size:              %s\n", global_max_sync_data_size_ua.toStringBestSI().c_str());
        g_output.output("  Global Sync data size:           %s\n", global_sync_data_size_ua.toStringBestSI().c_str());
        g_output.output("------------------------------------------------------------\n");
        g_output.output("\n");
        g_output.output("\n");
    }

#ifdef SST_CONFIG_HAVE_MPI
    if ( 0 == myRank.rank ) {
#endif
        // Print out the simulation time regardless of verbosity.
        g_output.output(
            "Simulation is complete, simulated time: %s\n", threadInfo[0].simulated_time.toStringBestSI().c_str());
#ifdef SST_CONFIG_HAVE_MPI
    }
#endif

#ifdef USE_MEMPOOL
    if ( cfg.event_dump_file() != "" ) {
        bool   print_header = false;
        Output out("", 0, 0, Output::FILE, cfg.event_dump_file());
        if ( cfg.event_dump_file() == "STDOUT" || cfg.event_dump_file() == "stdout" ) {
            out.setOutputLocation(Output::STDOUT);
            print_header = true;
        }
        if ( cfg.event_dump_file() == "STDERR" || cfg.event_dump_file() == "stderr" ) {
            out.setOutputLocation(Output::STDERR);
            print_header = true;
        }
        if ( print_header ) {
#ifdef SST_CONFIG_HAVE_MPI
            MPI_Barrier(MPI_COMM_WORLD);
#endif
            if ( 0 == myRank.rank ) { out.output("\nUndeleted Mempool Items:\n"); }
#ifdef SST_CONFIG_HAVE_MPI
            MPI_Barrier(MPI_COMM_WORLD);
#endif
        }
        MemPoolAccessor::printUndeletedMemPoolItems("  ", out);
    }
#endif

#ifdef SST_CONFIG_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
