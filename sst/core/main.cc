// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#ifdef HAVE_PYTHON
#include <Python.h>
#endif

#include "sst/core/serialization.h"


#ifdef HAVE_MPI
#include <boost/mpi.hpp>
#include <boost/mpi/timer.hpp>
#endif

#include <iomanip>
#include <iostream>
#include <fstream>
#include <signal.h>

#include <sst/core/archive.h>
#include <sst/core/config.h>
#include <sst/core/configGraph.h>
#include <sst/core/debug.h>
#include <sst/core/factory.h>
#include <sst/core/simulation.h>

#include <sst/core/part/simplepart.h>
#include <sst/core/part/rrobin.h>
#include <sst/core/part/linpart.h>

#include <sst/core/cputimer.h>

#ifdef HAVE_ZOLTAN
#include <sst/core/part/zoltpart.h>
#endif

#include <sst/core/model/sstmodel.h>
#include <sst/core/model/pymodel.h>
#include <sst/core/memuse.h>

#include <sys/resource.h>

using namespace SST::Core;
using namespace std;
using namespace SST;

static void
sigHandlerPrintStatus(int signal)
{
    Simulation::setSignal(signal);
}

static void dump_partition(SST::Output* sim_output, Config& cfg, ConfigGraph* graph,
	const int rank, const int size) {
	
	///////////////////////////////////////////////////////////////////////	
	// If the user asks us to dump the partionned graph.
	if(cfg.dump_component_graph_file != "" && rank == 0) {
		if(cfg.verbose) {
			sim_output->verbose(CALL_INFO, 2, 0,
				"# Dumping partitionned component graph to %s\n",
				cfg.dump_component_graph_file.c_str());
		}

		ofstream graph_file(cfg.dump_component_graph_file.c_str());
		ConfigComponentMap_t& component_map = graph->getComponentMap();

		for(int i = 0; i < size; i++) {
			graph_file << "Rank: " << i << " Component List:" << std::endl;

			for (ConfigComponentMap_t::const_iterator j = component_map.begin() ; j != component_map.end() ; ++j) {
				if(j->rank == i) {
					graph_file << "   " << j->name << " (ID=" << j->id << ")" << std::endl;
					graph_file << "      -> type      " << j->type << std::endl;
					graph_file << "      -> weight    " << j->weight << std::endl;
					graph_file << "      -> linkcount " << j->links.size() << std::endl;
					graph_file << "      -> rank      " << j->rank << std::endl;
				}
			}
		}

		graph_file.close();

		if(cfg.verbose) {
			sim_output->verbose(CALL_INFO, 2, 0,
				"# Dump of partition graph is complete.\n");
		}
	}
}

static void do_graph_wireup(SST::Output* sim_output, ConfigGraph* graph, 
	SST::Simulation* sim, SST::Config& cfg, const int size, const int rank) {
	
		if ( !graph->checkRanks( size ) ) {
			if ( rank == 0 ) {
				sim_output->fatal(CALL_INFO, 1,
					"ERROR: Bad partitionning; partition included unknown ranks.\n");
			}
		}
		else {
			if ( !graph->containsComponentInRank( rank ) ) {
				sim_output->output("WARNING: No components are assigned to rank: %d\n", 
					rank);
			}
		}


		// User asked us to dump the config graph to a file
		if(cfg.dump_config_graph != "") {
			graph->dumpToFile(cfg.dump_config_graph, &cfg, false);
		}
		if(cfg.output_dot != "") {
			graph->dumpToFile(cfg.output_dot, &cfg, true);
		}

		sim->performWireUp( *graph, rank );
}

