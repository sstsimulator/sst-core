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

#include <signal.h>

#include <sst/core/archive.h>
#include <sst/core/config.h>
#include <sst/core/configGraph.h>
#include <sst/core/zolt.h>
#include <sst/core/simulation.h>
#include <sst/core/action.h>
#include <sst/core/activity.h>

using namespace std;
using namespace SST;

static void
sigHandlerPrintStatus(int signal)
{
    Simulation::printStatus();
}

int 
main(int argc, char *argv[])
{
    boost::mpi::environment* mpiEnv = new boost::mpi::environment(argc,argv);
    boost::mpi::communicator world;

    Config cfg;
    SST::Simulation*  sim;

    if ( cfg.Init( argc, argv, world.rank() ) ) {
        delete mpiEnv;
        return -1;
    }

    Archive archive(cfg.archiveType, cfg.archiveFile);

    printf("main() My rank is %d, on %d nodes\n", world.rank(), world.size());
    DebugInit( world.rank(), world.size() );

    if ( cfg.runMode == Config::INIT || cfg.runMode == Config::BOTH ) { 
        sim = Simulation::createSimulation(&cfg, world.rank(), world.size());

        signal(SIGUSR1, sigHandlerPrintStatus);

        SDL_CompMap_t sdlMap;
        xml_parse( cfg.sdlfile, sdlMap );
        Graph graph(0);

        makeGraph(sim, sdlMap, graph);
	if ( !strcmp(cfg.partitioner.c_str(),"zoltan") ) {
	    partitionGraph( graph, argc, argv );
	}
	else {
	    // For now, only option is self, so do nothing
	}
        int minPart = findMinPart( graph );
	sim->performWireUp( graph, sdlMap, minPart, world.rank() );
        sim->WireUp( graph, sdlMap, minPart, world.rank() );

        if (cfg.archive) {
            archive.SaveSimulation(sim);
            delete sim;
	    printf("Finished writing serialization file\n");
        }
    }

    if ( cfg.runMode == Config::RUN || cfg.runMode == Config::BOTH ) { 
        if ( cfg.archive ) {
            sim = archive.LoadSimulation();
	    printf("Finished reading serialization file\n");
        }
	
        sim->Run();
        delete sim;
    }

    delete mpiEnv;
    return 0;
}

