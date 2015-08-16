// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#ifdef SST_CONFIG_HAVE_PYTHON
#include <Python.h>
#endif

#include "sst/core/serialization.h"


#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif

#include <iomanip>
#include <iostream>
#include <fstream>
#include <signal.h>

#include <sst/core/activity.h>
#include <sst/core/archive.h>
#include <sst/core/config.h>
#include <sst/core/configGraph.h>
#include <sst/core/factory.h>
#include <sst/core/simulation.h>
#include <sst/core/timeVortex.h>
#include <sst/core/part/sstpart.h>

#include <sst/core/cputimer.h>

#include <sst/core/model/sstmodel.h>
#include <sst/core/model/pymodel.h>
#include <sst/core/memuse.h>
#include <sst/core/iouse.h>

#include <sys/resource.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include <sst/core/objectComms.h>

// Configuration Graph Generation Options
#include <sst/core/configGraphOutput.h>
#include <sst/core/cfgoutput/pythonConfigOutput.h>
#include <sst/core/cfgoutput/dotConfigOutput.h>
#include <sst/core/cfgoutput/xmlConfigOutput.h>
#include <sst/core/cfgoutput/jsonConfigOutput.h>

using namespace SST::Core;
using namespace SST::Partition;
using namespace std;
using namespace SST;

static void
SimulationSigHandler(int signal)
{
    Simulation::setSignal(signal);
}