int
main(int argc, char *argv[])
{

#ifdef HAVE_MPI
    boost::mpi::environment* mpiEnv = new boost::mpi::environment(argc,argv);
    boost::mpi::communicator world;
    const int rank = world.rank();
    const int size = world.size();
#else
    const int rank = 0;
    const int size = 1;
#endif
    Config cfg(rank, size);

    Output* sim_output = NULL;
    SST::Simulation* sim = NULL;

    // All ranks parse the command line
    if ( cfg.parseCmdLine(argc, argv) ) {
#ifdef HAVE_MPI
	delete mpiEnv;
#endif
        return -1;
    }

    SSTModelDescription* modelGen = 0;

    if ( cfg.sdlfile != "NONE" ) {
        string file_ext = "";

        if(cfg.sdlfile.size() > 3) {
            file_ext = cfg.sdlfile.substr(cfg.sdlfile.size() - 3);

            if(file_ext == "xml" || file_ext == "sdl") {
                cfg.model_options = cfg.sdlfile;
                cfg.sdlfile = SST_INSTALL_PREFIX "/libexec/xmlToPython.py";
                file_ext = ".py";
            }
            if(file_ext == ".py") {
                modelGen = new SSTPythonModelDefinition(cfg.sdlfile, cfg.verbose, &cfg);
            }
            else {
                std::cerr << "Unsupported SDL file type: " << file_ext << std::endl;
                return -1;
            }
        } else {
            return -1;
        }

    }

    Archive archive(cfg.archiveType, cfg.archiveFile);

    double start = sst_get_cpu_time();
    double end_build, start_run, end_run;

    if ( cfg.verbose ) printf("# main() My rank is %d, on %d nodes\n", rank, size);
    DebugInit( rank, size );

	if ( cfg.runMode == Config::INIT || cfg.runMode == Config::BOTH ) {
		// Now we have a config graph, create the simulation and configure signal handlers
		sim = Simulation::createSimulation(&cfg, rank, size);
		sim_output = &(Simulation::getSimulation()->getSimulationOutput());

		// Get the memory before we create the graph
		const uint64_t pre_graph_create_rss = maxGlobalMemSize();

		ConfigGraph* graph = NULL;

		if ( size == 1 ) {
			double start_graph_gen = sst_get_cpu_time();

			if ( cfg.generator != "NONE" ) {
				generateFunction func = sim->getFactory()->GetGenerator(cfg.generator);
				graph = new ConfigGraph();
				func(graph,cfg.generator_options,size);
			}
			else {
				graph = modelGen->createConfigGraph();
			}

			double end_graph_gen = sst_get_cpu_time();

			if(cfg.verbose && (rank == 0)) {
				sim_output->output("# ------------------------------------------------------------\n");
				sim_output->output("# Graph construction took %f seconds\n",
					(end_graph_gen - start_graph_gen));
			}

			// Check config graph to see if there are structural errors.
			if ( graph->checkForStructuralErrors() ) {
				sim_output->fatal(CALL_INFO, -1, "Structure errors found in the ConfigGraph.\n");
			}

			// Set all components to be instanced on rank 0 (the only
			// one that exists)
			graph->setComponentRanks(0);
		} else if ( cfg.partitioner == "zoltan" ) {
#ifdef HAVE_ZOLTAN
			double start_graph_gen = sst_get_cpu_time();
			graph = new ConfigGraph();

			if ( rank == 0 ) {
				if ( cfg.generator != "NONE" ) {
					generateFunction func = sim->getFactory()->GetGenerator(cfg.generator);
					func(graph,cfg.generator_options, size);
				} else {
					graph = modelGen->createConfigGraph();
				}

				// Check config graph to see if there are structural errors.
				if ( graph->checkForStructuralErrors() ) {
					sim_output->fatal(CALL_INFO, -1, "Structure errors found in the ConfigGraph.\n");
				}
			}

			double end_graph_gen = sst_get_cpu_time();

			if(cfg.verbose && (rank == 0)) {
        			sim_output->output("# ------------------------------------------------------------\n");
				sim_output->output("# Graph construction took %f seconds.\n",
					(end_graph_gen - start_graph_gen));
			}

			if(cfg.verbose && rank == 0) {
					sim_output->output("# Partitionning using Zoltan...\n");
			}

			SSTZoltanPartition* zolt_part = new SSTZoltanPartition(cfg.verbose);
			zolt_part->performPartition(graph);

			delete zolt_part;

			sim_output->output("# Graph construction took %f seconds.\n",
				(end_graph_gen - start_graph_gen));
#else
			sim_output->fatal(CALL_INFO, -1, "Zoltan support is not available. Configure did not find the Zoltan library.\n");
#endif
		} else if ( rank == 0 ) {
			// Perform partitionning for parallel jobs, not using Zoltan
			double start_graph_gen = sst_get_cpu_time();

			if ( cfg.generator != "NONE" ) {
				graph = new ConfigGraph();
				generateFunction func = sim->getFactory()->GetGenerator(cfg.generator);
				func(graph,cfg.generator_options, size);
			}
			else {
				graph = modelGen->createConfigGraph();
			}

			if ( rank == 0 ) {
				// Check config graph to see if there are structural errors.
				if ( graph->checkForStructuralErrors() ) {
					sim_output->fatal(CALL_INFO, -1, "Structure errors found in the ConfigGraph.\n");
				}
			}

			double end_graph_gen = sst_get_cpu_time();

			if(cfg.verbose && (rank == 0)) {
        			sim_output->output("# ------------------------------------------------------------\n");
				sim_output->output("# Graph construction took %f seconds.\n",
					(end_graph_gen - start_graph_gen));
			}

			double start_part = sst_get_cpu_time();

			// Do the partitioning.
			if ( cfg.partitioner != "self" ) {
				// If partitioning not specified by sdl or generator,
				// set all component ranks to -1 so it's easier to
				// detect some types of partitioning errors
				graph->setComponentRanks(-1);
			}

			if ( cfg.partitioner == "self" ) {
				// For now, do nothing.  Eventually we need to
				// have a checker for the partitioning.
				if(rank == 0) {
					sim_output->output("# SST will use a self-guided partition scheme.\n");
				}
			} else if ( cfg.partitioner == "simple" ) {
				if(cfg.verbose && rank == 0) {
					sim_output->output("# Performing a simple partition...\n");
				}

				simple_partition(graph, size);

				if(cfg.verbose && rank == 0) {
					sim_output->output("# Partitionning process is completed\n");
				}
			} else if ( cfg.partitioner == "rrobin" || cfg.partitioner == "roundrobin" ) {
				// perform a basic round robin partition
				if(cfg.verbose && rank == 0) {
					sim_output->output("# Performing a round-robin partition...\n");
				}

				rrobin_partition(graph, size);

				if(cfg.verbose && rank == 0) {
					sim_output->output("# Partitionning process is completed.\n");
				}
			} else if ( cfg.partitioner == "linear" ) {
				if(cfg.verbose && rank == 0) {
					sim_output->output("# Partitionning using a linear scheme...\n");
				}

				SSTLinearPartition* linear = new SSTLinearPartition(size, cfg.verbose);
				linear->performPartition(graph);
				delete linear;

				if(cfg.verbose && rank == 0) {
					sim_output->output("# Partitionning process is completed\n");
				}
			} else {
				if(rank == 0) {
					sim_output->output("# Partition scheme was not specified using: %s\n", cfg.partitioner.c_str());
				}

				partitionFunction func = sim->getFactory()->GetPartitioner(cfg.partitioner);
				func(graph,size);
			}

			double end_part = sst_get_cpu_time();

			if(cfg.verbose && (rank == 0)) {
				std::cout << "# Graph partitionning took " <<
					(end_part - start_part) << " seconds." << std::endl;
			}
		}
		else {
			graph = new ConfigGraph();
		}

		const uint64_t post_graph_create_rss = maxGlobalMemSize();

		if(cfg.verbose && (0 == rank) ){
			sim_output->output("# Graph construction and partition raised RSS by %" PRIu64 " KB\n",
                        	(post_graph_create_rss - pre_graph_create_rss));
			sim_output->output("# ------------------------------------------------------------\n");
		}

		// Delete the model generator
        	delete modelGen;
        	modelGen = NULL;

		// Output the partition information is user requests it
		dump_partition(sim_output, cfg, graph, rank, size);

		///////////////////////////////////////////////////////////////////////
		// Broadcast the data structures if only rank 0 built the
		// graph
        if ( size > 1 ) {
            broadcast(world, *graph, 0);
            broadcast(world, Params::keyMap, 0);
            broadcast(world, Params::keyMapReverse, 0);
            broadcast(world, Params::nextKeyID, 0);
            broadcast(world, cfg, 0);
		}

		// Perform the wireup
		do_graph_wireup(sim_output, graph, sim, cfg, size, rank);

		delete graph;
	}

    end_build = sst_get_cpu_time();
    double build_time = end_build - start;
    // std::cout << "#  Build time: " << build_time << " s" << std::endl;

    double max_build_time = build_time;
#ifdef HAVE_MPI
    all_reduce(world, &build_time, 1, &max_build_time, boost::mpi::maximum<double>() );
#endif

    start_run = sst_get_cpu_time();
    // double simulated_time = 0.0;
    // char simulated_time_prefix = ' ';
    UnitAlgebra simulated_time;

    signal(SIGUSR1, sigHandlerPrintStatus);
    signal(SIGUSR2, sigHandlerPrintStatus);
    // libpython appears to be replacing the signal handler for SIGINT.  Let's restore it to SIG_DFL
    signal(SIGINT, SIG_DFL);

    if ( cfg.runMode == Config::RUN || cfg.runMode == Config::BOTH ) {
        if ( cfg.archive ) {
            sim = archive.loadSimulation();
            printf("# Finished reading serialization file\n");
        }

        if ( cfg.verbose ) {
            printf("# Starting main event loop\n");

            time_t the_time = time(0);
            struct tm* now = localtime( &the_time );

            std::cout << "# Start time: " <<
                (now->tm_year + 1900) << "/" <<
                (now->tm_mon+1) << "/";

            if(now->tm_mday < 10) 
                std::cout << "0";

            std::cout << (now->tm_mday) << " at: ";

            if(now->tm_hour < 10) 
                std::cout << "0";

            std::cout << (now->tm_hour) << ":";

            if(now->tm_min < 10)
                std::cout << "0";

            std::cout << (now->tm_min)  << ":";

            if(now->tm_sec < 10) 
                std::cout << "0";

            std::cout << (now->tm_sec) << std::endl;
        }

        sim->setStopAtCycle(&cfg);

        sim->initialize();
        sim->run();

        simulated_time = sim->getFinalSimTime();
    }

    end_run = sst_get_cpu_time();

    double run_time = end_run - start_run;
    double total_time = end_run - start;

    double max_run_time, max_total_time;

#ifdef HAVE_MPI
    all_reduce(world, &run_time, 1, &max_run_time, boost::mpi::maximum<double>() );
    all_reduce(world, &total_time, 1, &max_total_time, boost::mpi::maximum<double>() );
#else
    max_run_time = run_time;
    max_total_time = total_time;
#endif

    const uint64_t local_max_rss  = maxLocalMemSize();
    const uint64_t global_max_rss = maxGlobalMemSize();
    const uint64_t local_max_pf   = maxLocalPageFaults();
    const uint64_t global_pf      = globalPageFaults();

    if ( rank == 0 && cfg.verbose ) {
	char ua_buffer[256];
	sprintf(ua_buffer, "%" PRIu64 "KB", local_max_rss);
	UnitAlgebra max_rss_ua(ua_buffer);

	sprintf(ua_buffer, "%" PRIu64 "KB", global_max_rss);
	UnitAlgebra global_rss_ua(ua_buffer);

	sim_output->output("\n");
	sim_output->output("#\n");
        sim_output->output("# ------------------------------------------------------------\n");
	sim_output->output("# Simulation Timing Information:\n");
	sim_output->output("# Build time:                      %f seconds\n", max_build_time);
	sim_output->output("# Simulation time:                 %f seconds\n", max_run_time);
	sim_output->output("# Total time:                      %f seconds\n", max_total_time);
	sim_output->output("# Simulated time:                  %s\n", simulated_time.toStringBestSI().c_str());
	sim_output->output("#\n");
	sim_output->output("# Simulation Resource Information:\n");
	sim_output->output("# Max Resident Set Size:           %s\n",
		max_rss_ua.toStringBestSI().c_str());
	sim_output->output("# Approx. Global Max RSS Size:     %s\n",
		global_rss_ua.toStringBestSI().c_str());
	sim_output->output("# Max Local Page Faults:           %" PRIu64 " faults\n",
		local_max_pf);
	sim_output->output("# Global Page Faults:              %" PRIu64 " faults\n",
		global_pf);
        sim_output->output("# ------------------------------------------------------------\n");
	sim_output->output("#\n");
	sim_output->output("\n");
    }

#ifdef HAVE_MPI
    if( 0 == rank ) {
#endif
    // Print out the simulation time regardless of verbosity.
        std::cout << "Simulation is complete, simulated time: " << simulated_time.toStringBestSI() << std::endl;
#ifdef HAVE_MPI
    }
#endif

    // Delete the simulation object
    delete sim;

#ifdef HAVE_MPI
    delete mpiEnv;
#endif

    return 0;
}

