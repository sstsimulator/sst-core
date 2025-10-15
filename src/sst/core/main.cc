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

#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
// The Python header already defines this and should override one from the
// command line.
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#include <Python.h>
REENABLE_WARNING

#include "sst/core/activity.h"
#include "sst/core/checkpointAction.h"
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
#include "sst/core/realtime.h"
#include "sst/core/shared/sharedObject.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/sst_mpi.h"
#include "sst/core/statapi/statengine.h"
#include "sst/core/stringize.h"
#include "sst/core/threadsafe.h"
#include "sst/core/timeLord.h"
#include "sst/core/timeVortex.h"
#include "sst/core/timingOutput.h"
#include "sst/core/unitAlgebra.h"

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
initialize_unitalgebra()
{
    Units::registerBaseUnit("s");
    Units::registerBaseUnit("B");
    Units::registerBaseUnit("b");
    Units::registerBaseUnit("events");

    Units::registerCompoundUnit("Hz", "1/s");
    // Yes, I know it's wrong, but other people don't always realize that
    Units::registerCompoundUnit("hz", "1/s");
    Units::registerCompoundUnit("Bps", "B/s");
    Units::registerCompoundUnit("bps", "b/s");
    Units::registerCompoundUnit("event", "events");
}

static void
dump_partition(ConfigGraph* graph, const RankInfo& size)
{
    Config& cfg = Simulation_impl::config;

    ///////////////////////////////////////////////////////////////////////
    // If the user asks us to dump the partitioned graph.
    if ( cfg.component_partition_file() != "" ) {
        if ( cfg.verbose() ) {
            g_output.verbose(CALL_INFO, 1, 0, "# Dumping partitioned component graph to %s\n",
                cfg.component_partition_file().c_str());
        }

        std::ofstream         graph_file(cfg.component_partition_file().c_str());
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

        if ( cfg.verbose() ) {
            g_output.verbose(CALL_INFO, 2, 0, "# Dump of partition graph is complete.\n");
        }
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

// Functions to do shared (static) initialization and notification for
// stats engines.  Right now, the StatGroups are per MPI rank and
// everything else in StatEngine is per partition.
static void
do_statengine_static_initialization(StatsConfig* stats_config, const RankInfo& myRank)
{
    if ( myRank.thread != 0 ) return;
    StatisticProcessingEngine::static_setup(stats_config);
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
// static void
static void
do_statengine_initialization(StatsConfig* stats_config, SST::Simulation_impl* sim, const RankInfo& UNUSED(myRank))
{
    sim->initializeStatisticEngine(stats_config);
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
doSerialOnlyGraphOutput(ConfigGraph* graph)
{
    Config& cfg = Simulation_impl::config;
    // See if user asked us to dump the config graph in dot graph format
    if ( cfg.output_dot() != "" ) {
        DotConfigGraphOutput out(cfg.output_dot().c_str());
        out.generate(&cfg, graph);
    }
}

// This should only be called once in main().  Either before or after
// graph broadcast depending on if parallel_load is turned on or not.
// If on, call it after graph broadcast, if off, call it before.
static void
doParallelCapableGraphOutput(ConfigGraph* graph, const RankInfo& myRank, const RankInfo& world_size)
{
    Config& cfg = Simulation_impl::config;
    // User asked us to dump the config graph to a file in Python
    if ( cfg.output_config_graph() != "" ) {
        // See if we are doing parallel output
        std::string file_name(cfg.output_config_graph());
        if ( cfg.parallel_output() && world_size.rank != 1 ) {
            // Append rank number to base filename
            std::string ext = addRankToFileName(file_name, myRank.rank);
            if ( ext != ".py" ) {
                g_output.fatal(CALL_INFO, 1, "--output-config requires a filename with a .py extension\n");
            }
        }
        PythonConfigGraphOutput out(file_name.c_str());
        out.generate(&cfg, graph);
    }

    // User asked us to dump the config graph in JSON format
    if ( cfg.output_json() != "" ) {
        std::string file_name(cfg.output_json());
        if ( cfg.parallel_output() ) {
            // Append rank number to base filename
            std::string ext = addRankToFileName(file_name, myRank.rank);
            if ( ext != ".json" ) {
                g_output.fatal(CALL_INFO, 1, "--output-json requires a filename with a .json extension\n");
            }
        }
        JSONConfigGraphOutput out(file_name.c_str());
        out.generate(&cfg, graph);
    }
}

static void
start_graph_creation(ConfigGraph*& graph, const RankInfo& world_size, const RankInfo& myRank)
{
    Config&                  cfg    = Simulation_impl::config;
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
    std::unique_ptr<SSTModelDescription> modelGen;

    force_rank_sequential_start(cfg.rank_seq_startup(), myRank, world_size);

    if ( cfg.configFile() != "NONE" ) {
        // Get the file extension by finding the last .
        std::string extension = cfg.configFile().substr(cfg.configFile().find_last_of("."));

        std::string model_name;
        try {
            model_name = extension_map.at(extension);
        }
        catch ( std::exception& e ) {
            std::cerr << "Unsupported SDL file type: \"" << extension << "\"" << std::endl;
            SST_Exit(EXIT_FAILURE);
        }

        // If doing parallel load, make sure this model is parallel capable
        if ( cfg.parallel_load() && !SSTModelDescription::isElementParallelCapable(model_name) ) {
            std::cerr << "Model type for extension: \"" << extension << "\" does not support parallel loading."
                      << std::endl;
            SST_Exit(EXIT_FAILURE);
        }

        if ( myRank.rank == 0 || cfg.parallel_load() ) {
            modelGen.reset(
                Factory::createModelDescription(model_name, cfg.configFile(), cfg.verbose(), cfg, sst_get_cpu_time()));
        }
    }

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
}

static double
start_partitioning(const RankInfo& world_size, const RankInfo& myRank, Factory* factory, ConfigGraph* graph)
{
    Config& cfg        = Simulation_impl::config;
    ////// Start Partitioning //////
    double  start_part = sst_get_cpu_time();

    if ( !cfg.parallel_load() ) {
        // Normal partitioning

        // // If this is a serial job, just use the single partitioner,
        // // but the same code path
        // if ( world_size.rank == 1 && world_size.thread == 1 ) cfg.partitioner_ = "sst.single";

        // Get the partitioner.  Built in partitioners are in the "sst" library.
        SSTPartitioner* partitioner = factory->CreatePartitioner(cfg.partitioner(), world_size, myRank, cfg.verbose());
        try {
            if ( partitioner->requiresConfigGraph() ) {
                partitioner->performPartition(graph);
            }
            else {
                PartitionGraph* pgraph;
                if ( myRank.rank == 0 ) {
                    pgraph = graph->getCollapsedPartitionGraph();
                }
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
    // Config*      config;
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
start_simulation(uint32_t tid, SimThreadInfo_t& info, Core::ThreadSafe::Barrier& barrier, SimTime_t currentSimCycle,
    int currentPriority)
{
    Config& cfg = Simulation_impl::config;

    // Setup Mempools
    Core::MemPoolAccessor::initializeLocalData(tid);
    info.myRank.thread = tid;

    bool restart = cfg.load_from_checkpoint();

    ////// Create Simulation Objects //////
    SST::Simulation_impl* sim =
        Simulation_impl::createSimulation(info.myRank, info.world_size, restart, currentSimCycle, currentPriority);
    double start_run = 0.0;

    // Thread zero needs to initialize the checkpoint data structures
    // if any checkpointing options were turned on.  This will return
    // an empty string if checkpointing was not enabled.

    if ( tid == 0 ) {
        sim->checkpoint_directory_ = Checkpointing::initializeCheckpointInfrastructure(
            &cfg, sim->real_time_->canInitiateCheckpoint(), info.myRank.rank);

        // if ( sim->checkpoint_directory_ != "" ) {
        //     // Write out any data structures needed for all checkpoints
        // }
    }
    // Wait for all checkpointing files to be initialized
    barrier.wait();

    double start_build = sst_get_cpu_time();

    StatsConfig* stats_config = nullptr;

    if ( !restart ) {
        sim->processGraphInfo(*info.graph, info.myRank, info.min_part);

        barrier.wait();
        stats_config = info.graph->getStatsConfig();
    }
    else {
        stats_config = Simulation_impl::stats_config_;
    }

    // Setup the stats engine
    force_rank_sequential_start(cfg.rank_seq_startup(), info.myRank, info.world_size);

    barrier.wait();

    if ( tid == 0 ) {
        do_statengine_static_initialization(stats_config, info.myRank);
    }
    barrier.wait();

    do_statengine_initialization(stats_config, sim, info.myRank);
    barrier.wait();

    force_rank_sequential_stop(cfg.rank_seq_startup(), info.myRank, info.world_size);
    barrier.wait();

    if ( restart ) {

        // Finish parsing checkpoint for restart
        sim->restart();

        barrier.wait();

        if ( info.myRank.thread == 0 ) {
            sim->exchangeLinkInfo();
        }

        barrier.wait();

    } // if ( restart )


    // Setup the real time actions (all of these have to be defined on
    // the command-line or SDL file, they will not be checkpointed and
    // restored
    sim->setupSimActions();


    if ( !restart ) {

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
        // 3-5 minutes versus less than a second.
        for ( uint32_t i = 0; i < info.world_size.thread; ++i ) {
            if ( i == info.myRank.thread ) {
                do_link_preparation(info.graph, sim, info.myRank, info.min_part);
            }
            barrier.wait();
        }
#else
        do_link_preparation(info.graph, sim, info.myRank, info.min_part);
#endif
        barrier.wait();

        // Create all the simulation components
        do_graph_wireup(info.graph, sim, info.myRank, info.min_part);
        barrier.wait();

        if ( tid == 0 ) {
            // Store off the stats config for use with checkpoints
            Simulation_impl::stats_config_ = info.graph->takeStatsConfig();
            delete info.graph;
        }

        force_rank_sequential_stop(cfg.rank_seq_startup(), info.myRank, info.world_size);

        barrier.wait();

        if ( info.myRank.thread == 0 ) {
            sim->exchangeLinkInfo();
        }

        barrier.wait();

    } // if ( !restart)

    start_run       = sst_get_cpu_time();
    info.build_time = start_run - start_build;

    if ( tid == 0 && info.world_size.rank > 1 ) {
        SST_MPI_Barrier(MPI_COMM_WORLD);
    }

    if ( !restart ) {
        barrier.wait();

        if ( cfg.runMode() == SimulationRunMode::RUN || cfg.runMode() == SimulationRunMode::BOTH ) {
            if ( cfg.verbose() && 0 == tid ) {
                g_output.verbose(CALL_INFO, 1, 0, "# Starting main event loop\n");

                time_t     the_time = time(nullptr);
                struct tm* now      = localtime(&the_time);

                g_output.verbose(CALL_INFO, 1, 0, "# Start time: %04u/%02u/%02u at: %02u:%02u:%02u\n",
                    (now->tm_year + 1900), (now->tm_mon + 1), now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
            }

            if ( tid == 0 && info.world_size.rank > 1 ) {
                // If we are a MPI_parallel job, need to makes sure that all used
                // libraries are loaded on all ranks.
#ifdef SST_CONFIG_HAVE_MPI
                std::set<std::string> lib_names;
                std::set<std::string> other_lib_names;
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

            if ( tid == 0 ) {
                Simulation_impl::basicPerf.endRegion("construct");
                Simulation_impl::basicPerf.endRegion("build");

                Simulation_impl::basicPerf.beginRegion("execute");
            }


            if ( tid == 0 ) Simulation_impl::basicPerf.beginRegion("init");
            sim->initialize();
            barrier.wait();
            if ( tid == 0 ) Simulation_impl::basicPerf.endRegion("init");

            /* Run Set */
            if ( tid == 0 ) Simulation_impl::basicPerf.beginRegion("setup");
            sim->setup();
            barrier.wait();

            /* Finalize all the stat outputs */
            do_statoutput_start_simulation(info.myRank);
            barrier.wait();

            sim->prepare_for_run();
            if ( tid == 0 ) Simulation_impl::basicPerf.endRegion("setup");
        } // end !RUNMODE == INIT
        else {
            // Need to all all the regions that were skipped because
            // RUNMODE was set to INIT
            if ( tid == 0 ) {
                Simulation_impl::basicPerf.endRegion("construct");
                Simulation_impl::basicPerf.endRegion("build");

                Simulation_impl::basicPerf.beginRegion("execute");

                Simulation_impl::basicPerf.beginRegion("init");
                Simulation_impl::basicPerf.endRegion("init");

                Simulation_impl::basicPerf.beginRegion("setup");
                Simulation_impl::basicPerf.endRegion("setup");
            }
        }
    } // end if !restart
    else {
        // Just need to add all the regions that were skipped to the
        // basicPerf object
        if ( tid == 0 ) {
            Simulation_impl::basicPerf.endRegion("construct");
            Simulation_impl::basicPerf.endRegion("build");

            Simulation_impl::basicPerf.beginRegion("execute");

            Simulation_impl::basicPerf.beginRegion("init");
            Simulation_impl::basicPerf.endRegion("init");

            Simulation_impl::basicPerf.beginRegion("setup");
            Simulation_impl::basicPerf.endRegion("setup");
        }
    }

    /* Run Simulation */
    if ( cfg.runMode() == SimulationRunMode::RUN || cfg.runMode() == SimulationRunMode::BOTH ) {
        if ( tid == 0 ) Simulation_impl::basicPerf.beginRegion("run");
        sim->run();
        barrier.wait();
        if ( tid == 0 ) Simulation_impl::basicPerf.endRegion("run");

        /* Adjust clocks at simulation end to
         * reflect actual simulation end if that
         * differs from detected simulation end
         */
        sim->adjustTimeAtSimEnd();
        barrier.wait();

        if ( tid == 0 ) Simulation_impl::basicPerf.beginRegion("complete");
        sim->complete();
        barrier.wait();
        if ( tid == 0 ) Simulation_impl::basicPerf.endRegion("complete");

        if ( tid == 0 ) Simulation_impl::basicPerf.beginRegion("finish");
        sim->finish();
        barrier.wait();

        /* Tell stat outputs simulation is done */
        do_statoutput_end_simulation(info.myRank);
        barrier.wait();
        if ( tid == 0 ) Simulation_impl::basicPerf.endRegion("finish");
    }
    else {
        // For RUNMODE set to INIT
        if ( tid == 0 ) {
            Simulation_impl::basicPerf.beginRegion("run");
            Simulation_impl::basicPerf.endRegion("run");

            Simulation_impl::basicPerf.beginRegion("complete");
            Simulation_impl::basicPerf.endRegion("complete");

            Simulation_impl::basicPerf.beginRegion("finish");
            Simulation_impl::basicPerf.endRegion("finish");
        }
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
    std::string file = cfg.profiling_output();
    if ( file == "stdout" ) {
        // Output to the console, so we will force both rank and
        // thread output to be sequential
        force_rank_sequential_start(info.world_size.rank > 1, info.myRank, info.world_size);

        for ( uint32_t i = 0; i < info.world_size.thread; ++i ) {
            if ( i == info.myRank.thread ) {
                sim->printProfilingInfo(stdout);
            }
            barrier.wait();
        }

        force_rank_sequential_stop(info.world_size.rank > 1, info.myRank, info.world_size);
        barrier.wait();
    }
    else {
        // Output to file
        if ( info.world_size.rank > 1 ) {
            addRankToFileName(file, info.myRank.rank);
        }

        // First thread will open a new file
        std::string mode;
        // Thread 0 will open a new file, all others will append
        if ( info.myRank.thread == 0 )
            mode = "w";
        else
            mode = "a";

        for ( uint32_t i = 0; i < info.world_size.thread; ++i ) {
            if ( i == info.myRank.thread ) {
                fp = Simulation_impl::filesystem.fopen(file, mode.c_str());
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
    using SST::Util::BasicPerfTracker;

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
    int      myrank = 0;
    int      mysize = 1;
    RankInfo world_size(1, 1);
    RankInfo myRank(0, 0);
#endif

    // Initialize the performance tracker
    Simulation_impl::basicPerf.initialize(myrank, mysize);

    // Mark the start of the full run region
    Simulation_impl::basicPerf.beginRegion("total");

    /************************************************************************************
      Here are the major phases of simulation as represented in main()
      and its corresponding functions.  Each of these regions will
      track performance stats using the BasicPerfTracker. Note that
      things are synchronized between each of these phases, so only
      thread 0 will track time.

          1 - Config object initialization (performance not tracked)
          2 - Build phase
            a - Graph processing
              1 - Model Generation (ConfigGraph build)
              2 - Graph Partitioning
              3 - Graph distribution
            b - Simulation object construction
          3 - Execution phase
            a - init()
            b - setup()
            c - run()
            d - complete()
            e - finish()
          4 - Destruct the simulation objects
    *************************************************************************************/

    /**************************************************************************
      1 - Config Object Initialization

        During Config Object Initialization, the command line is
        parsed and the Config object is initialized
    ***************************************************************************/
    Simulation_impl::config.initialize(world_size.rank, myrank == 0);
    Config& cfg = Simulation_impl::config;

    initialize_unitalgebra();

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

    /**************************************************************************
      2 - Build phase

        Build reads the input file and creates the simulation objects
        in preparation for the execution phase.

        Sub-phases:
        a - ConfigGraph processing
        b - Simulation object construction
    ***************************************************************************/
    Simulation_impl::basicPerf.beginRegion("build");

    /**************************************************************************
      2.a - ConfigGraph Processing

        The purpose of graph processing is to create the ConfigGraph
        object, finalize the Config object (model generation can
        change Config options) and ensure that each rank has the data
        it needs to construct the simulation objects.  This phase also
        initializes the Factory at the appropriate time. This
        includes:

        1 - Model generation. Generate the ConfigGraph, finalize the
            Config object and decide if this is a restart or regular
            run

        2 - Partition the graph.  This phase is skipped for serial
            jobs, normal jobs that were loaded in parallel, and for
            restarted jobs that have no repartitioning.

        3 - Distribute the graph.  This phase splits up the graph and
            distributes it to all the ranks.  This phase is skipped
            for serial jobs, normal jobs that were loaded in parallel,
            and for restarted jobs that have no repartitioning.

        Outputs:

          - Config object with finalized options set
          - Distributed ConfigGraph and Config objects
          - Factory object initialized
          - Output object initialized
          - TimeLord initialized
          - Set currentSimCycle and currentPriority to use for
            Simulation object construction.
    ***************************************************************************/
    Simulation_impl::basicPerf.beginRegion("graph-processing");

    // Need to ensure that the input file exists

    // If we are doing a parallel load with a file per rank, add the
    // rank number to the file name before the extension
    if ( cfg.parallel_load() && cfg.parallel_load_mode_multi() && world_size.rank != 1 ) {
        addRankToFileName(cfg.configFile_.value, myRank.rank);
    }

    if ( cfg.checkConfigFile() == false ) {
        return -1; /* checkConfigFile provides error message */
    }


    /**************************************************************************
      2.a.1 - Model generation

        Creates the ConfigGraph, merges any program options set in the
        SDL file and/or carried over from a checkpoint.  This stage
        will also determine the currentSimCycle and currentPriority,
        which is 0, 0 for a normal run and read from the checkpoint
        for a restart run. This phase also initializes the Factory and
        updates the path after the model generation.

        Outputs of this phase are the Config object with finalized
        options set, the ConfigGraph (if needed), and currentSimCycle
        and currentPriority to use for Simulation object construction.
        The ConfigGraph isn't needed for restart runs that use the
        original partitioning.
    ***************************************************************************/
    Simulation_impl::basicPerf.beginRegion("model-generation");

    ConfigGraph* graph    = nullptr;
    SimTime_t    min_part = 0xffffffffffffffffl;

    // Count of the number of components
    uint64_t comp_count = 0;

    Simulation_impl::basicPerf.beginRegion("model-execution");
    Factory::createFactory(cfg.getLibPath());
    start_graph_creation(graph, world_size, myRank);

    Factory::getFactory()->updateSearchPaths(cfg.getLibPath());
    // Check to see if we are doing a restart from a checkpoint
    bool restart = cfg.load_from_checkpoint();


    Simulation_impl::basicPerf.endRegion("model-execution");

    //// Initialize global data that needed to wait until Config was
    //// possibly updated by the SDL file

    // Set the number of threads
    world_size.thread = cfg.num_threads();

    // Create global output object
    Output::setFileName(cfg.debugFile() != "/dev/null" ? cfg.debugFile() : "sst_output");
    Output::setWorldSize(world_size.rank, world_size.thread, myrank);
    g_output = Output::setDefaultObject(cfg.output_core_prefix(), cfg.verbose(), 0, Output::STDOUT);

    g_output.verbose(CALL_INFO, 1, 0, "#main() My rank is (%u.%u), on %u/%u nodes/threads\n", myRank.rank,
        myRank.thread, world_size.rank, world_size.thread);

    // TimeLord must be initialized prior to postCreationCleanup() call
    Simulation_impl::getTimeLord()->init(cfg.timeBase());

    // Check the ConfigGraph and finalize things

    // Cleanup after graph creation, but only if rank participated
    // in graph construction
    if ( myRank.rank == 0 || cfg.parallel_load() ) {

        Simulation_impl::basicPerf.beginRegion("graph-cleanup");
        if ( cfg.parallel_load() ) graph->reduceGraphToSingleRank(myRank.rank);
        graph->postCreationCleanup();
        Simulation_impl::basicPerf.endRegion("graph-cleanup");

        // Check config graph to see if there are structural errors.
        Simulation_impl::basicPerf.beginRegion("graph-error-check");
        if ( graph->checkForStructuralErrors() ) {
            g_output.fatal(CALL_INFO, 1, "Structure errors found in the ConfigGraph.\n");
        }
        Simulation_impl::basicPerf.endRegion("graph-error-check");
    }
    else {
        Simulation_impl::basicPerf.beginRegion("graph-cleanup");
        Simulation_impl::basicPerf.endRegion("graph-cleanup");

        Simulation_impl::basicPerf.beginRegion("graph-error-check");
        Simulation_impl::basicPerf.endRegion("graph-error-check");
    }

    // Compute the total number components in the simulation.
    if ( !restart ) {
        if ( !cfg.parallel_load() && myRank.rank == 0 ) {
            comp_count = graph->getNumComponents();
        }
        else if ( cfg.parallel_load() ) {
            uint64_t my_count = graph->getNumComponentsInMPIRank(myRank.rank);
            SST_MPI_Allreduce(&my_count, &comp_count, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
        }
        Simulation_impl::basicPerf.addMetric("component count", comp_count);
    }


    // Set up the Filesystem object with the specified output directory
    bool out_dir_okay = Simulation_impl::filesystem.setBasePath(cfg.output_directory());
    if ( !out_dir_okay ) {
        fprintf(stderr,
            "ERROR: Directory specified with --output-directory (%s) is not valid.  Most likely causes are that the "
            "user does not have permissions to write to this path, or a file of the same name exists.\n",
            cfg.output_directory().c_str());
        return -1;
    }

    Simulation_impl::basicPerf.endRegion("model-generation");

    if ( myRank.rank == 0 ) {
        // Get the global and max memory usage.  These calls will generate
        // implicit collectives so all ranks have to call them
        uint64_t global_mem_begin = Simulation_impl::basicPerf.getGlobalTotalRegionBeginMemSize("model-generation");
        uint64_t global_mem_end   = Simulation_impl::basicPerf.getGlobalTotalRegionEndMemSize("model-generation");
        int64_t  global_mem_diff  = global_mem_end - global_mem_begin;
        std::pair<uint64_t, int> max_mem = Simulation_impl::basicPerf.getGlobalMaxRegionEndMemSize("model-generation");

        double graph_gen_time = Simulation_impl::basicPerf.getRegionDuration("model-generation");
        g_output.verbose(CALL_INFO, 1, 0, "# ------------------------------------------------------------\n");
        g_output.verbose(CALL_INFO, 1, 0, "# Graph construction took %f seconds.\n", graph_gen_time);
        g_output.verbose(CALL_INFO, 1, 0, "# Global memory use is %" PRIu64 "kb (raised %" PRIi64 "kb)\n",
            global_mem_end, global_mem_diff);
        if ( world_size.rank > 1 )
            g_output.verbose(
                CALL_INFO, 1, 0, "# Max memory use is %" PRIu64 "kb (rank %d)\n", max_mem.first, max_mem.second);
        if ( !restart ) g_output.verbose(CALL_INFO, 1, 0, "# Graph contains %" PRIu64 " components\n", comp_count);
        g_output.verbose(CALL_INFO, 1, 0, "# ------------------------------------------------------------\n");
    }

    /******** End Model Generation ********/


    /**************************************************************************
      2.a.2 - Graph Partitioning

        Partitions the graph.  This phase is skipped for serial
        jobs, normal jobs that were loaded in parallel, and for
        restarted jobs that have no repartitioning.

        3 - Distribute the graph.  This phase splits up the graph and
            distributes it to all the ranks.  This phase is skipped
            for serial jobs, normal jobs that were loaded in parallel,
            and for restarted jobs that have no repartitioning.

        Outputs of this phase are the Config object with finalized
        options set, a distributed ConfigGraph, and currentSimCycle
        and currentPriority to use for Simulation object construction.
    ***************************************************************************/

    // For now, nothing to do in the restart path
    Simulation_impl::basicPerf.beginRegion("graph-partitioning");
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
    start_partitioning(world_size, myRank, Factory::getFactory(), graph);
    ////// End Partitioning //////

    ////// Calculate Minimum Partitioning //////
    SimTime_t local_min_part = 0xffffffffffffffffl;
    if ( world_size.rank > 1 ) {
        // Check the graph for the minimum latency crossing a partition boundary
        if ( myRank.rank == 0 || cfg.parallel_load() ) {
            local_min_part = graph->getMinimumPartitionLatency();
        }

        // Fix for case that probably doesn't matter in practice, but
        // does come up during some specific testing.  If there are no
        // links that cross the boundary and we're a multi-rank job,
        // we need to put in a sync interval to look for the exit
        // conditions being met.
        // if ( min_part == MAX_SIMTIME_T ) {
        //     // std::cout << "No links cross rank boundary" << std::endl;
        //     min_part = Simulation_impl::getTimeLord()->getSimCycles("1us","");
        // }

        SST_MPI_Allreduce(&local_min_part, &min_part, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD);
        // Comms::broadcast(min_part, 0);
    } // if ( world_size > 1 )
    ////// End Calculate Minimum Partitioning //////

    ////// Write out the graph, if requested //////
    if ( myRank.rank == 0 && !restart ) {
        doSerialOnlyGraphOutput(graph);

        if ( !cfg.parallel_output() ) {
            doParallelCapableGraphOutput(graph, myRank, world_size);
        }
    }

    Simulation_impl::basicPerf.endRegion("graph-partitioning");

    /**************************************************************************
      2.a.3 - Graph Distribution

        Distribute the graph.  This phase splits up the graph and
        distributes it to all the ranks.  This phase is skipped for
        serial jobs, normal jobs that were loaded in parallel, and for
        restarted jobs that have no repartitioning.
    ***************************************************************************/
    Simulation_impl::basicPerf.beginRegion("graph-distribution");


#ifdef SST_CONFIG_HAVE_MPI
    if ( world_size.rank > 1 && !cfg.parallel_load() ) {
        try {
            Comms::broadcast(Params::keyMap, 0);
            Comms::broadcast(Params::keyMapReverse, 0);
            Comms::broadcast(Params::nextKeyID, 0);
            Comms::broadcast(Params::shared_params, 0);

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
    if ( cfg.parallel_output() && !restart ) {
        doParallelCapableGraphOutput(graph, myRank, world_size);
    }
    Simulation_impl::basicPerf.endRegion("graph-distribution");
    Simulation_impl::basicPerf.endRegion("graph-processing");

    if ( myRank.rank == 0 ) {
        // Get the global and max memory usage.  These calls will generate
        // implicit collectives so all ranks have to call them
        uint64_t global_mem_begin = Simulation_impl::basicPerf.getGlobalTotalRegionBeginMemSize("graph-partitioning");
        uint64_t global_mem_end   = Simulation_impl::basicPerf.getGlobalTotalRegionEndMemSize("graph-distribution");
        uint64_t global_mem_diff  = global_mem_end - global_mem_begin;
        std::pair<uint64_t, int> max_mem =
            Simulation_impl::basicPerf.getGlobalMaxRegionEndMemSize("graph-distribution");

        double graph_gen_time = Simulation_impl::basicPerf.getRegionDuration("graph-distribution");
        g_output.verbose(CALL_INFO, 1, 0, "# ------------------------------------------------------------\n");
        g_output.verbose(
            CALL_INFO, 1, 0, "# Graph partitioning, output and distribution took %f seconds.\n", graph_gen_time);
        g_output.verbose(CALL_INFO, 1, 0, "# Global memory use is %" PRIu64 "kb (raised %" PRIi64 "kb)\n",
            global_mem_end, global_mem_diff);
        if ( world_size.rank > 1 )
            g_output.verbose(
                CALL_INFO, 1, 0, "# Max memory use is %" PRIu64 "kb (rank %d)\n", max_mem.first, max_mem.second);
        g_output.verbose(CALL_INFO, 1, 0, "# ------------------------------------------------------------\n");

        // Output the partition information if user requests it
        dump_partition(graph, world_size);
    }

    /******** Register signal handlers, if not disabled ********/
    if ( cfg.enable_sig_handling() ) {
        g_output.verbose(CALL_INFO, 1, 0, "Signal handlers will be registered for USR1, USR2, INT, ALRM, and TERM\n");
        RealTimeManager::installSignalHandlers();
    }
    else {
        // Print out to say disabled?
        g_output.verbose(CALL_INFO, 1, 0, "Signal handlers are disabled by user input\n");
    }

    /**************************************************************************
      2.b - Simulation Construction

        The purpose of simulation construction is to create all the
        objects needed to run the simulation.

        This region begins in the main thread, but ends after all
        other threads have been started.

        Outputs:

          - Simulation objects ready for execution
    ***************************************************************************/

    /**************************************************************************
      Phases that start/stop in other parts of the code:

      NOTE: These regions happen in the threads, so only thread 0 will
      record stats.  This will give a good estimate because the
      threads are either explicitly or implicitly synchronized between
      each region.

        3 - Execute

          The execute phase happens entirely in the threaded portion
          of the code.  Each thread executes the start_simulation()
          function, and start_simulation() makes calls into the
          Simulation object.  Those regions are detailed in the
          corresponding code sections.

        4 - Destruct

          The destruct phase happens entirely in the threaded portion
          of the code.
    ***************************************************************************/
    Core::ThreadSafe::Barrier mainBarrier(world_size.thread);

    Simulation_impl::basicPerf.beginRegion("construct");

    // Initialize Simulation object data members and barriers
    Simulation_impl::factory    = Factory::getFactory();
    Simulation_impl::sim_output = g_output;
    Simulation_impl::resizeBarriers(world_size.thread);
    CheckpointAction::barrier.resize(world_size.thread);
#ifdef USE_MEMPOOL
    MemPoolAccessor::initializeGlobalData(world_size.thread, cfg.cache_align_mempools());
#endif

    // On restart, need to intialize SharedObjectManager, stats_config_ and load libraries from the checkpoint
    if ( restart ) graph->restoreRestartData();

    std::vector<std::thread>     threads(world_size.thread);
    std::vector<SimThreadInfo_t> threadInfo(world_size.thread);
    for ( uint32_t i = 0; i < world_size.thread; i++ ) {
        threadInfo[i].myRank        = myRank;
        threadInfo[i].myRank.thread = i;
        threadInfo[i].world_size    = world_size;
        threadInfo[i].graph         = graph;
        threadInfo[i].min_part      = min_part;
    }

    /* Block signals for all threads */
    sigset_t maskset;
    sigfillset(&maskset);
    pthread_sigmask(SIG_BLOCK, &maskset, nullptr);

    // Start the remaining threads for the simulation
    try {
        Output::setThreadID(std::this_thread::get_id(), 0);
        for ( uint32_t i = 1; i < world_size.thread; i++ ) {
            threads[i] = std::thread(start_simulation, i, std::ref(threadInfo[i]), std::ref(mainBarrier),
                graph->cpt_currentSimCycle, graph->cpt_currentPriority);
            Output::setThreadID(threads[i].get_id(), i);
        }

        /* Unblock signals on thread 0 */
        pthread_sigmask(SIG_UNBLOCK, &maskset, NULL);
        // Call start_simulation for the main thread
        start_simulation(0, threadInfo[0], mainBarrier, graph->cpt_currentSimCycle, graph->cpt_currentPriority);

        // Join all the threads when the execute phase is done
        for ( uint32_t i = 1; i < world_size.thread; i++ ) {
            threads[i].join();
        }

        Simulation_impl::shutdown();
    }
    catch ( std::exception& e ) {
        g_output.fatal(CALL_INFO, -1, "Error encountered during simulation: %s\n", e.what());
    }
    Simulation_impl::basicPerf.endRegion("execute");

    Simulation_impl::basicPerf.endRegion("total");

    for ( uint32_t i = 1; i < world_size.thread; i++ ) {
        threadInfo[0].simulated_time = std::max(threadInfo[0].simulated_time, threadInfo[i].simulated_time);
        threadInfo[0].run_time       = std::max(threadInfo[0].run_time, threadInfo[i].run_time);
        threadInfo[0].build_time     = std::max(threadInfo[0].build_time, threadInfo[i].build_time);

        threadInfo[0].max_tv_depth = std::max(threadInfo[0].max_tv_depth, threadInfo[i].max_tv_depth);
        threadInfo[0].current_tv_depth += threadInfo[i].current_tv_depth;
        threadInfo[0].sync_data_size += threadInfo[i].sync_data_size;
    }

    double max_run_time   = Simulation_impl::basicPerf.getRegionDuration("run");
    double max_build_time = Simulation_impl::basicPerf.getRegionDuration("build");
    double max_total_time = Simulation_impl::basicPerf.getRegionDuration("total");

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

    MPI_Allreduce(&local_max_tv_depth, &global_max_tv_depth, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local_current_tv_depth, &global_current_tv_depth, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&local_sync_data_size, &global_max_sync_data_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local_sync_data_size, &global_sync_data_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&mempool_size, &max_mempool_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&mempool_size, &global_mempool_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&active_activities, &global_active_activities, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
#else
    global_max_tv_depth       = local_max_tv_depth;
    global_current_tv_depth   = local_current_tv_depth;
    global_max_sync_data_size = 0;
    global_max_sync_data_size = 0;
    max_mempool_size          = mempool_size;
    global_mempool_size       = mempool_size;
    global_active_activities  = active_activities;
#endif

    // These functions invoke MPI_Allreduce
    const uint64_t local_max_rss     = maxLocalMemSize();
    const uint64_t global_max_rss    = maxGlobalMemSize();
    const uint64_t local_max_pf      = maxLocalPageFaults();
    const uint64_t global_pf         = globalPageFaults();
    const uint64_t global_max_io_in  = maxInputOperations();
    const uint64_t global_max_io_out = maxOutputOperations();

    if ( cfg.verbose() || cfg.print_timing() || cfg.timing_json() != "" ) {
        if ( myRank.rank == 0 ) {
            int          timing_verbose = cfg.print_timing() == 0 ? (cfg.verbose() > 0 ? 2 : 0) : cfg.print_timing();
            TimingOutput timingOutput(g_output, timing_verbose);
            if ( cfg.timing_json() != "" ) timingOutput.setJSON(cfg.timing_json());

            timingOutput.set(TimingOutput::Key::LOCAL_MAX_RSS, local_max_rss);
            timingOutput.set(TimingOutput::Key::GLOBAL_MAX_RSS, global_max_rss);
            timingOutput.set(TimingOutput::Key::LOCAL_MAX_PF, local_max_pf);
            timingOutput.set(TimingOutput::Key::GLOBAL_PF, global_pf);
            timingOutput.set(TimingOutput::Key::GLOBAL_MAX_IO_IN, global_max_io_in);
            timingOutput.set(TimingOutput::Key::GLOBAL_MAX_IO_OUT, global_max_io_out);
            timingOutput.set(TimingOutput::Key::GLOBAL_MAX_SYNC_DATA_SIZE, global_max_sync_data_size);
            timingOutput.set(TimingOutput::Key::GLOBAL_SYNC_DATA_SIZE, global_sync_data_size);
            timingOutput.set(TimingOutput::Key::MAX_MEMPOOL_SIZE, (uint64_t)max_mempool_size);
            timingOutput.set(TimingOutput::Key::GLOBAL_MEMPOOL_SIZE, (uint64_t)global_mempool_size);
            timingOutput.set(TimingOutput::Key::MAX_BUILD_TIME, max_build_time);
            timingOutput.set(TimingOutput::Key::MAX_RUN_TIME, max_run_time);
            timingOutput.set(TimingOutput::Key::MAX_TOTAL_TIME, max_total_time);
            timingOutput.set(TimingOutput::Key::SIMULATED_TIME_UA, threadInfo[0].simulated_time);
            timingOutput.set(TimingOutput::Key::GLOBAL_ACTIVE_ACTIVITIES, (uint64_t)global_active_activities);
            timingOutput.set(TimingOutput::Key::GLOBAL_CURRENT_TV_DEPTH, global_current_tv_depth);
            timingOutput.set(TimingOutput::Key::GLOBAL_MAX_TV_DEPTH, global_max_tv_depth);
            timingOutput.set(TimingOutput::Key::RANKS, (uint64_t)world_size.rank);
            timingOutput.set(TimingOutput::Key::THREADS, (uint64_t)world_size.thread);
            timingOutput.generate();
        }
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
            if ( 0 == myRank.rank ) {
                out.output("\nUndeleted Mempool Items:\n");
            }
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