static void dump_partition(SST::Output* sim_output, Config& cfg, ConfigGraph* graph,
	const int rank, const int size) {
	
	///////////////////////////////////////////////////////////////////////	
	// If the user asks us to dump the partionned graph.
	if(cfg.dump_component_graph_file != "" && rank == 0) {
		if(cfg.verbose) {
			sim_output->verbose(CALL_INFO, 1, 0,
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
			sim_output->verbose(CALL_INFO, 1, 0,
				"# Dump of partition graph is complete.\n");
		}
	}
}

static void do_graph_wireup(SST::Output* sim_output, ConfigGraph* graph, 
                            SST::Simulation* sim, SST::Config& cfg, const int size, const int rank,
                            SimTime_t min_part) {

    if ( !graph->containsComponentInRank( rank ) ) {
        sim_output->output("WARNING: No components are assigned to rank: %d\n", 
                           rank);
    }

    std::vector<ConfigGraphOutput*> graphOutputs;

    // User asked us to dump the config graph to a file in Python
    if(cfg.output_config_graph != "") {
	graphOutputs.push_back( new PythonConfigGraphOutput(cfg.output_config_graph.c_str()) );
    }

    // user asked us to dump the config graph in dot graph format
    if(cfg.output_dot != "") {
	graphOutputs.push_back( new DotConfigGraphOutput(cfg.output_dot.c_str()) );
    }

    // User asked us to dump the config graph in XML format (for energy experiments)
    if(cfg.output_xml != "") {
	graphOutputs.push_back( new XMLConfigGraphOutput(cfg.output_xml.c_str()) );
    }

    // User asked us to dump the config graph in JSON format (for OCCAM experiments)
    if(cfg.output_json != "") {
	graphOutputs.push_back( new XMLConfigGraphOutput(cfg.output_json.c_str()) );
    }

    for(size_t i = 0; i < graphOutputs.size(); i++) {
	graphOutputs[i]->generate(&cfg, graph);
	delete graphOutputs[i];
    }

    sim->performWireUp( *graph, rank, min_part );
}


int
main(int argc, char *argv[])
{

#ifdef SST_CONFIG_HAVE_MPI
    // boost::mpi::environment* mpiEnv = new boost::mpi::environment(argc,argv);
    // boost::mpi::communicator world;
    MPI_Init(&argc, &argv);
    
    int myrank = 0;
    int mysize = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &mysize);

    const int rank = myrank;
    const int size = mysize;
#else
    const int rank = 0;
    const int size = 1;
#endif
    Config cfg(rank, size);

    Output* sim_output = NULL;
    SST::Simulation* sim = NULL;

    // All ranks parse the command line
    if ( cfg.parseCmdLine(argc, argv) ) {
#ifdef SST_CONFIG_HAVE_MPI
        // delete mpiEnv;
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

    double start = sst_get_cpu_time();
    double end_build, start_run, end_run;

	if ( cfg.runMode == Config::INIT || cfg.runMode == Config::BOTH ) {
		sim = Simulation::createSimulation(&cfg, rank, size);
		sim_output = &(Simulation::getSimulation()->getSimulationOutput());

		sim_output->verbose(CALL_INFO, 1, 0, "SST rank %d is now active on %d total ranks.\n", rank, size);

		// Get the memory before we create the graph
		const uint64_t pre_graph_create_rss = maxGlobalMemSize();

        
        ////// Start ConfigGraph Creation //////
		ConfigGraph* graph = NULL;

        double start_graph_gen = sst_get_cpu_time();
        graph = new ConfigGraph();
        
        // Only rank 0 will populate the graph
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

		// Delete the model generator
        delete modelGen;
        modelGen = NULL;
        
        double end_graph_gen = sst_get_cpu_time();
        
        if ( cfg.verbose && (rank == 0) ) {
            sim_output->verbose(CALL_INFO, 1, 0, "------------------------------------------------------------\n");
            sim_output->verbose(CALL_INFO, 1, 0, "Graph construction took %f seconds.\n",
                               (end_graph_gen - start_graph_gen));
        }
        ////// End ConfigGraph Creation //////
        
        ////// Start Partitioning //////
        double start_part = sst_get_cpu_time();
        
        // If this is a serial job, just use the single partitioner,
        // but the same code path
        if ( size == 1 ) cfg.partitioner = "single";
        SSTPartitioner* partitioner = SSTPartitioner::getPartitioner(cfg.partitioner, size, rank, cfg.verbose);
        if ( partitioner == NULL ) {
            // Not a built in partitioner, see if this is a
            // partitioner contained in an element library.
            partitionFunction func = sim->getFactory()->GetPartitioner(cfg.partitioner);
            partitioner = func(size, rank, cfg.verbose);
        }
        
        if ( partitioner->requiresConfigGraph() ) {
            partitioner->performPartition(graph);
        }
        else {
            PartitionGraph* pgraph;
            if ( rank == 0 ) {
                pgraph = graph->getCollapsedPartitionGraph();
            }
            else {
                pgraph = new PartitionGraph();
            }
            
            if ( rank == 0 || partitioner->spawnOnAllRanks() ) {
                partitioner->performPartition(pgraph);
                
                if ( rank == 0 ) graph->annotateRanks(pgraph);
            }
            
            delete pgraph;
        }
        
        delete partitioner;

        // Check the partitioning to make sure it is sane
        if ( rank == 0 ) {
            if ( !graph->checkRanks( size ) ) {
				sim_output->fatal(CALL_INFO, 1,
                                  "ERROR: Bad partitionning; partition included unknown ranks.\n");
			}
		}
        double end_part = sst_get_cpu_time();

	if(0 == rank) {
		sim_output->verbose(CALL_INFO, 1, 0, "Graph partition took %f seconds\n",
			(end_part - start_part));
     	}
        ////// End Partitioning //////

		const uint64_t post_graph_create_rss = maxGlobalMemSize();

		if(0 == rank){
			sim_output->verbose(CALL_INFO, 1, 0, "Graph construction and partition raised RSS by %" PRIu64 " KB\n",
                        	(post_graph_create_rss - pre_graph_create_rss));
			sim_output->verbose(CALL_INFO, 1, 0, "------------------------------------------------------------\n");
		}

        // Output the partition information is user requests it
        dump_partition(sim_output, cfg, graph, rank, size);

        SimTime_t min_part = 0xffffffffffffffffl;
        if ( size > 1 ) {
            // Check the graph for the minimum latency crossing a partition boundary
            if ( rank == 0 ) {
                ConfigComponentMap_t comps = graph->getComponentMap();
                ConfigLinkMap_t links = graph->getLinkMap();
                // Find the minimum latency across a partition
                for( ConfigLinkMap_t::iterator iter = links.begin();
                     iter != links.end(); ++iter ) {
                    ConfigLink &clink = *iter;
                    int rank[2];
                    rank[0] = comps[clink.component[0]].rank;
                    rank[1] = comps[clink.component[1]].rank;
                    if ( rank[0] == rank[1] ) continue;
                    if ( clink.getMinLatency() < min_part ) {
                        min_part = clink.getMinLatency();
                    }
                }
            }
#ifdef SST_CONFIG_HAVE_MPI

            // broadcast(world, min_part, 0);
            Comms::broadcast(min_part, 0);
#endif
        }
#if 0
#ifdef SST_CONFIG_HAVE_MPI
            ///////////////////////////////////////////////////////////////////////
            // Broadcast the data structures if only rank 0 built the
            // graph
        if ( size > 1 ) {
            //            broadcast(world, *graph, 0);
            // broadcast(world, Params::keyMap, 0);
            // broadcast(world, Params::keyMapReverse, 0);
            // broadcast(world, Params::nextKeyID, 0);
            // broadcast(world, cfg, 0);
            broadcast(*graph, 0);
            broadcast(Params::keyMap, 0);
            broadcast(Params::keyMapReverse, 0);
            broadcast(Params::nextKeyID, 0);
            broadcast(cfg, 0);
		}
#endif
#endif

#if 0
        
        if ( size > 1 ) {
#ifdef SST_CONFIG_HAVE_MPI
			// broadcast(world, Params::keyMap, 0);
			// broadcast(world, Params::keyMapReverse, 0);
			// broadcast(world, Params::nextKeyID, 0);
            // broadcast(world, cfg, 0);
			broadcast(Params::keyMap, 0);
			broadcast(Params::keyMapReverse, 0);
			broadcast(Params::nextKeyID, 0);
            broadcast(cfg, 0);

            ConfigGraph* sub;
            if ( rank == 0 ) {
                for ( int i = 0; i < size; i++ ) {
                    // cout << "Create subgraph" << endl;
                    sub = graph->getSubGraph(i,i);
                    // cout << "done" << endl;
                    // world.send(i, 0, *sub);
                    send(i, 0, *sub);
                    // cout << "here 1" << endl;
                    delete sub;
                    // cout << "here 2" << endl;
                }
            }
            else {
                // world.recv(0, 0, *graph);
                recv(0, 0, *graph);
            }
            // world.barrier();
            MPI_Barrier(MPI_COMM_WORLD);
#endif
		}
#endif
        

        
#if 1        
#ifdef SST_CONFIG_HAVE_MPI
		if ( size > 1 ) {
            
            // broadcast(world, Params::keyMap, 0);
            // broadcast(world, Params::keyMapReverse, 0);
            // broadcast(world, Params::nextKeyID, 0);
            // broadcast(world, cfg, 0);
            Comms::broadcast(Params::keyMap, 0);
            Comms::broadcast(Params::keyMapReverse, 0);
            Comms::broadcast(Params::nextKeyID, 0);
            Comms::broadcast(cfg, 0);

            std::set<int> my_ranks;
            std::set<int> your_ranks;

            // boost::mpi::communicator comm;
            // std::vector<boost::mpi::request> pending_requests;

            if ( 0 == rank ) {
                // Split the rank space in half
                for ( int i = 0; i < size/2; i++ ) {
                    my_ranks.insert(i);
                }
                
                for ( int i = size/2; i < size; i++ ) {
                    your_ranks.insert(i);
                }

                // Need to send the your_ranks set and the proper
                // subgraph for further distribution                
                // double start = sst_get_cpu_time();
                ConfigGraph* your_graph = graph->getSubGraph(your_ranks);
                // double end = sst_get_cpu_time();
                // cout << (end-start) << " seconds" << endl;
                int dest = *your_ranks.begin();
                // pending_requests.push_back(world.isend(dest, 0, your_ranks));
                // pending_requests.push_back(world.isend(dest, 0, *your_graph));
                // boost::mpi::wait_all(pending_requests.begin(), pending_requests.end());
                // pending_requests.clear();
                Comms::send(dest, 0, your_ranks);
                Comms::send(dest, 0, *your_graph);
                your_ranks.clear();
                // delete your_graph;
            }
            else {
                // world.recv(boost::mpi::any_source, 0, my_ranks);
                // world.recv(boost::mpi::any_source, 0, *graph);
                Comms::recv(MPI_ANY_SOURCE, 0, my_ranks);
                Comms::recv(MPI_ANY_SOURCE, 0, *graph);
            }

            while ( my_ranks.size() != 1 ) {
                // This means I have more data to pass on to other ranks
                std::set<int>::iterator mid = my_ranks.begin();
                for ( unsigned int i = 0; i < my_ranks.size() / 2; i++ ) {
                    ++mid;
                }
                your_ranks.insert(mid,my_ranks.end());
                my_ranks.erase(mid,my_ranks.end());

                // double start = sst_get_cpu_time();
                ConfigGraph* your_graph = graph->getSubGraph(your_ranks);
                // double end = sst_get_cpu_time();
                // cout << (end-start) << " seconds" << endl;
                int dest = *your_ranks.begin();

                // pending_requests.push_back(world.isend(dest, 0, your_ranks));
                // pending_requests.push_back(world.isend(dest, 0, *your_graph));
                // boost::mpi::wait_all(pending_requests.begin(), pending_requests.end());
                // pending_requests.clear();
                Comms::send(dest, 0, your_ranks);
                Comms::send(dest, 0, *your_graph);
                your_ranks.clear();
                delete your_graph;
            }

	    if( *my_ranks.begin() != rank) {
		sim_output->fatal(CALL_INFO, -1, "Error in distributing configuration graph\n");
	    }
        }
#endif
#endif
        
		// Perform the wireup
		do_graph_wireup(sim_output, graph, sim, cfg, size, rank, min_part);

		delete graph;
	}
    
    end_build = sst_get_cpu_time();
    double build_time = end_build - start;
    // std::cout << "#  Build time: " << build_time << " s" << std::endl;

    double max_build_time = build_time;
#ifdef SST_CONFIG_HAVE_MPI
    // all_reduce(world, &build_time, 1, &max_build_time, boost::mpi::maximum<double>() );
    MPI_Allreduce( &build_time, &max_build_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );
#endif

    start_run = sst_get_cpu_time();
    // double simulated_time = 0.0;
    // char simulated_time_prefix = ' ';
    UnitAlgebra simulated_time;

    if(cfg.enable_sig_handling) {
	sim_output->verbose(CALL_INFO, 1, 0, "Signal handers will be registed for USR1, USR2, INT and TERM...\n");
	if(SIG_ERR == signal(SIGUSR1, SimulationSigHandler)) {
		sim_output->fatal(CALL_INFO, -1, "Installation of SIGUSR1 signal handler failed.\n");
	}

	if(SIG_ERR == signal(SIGUSR2, SimulationSigHandler)) {
		sim_output->fatal(CALL_INFO, -1, "Installation of SIGUSR2 signal handler failed\n");
	}

	if(SIG_ERR == signal(SIGINT, SimulationSigHandler)) {
		sim_output->fatal(CALL_INFO, -1, "Installation of SIGINT signal handler failed\n");
	}

	if(SIG_ERR == signal(SIGTERM, SimulationSigHandler)) {
		sim_output->fatal(CALL_INFO, -1, "Installation of SIGTERM signal handler failed\n");
	}

	sim_output->verbose(CALL_INFO, 1, 0, "Signal handler registration is completed\n");
    } else {
	// Print out to say disabled?
	sim_output->verbose(CALL_INFO, 1, 0, "Signal handlers are disabled by user input\n");
    }

    if ( cfg.runMode == Config::RUN || cfg.runMode == Config::BOTH ) {
        if ( cfg.verbose ) {
	    sim_output->verbose(CALL_INFO, 1, 0, "Starting main event loop...\n");

            time_t the_time = time(0);
            struct tm* now = localtime( &the_time );

	    char* date_time_buffer = (char*) malloc(sizeof(char) * 256);
	    sprintf(date_time_buffer, "%4d/%02d/%02d at %02d:%02d:%02d",
		(now->tm_year + 1900), (now->tm_mon+1), now->tm_mday,
		now->tm_hour, now->tm_min, now->tm_sec);
	    sim_output->verbose(CALL_INFO, 1, 0, "Start time: %s\n", date_time_buffer);
            free(date_time_buffer);
        }

        sim->setStopAtCycle(&cfg);

        // If we are a parallel job, need to makes sure that all used
        // libraries are loaded on all ranks.
#ifdef SST_CONFIG_HAVE_MPI
        if ( size > 1 ) {
            set<string> lib_names;
            set<string> other_lib_names;
            Simulation::getSimulation()->getFactory()->getLoadedLibraryNames(lib_names);
            // vector<set<string> > all_lib_names;

            // Send my lib_names to the next lowest rank
            // gather(world, lib_names, all_lib_names, 0);
            if ( rank == size - 1 ) {
                Comms::send(rank - 1, 0, lib_names);
                lib_names.clear();
            }
            else {
                Comms::recv(rank + 1, 0, other_lib_names);
                for ( set<string>::const_iterator iter = other_lib_names.begin();
                      iter != other_lib_names.end(); ++iter ) {
                    lib_names.insert(*iter);
                }
                if ( rank != 0 ) {
                    Comms::send(rank - 1, 0, lib_names);
                    lib_names.clear();
                }
            }

            // broadcast(world, lib_names, 0);
            Comms::broadcast(lib_names, 0);
            Simulation::getSimulation()->getFactory()->loadUnloadedLibraries(lib_names);
        }
#endif
        sim->initialize();
        SST::Interfaces::SimpleNetwork::exchangeMappingData();
        // std::cout << "Done merging network maps" << std::endl;
        sim->run();

        simulated_time = sim->getFinalSimTime();
    }

    end_run = sst_get_cpu_time();

    double run_time = end_run - start_run;
    double total_time = end_run - start;
    
    double max_run_time, max_total_time;
    
    uint64_t local_max_tv_depth = Simulation::getSimulation()->getTimeVortexMaxDepth();
    uint64_t global_max_tv_depth;
    uint64_t local_current_tv_depth = Simulation::getSimulation()->getTimeVortexCurrentDepth();
    uint64_t global_current_tv_depth;
    
    uint64_t local_sync_data_size = Simulation::getSimulation()->getSyncQueueDataSize();
    uint64_t global_max_sync_data_size, global_sync_data_size;

    uint64_t mempool_size;
    uint64_t active_activities;
    Activity::getMemPoolUsage(mempool_size, active_activities);
    uint64_t max_mempool_size, global_mempool_size, global_active_activities;
    
#ifdef SST_CONFIG_HAVE_MPI
    MPI_Allreduce(&run_time, &max_run_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );
    MPI_Allreduce(&total_time, &max_total_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );
    MPI_Allreduce(&local_max_tv_depth, &global_max_tv_depth, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
    MPI_Allreduce(&local_current_tv_depth, &global_current_tv_depth, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
    MPI_Allreduce(&local_sync_data_size, &global_max_sync_data_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
    MPI_Allreduce(&local_sync_data_size, &global_sync_data_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
    MPI_Allreduce(&mempool_size, &max_mempool_size, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
    MPI_Allreduce(&mempool_size, &global_mempool_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
    MPI_Allreduce(&active_activities, &global_active_activities, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
#else
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
    
    if ( rank == 0 ) {
	    char ua_buffer[256];
        sprintf(ua_buffer, "%" PRIu64 "KB", local_max_rss);
        UnitAlgebra max_rss_ua(ua_buffer);
        
        sprintf(ua_buffer, "%" PRIu64 "KB", global_max_rss);
        UnitAlgebra global_rss_ua(ua_buffer);

        sprintf(ua_buffer, "%" PRIu64 "B", global_max_sync_data_size);
        UnitAlgebra global_max_sync_data_size_ua(ua_buffer);

        sprintf(ua_buffer, "%" PRIu64 "B", global_sync_data_size);
        UnitAlgebra global_sync_data_size_ua(ua_buffer);

        sprintf(ua_buffer, "%" PRIu64 "B", max_mempool_size);
        UnitAlgebra max_mempool_size_ua(ua_buffer);

        sprintf(ua_buffer, "%" PRIu64 "B", global_mempool_size);
        UnitAlgebra global_mempool_size_ua(ua_buffer);
        
        sim_output->verbose(CALL_INFO, 1, 0, "\n");
        sim_output->verbose(CALL_INFO, 1, 0, "\n");
        sim_output->verbose(CALL_INFO, 1, 0, "------------------------------------------------------------\n");
        sim_output->verbose(CALL_INFO, 1, 0, "Simulation Timing Information:\n");
        sim_output->verbose(CALL_INFO, 1, 0, "Build time:                      %f seconds\n", max_build_time);
        sim_output->verbose(CALL_INFO, 1, 0, "Simulation time:                 %f seconds\n", max_run_time);
        sim_output->verbose(CALL_INFO, 1, 0, "Total time:                      %f seconds\n", max_total_time);
        sim_output->verbose(CALL_INFO, 1, 0, "Simulated time:                  %s\n", simulated_time.toStringBestSI().c_str());
        sim_output->verbose(CALL_INFO, 1, 0, "\n");
        sim_output->verbose(CALL_INFO, 1, 0, "Simulation Resource Information:\n");
        sim_output->verbose(CALL_INFO, 1, 0, "Max Resident Set Size:           %s\n",
                           max_rss_ua.toStringBestSI().c_str());
        sim_output->verbose(CALL_INFO, 1, 0, "Approx. Global Max RSS Size:     %s\n",
                           global_rss_ua.toStringBestSI().c_str());
        sim_output->verbose(CALL_INFO, 1, 0, "Max Local Page Faults:           %" PRIu64 " faults\n",
                           local_max_pf);
        sim_output->verbose(CALL_INFO, 1, 0, "Global Page Faults:              %" PRIu64 " faults\n",
                           global_pf);
        sim_output->verbose(CALL_INFO, 1, 0, "Max Output Blocks:               %" PRIu64 " blocks\n",
                           global_max_io_in);
        sim_output->verbose(CALL_INFO, 1, 0, "Max Input Blocks:                %" PRIu64 " blocks\n",
                           global_max_io_out);
        sim_output->verbose(CALL_INFO, 1, 0, "Max mempool usage:               %s\n",
                           max_mempool_size_ua.toStringBestSI().c_str());
        sim_output->verbose(CALL_INFO, 1, 0, "Global mempool usage:            %s\n",
                           global_mempool_size_ua.toStringBestSI().c_str());
        sim_output->verbose(CALL_INFO, 1, 0, "Global active activities:        %" PRIu64 " activities\n",
                           global_active_activities);
        sim_output->verbose(CALL_INFO, 1, 0, "Current global TimeVortex depth: %" PRIu64 " entries\n",
                           global_current_tv_depth);
        sim_output->verbose(CALL_INFO, 1, 0, "Max TimeVortex depth:            %" PRIu64 " entries\n",
                           global_max_tv_depth);
        sim_output->verbose(CALL_INFO, 1, 0, "Max Sync data size:              %s\n",
                           global_max_sync_data_size_ua.toStringBestSI().c_str());
        sim_output->verbose(CALL_INFO, 1, 0, "Global Sync data size:           %s\n",
                           global_sync_data_size_ua.toStringBestSI().c_str());
        sim_output->verbose(CALL_INFO, 1, 0, "------------------------------------------------------------\n");
        sim_output->verbose(CALL_INFO, 1, 0, "\n");
        sim_output->output("\n");

    }

#ifdef USE_MEMPOOL
    if ( cfg.event_dump_file != ""  ) {
        Output out("",0,0,Output::FILE, cfg.event_dump_file);
        if ( cfg.event_dump_file == "STDOUT" || cfg.event_dump_file == "stdout" ) out.setOutputLocation(Output::STDOUT);
        if ( cfg.event_dump_file == "STDERR" || cfg.event_dump_file == "stderr" ) out.setOutputLocation(Output::STDERR);
        // Activity::printUndeletedActivites("",*sim_output, sim->getEndSimCycle());
        Activity::printUndeletedActivites("",out, MAX_SIMTIME_T);
    }
#endif
    
#ifdef SST_CONFIG_HAVE_MPI
    if( 0 == rank ) {
#endif
    // Print out the simulation time regardless of verbosity.
        sim_output->output("Simulation is complete, simulated time: %s\n", simulated_time.toStringBestSI().c_str());
#ifdef SST_CONFIG_HAVE_MPI
    }
#endif

    // Delete the simulation object
    delete sim;

#ifdef SST_CONFIG_HAVE_MPI
    // delete mpiEnv;
    MPI_Finalize();
#endif

    return 0;
}

