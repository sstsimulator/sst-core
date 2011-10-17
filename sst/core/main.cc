// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization/core.h"

#include <boost/mpi.hpp>
#include <boost/mpi/timer.hpp>

#include <signal.h>

#include <iomanip>

#include <sst/core/archive.h>
#include <sst/core/config.h>
#include <sst/core/element.h>
#include <sst/core/factory.h>
#include <sst/core/configGraph.h>
#include <sst/core/zolt.h>
#include <sst/core/simulation.h>
#include <sst/core/action.h>
#include <sst/core/activity.h>

using namespace std;
using namespace SST;

static void
sigHandlerPrintStatus1(int signal)
{
    Simulation::printStatus(false);
}

static void
sigHandlerPrintStatus2(int signal)
{
    Simulation::printStatus(true);
}

int 
main(int argc, char *argv[])
{
    boost::mpi::environment* mpiEnv = new boost::mpi::environment(argc,argv);
    boost::mpi::communicator world;

    Config cfg(world.rank());
    SST::Simulation*  sim= NULL;

    // All ranks parse the command line
    if ( cfg.parse_cmd_line(argc, argv) ) {
    // if ( cfg.Init(argc, argv, world.rank()) ) {
	delete mpiEnv;
        return -1;
    }

    // In fast build mode, everyone builds the entire graph structure
    // (saving the slow broadcast).  In non-fast, only rank 0 will
    // parse the sdl and build the graph.  It is then broadcast.  In
    // single rank mode, the option is ignored.
    sdl_parser* parser;
    if ( cfg.sdlfile != "NONE" ) {
	if ( cfg.all_parse || world.rank() == 0 ) {
	    // Create the sdl parser
	    parser = new sdl_parser(cfg.sdlfile);
	    
	    cfg.sdl_version = parser->getVersion();
	    
	    string config_string = parser->getSDLConfigString();
	    cfg.parse_config_file(config_string);
	    // cfg.print();

	}

	// If this is a parallel job, we need to broadcast the configuration
	if ( world.size() > 1 ) {
	    broadcast(world,cfg,0);
	}
    }
	
    
    Archive archive(cfg.archiveType, cfg.archiveFile);

    boost::mpi::timer* timer = new boost::mpi::timer();
    double start = timer->elapsed();
    double end_build, start_run, end_run;
            
    if ( cfg.verbose) printf("# main() My rank is %d, on %d nodes\n", world.rank(), world.size());
    DebugInit( world.rank(), world.size() );

    if ( cfg.runMode == Config::INIT || cfg.runMode == Config::BOTH ) { 
        sim = Simulation::createSimulation(&cfg, world.rank(), world.size());

        signal(SIGUSR1, sigHandlerPrintStatus1);
        signal(SIGUSR2, sigHandlerPrintStatus2);

	if ( cfg.sdl_version == "2.0" ) {
	    ConfigGraph* graph;

	    if ( world.size() == 1 ) {
		if ( cfg.generator != "NONE" ) {
		    generateFunction func = sim->getFactory()->GetGenerator(cfg.generator);
		    graph = new ConfigGraph();
		    func(graph,cfg.generator_options);
		}
		else {
		    graph = parser->createConfigGraph();
		}
		graph->setComponentRanks(0);
	    }
	    // Need to worry about partitioning for parallel jobs
	    else if ( world.rank() == 0 || cfg.all_parse ) {
		if ( cfg.generator != "NONE" ) {
		    generateFunction func = sim->getFactory()->GetGenerator(cfg.generator);
		    func(graph,cfg.generator_options);
		}
		else {
		    graph = parser->createConfigGraph();
		}

		// Do the partitioning.
		if ( cfg.partitioner != "self" ) {
		    graph->setComponentRanks(-1);
		}

		if ( cfg.partitioner == "self" ) {
		    // For now, do nothing.  Eventually we need to
		    // have a checker for the partitioning.
		}
		else if ( cfg.partitioner == "zoltan" ) {
		    printf("Zoltan support is currently not available, aborting...\n");
		    abort();
		}
		else {
		    partitionFunction func = sim->getFactory()->GetPartitioner(cfg.partitioner);
		    func(graph,world.size());
		    // graph->print_graph(cout);
		    // exit(1);
		}
	    }
	    else {
		graph = new ConfigGraph();
	    }
	    // Broadcast the data structures if only rank 0 built the
	    // graph
	    if ( !cfg.all_parse ) broadcast(world, *graph, 0);

	    sim->performWireUp( *graph, world.rank() );
	    delete graph;
	}
	else {
	    SDL_CompMap_t sdlMap;
	    xml_parse( cfg.sdlfile, sdlMap );
	    Graph graph(0);
	    bool single = false;

	    makeGraph(sim, sdlMap, graph);
	    if ( !strcmp(cfg.partitioner.c_str(),"zoltan") ) {
#ifdef HAVE_ZOLTAN
		partitionGraph( graph, argc, argv );
#else
 		printf("Requested zoltan partitioner, but SST was built without zoltan support, aborting...\n");
 		exit(1);
#endif
	    }
	    else if ( !strcmp(cfg.partitioner.c_str(),"single") ) {
		single = true;
	    }
	    else if ( !strcmp(cfg.partitioner.c_str(),"self") ) {
		// For now, do nothing, later we need to do some sanity checks
	    }
	    int minPart = findMinPart( graph );
	    sim->performWireUp( graph, sdlMap, minPart, world.rank(), single );
	}
        if (cfg.archive) {
            archive.SaveSimulation(sim);
            delete sim;
	    printf("# Finished writing serialization file\n");
        }
    }

    end_build = timer->elapsed();
    double build_time = end_build - start;
    // std::cout << "#  Build time: " << build_time << " s" << std::endl;

    double max_build_time;
    all_reduce(world, &build_time, 1, &max_build_time, boost::mpi::maximum<double>() );

    start_run = timer->elapsed();
    
    if ( cfg.runMode == Config::RUN || cfg.runMode == Config::BOTH ) { 
        if ( cfg.archive ) {
            sim = archive.LoadSimulation();
	    printf("# Finished reading serialization file\n");
        }
	
	if ( cfg.verbose ) printf("# Starting main event loop\n");
        sim->Run();


	delete sim;
    }

    end_run = timer->elapsed();

    double run_time = end_run - start_run;
    double total_time = end_run - start;

    double max_run_time, max_total_time;

    all_reduce(world, &run_time, 1, &max_run_time, boost::mpi::maximum<double>() );
    all_reduce(world, &total_time, 1, &max_total_time, boost::mpi::maximum<double>() );

    if ( world.rank() == 0 && cfg.verbose ) {
	std::cout << setiosflags(ios::fixed) << setprecision(2);
	std::cout << "#" << endl << "# Simulation times" << endl;
	std::cout << "#  Build time: " << max_build_time << " s" << std::endl;
	std::cout << "#  Simulation time: " << max_run_time << " s" << std::endl;
	std::cout << "#  Total time: " << max_total_time << " s" << std::endl;
    }

    delete mpiEnv;
    return 0;
}

