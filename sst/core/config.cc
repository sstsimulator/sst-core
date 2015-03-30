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
#include "sst/core/serialization.h"
#include "sst/core/config.h"
#include "sst/core/part/sstpart.h"

#include <errno.h>
#include <iostream>
#include <boost/program_options.hpp>

#include <mpi.h>

#include "sst/core/build_info.h"
#include "sst/core/debug.h"
#include "sst/core/output.h"
//#include "sst/core/sdl.h"

namespace po = boost::program_options;

using namespace std;

namespace SST {

Config::~Config() {
    delete visNoConfigDesc;
    delete hiddenNoConfigDesc;
    delete legacyDesc;
    delete mainDesc;
    delete posDesc;
    delete var_map;
}
    
Config::Config( int my_rank, int world_size )
{
    rank        = my_rank;
	numRanks    = world_size;
    debugFile   = "/dev/null";
    runMode     = BOTH;
    libpath     = SST_ELEMLIB_DIR;
    addlLibPath = "";
    sdlfile     = "NONE";
    stopAtCycle = "0 ns";
    timeBase    = "1 ps";
    heartbeatPeriod = "N";
#if HAVE_MPI
    partitioner = "linear";
#else
    partitioner = "single";
#endif
    generator   = "NONE";
    generator_options   = "";
    dump_component_graph_file = "";
    output_directory = "";
#ifdef HAVE_PYTHON
    model_options = "";
#endif
    verbose     = 0;
    no_env_config = false;
    
    // Some config items can be initialized from either the command line or
    // the config file. The command line has precedence. We need to initialize
    // the items found in the sdl file first. They will be overridden later.
    // We need to find the sdlfile amongst the command line arguments

    visNoConfigDesc = new po::options_description( "Allowed options" );
    visNoConfigDesc->add_options()
        ("help,h", "print help message")
        ("verbose,v", "print information about core runtimes")
	("no-env-config", "disable SST automatic dynamic library environment configuration")
        ("version,V", "print SST Release Version")
    ;

    hiddenNoConfigDesc = new po::options_description( "" );
    hiddenNoConfigDesc->add_options()
        ("sdl-file", po::value< string >( &sdlfile ), 
                                "system description file")
    ; 

    legacyDesc = new po::options_description( "Legacy options" );
    legacyDesc->add_options()
        ("stopAtCycle", po::value< string >(&stopAtCycle), 
	                        "how long should the simulation run")
        ("timeBase", po::value< string >(&timeBase), 
                                "the base time of the simulation")
    ; 

    posDesc = new po::positional_options_description();
    posDesc->add("sdl-file", 1);

    // Build the string for the partitioner help
    string part_desc;
    part_desc.append("partitioner to be used < ");
    map<string,string> desc = SST::Partition::SSTPartitioner::getDescriptionMap();
    string prefix = "";
    for ( map<string,string>::const_iterator it = desc.begin(); it != desc.end() ; ++it ) {
        part_desc.append(prefix).append(it->first);
        prefix = " | ";
    }
    part_desc.append(" | lib.partitioner_name >\nDescriptions:");
    for ( map<string,string>::const_iterator it = desc.begin(); it != desc.end() ; ++it ) {
        part_desc.append("\n-  ").append(it->first).append(": ").append(it->second);
    }
    part_desc.append("\n-  lib.partitioner_name: Partitioner found in element library 'lib' with name 'partitioner_name'");
    mainDesc = new po::options_description( "" );
    mainDesc->add_options()
        ("debug", po::value< vector<string> >()->multitoken(), 
#ifdef HAVE_ZOLTAN
                "{ all | cache | queue | clock | sync | link |\
                 linkmap | linkc2m | linkc2c | linkc2s | comp | factory |\
                 stop | compevent | sim | clockevent | sdl | graph | zolt }")
#else
                "{ all | cache | queue  | clock | sync | link |\
                 linkmap | linkc2m | linkc2c | linkc2s | comp | factory |\
                 stop | compevent | sim | clockevent | sdl | graph }")
#endif
        ("debug-file", po::value <string> ( &debugFile ),
                                "file where debug output will go")
        ("lib-path", po::value< string >( &libpath ),
                                "component library path (overwrites default)")
        ("add-lib-path", po::value< string >( &addlLibPath ),
                                "add additional component library paths")
        ("run-mode", po::value< string >(), 
                                "run mode [ init | run | both ]")
        ("stop-at", po::value< string >(&stopAtCycle), 
	                        "set time at which simulation will end execution")
        ("heartbeat-period", po::value< string >(&heartbeatPeriod),
				"Set time for heart beats to be published (these are approximate timings published by the core to update on progress), default is every 10000 simulated seconds")
        ("timebase", po::value< string >(&timeBase), 
                                "sets the base time step of the simulation (default: 1ps)")
#if HAVE_MPI
        ("partitioner", po::value< string >(&partitioner),
         part_desc.c_str())
#endif
        ("generator", po::value< string >(&generator),
         "generator to be used to build simulation <lib.generator_name>")
        ("gen-options", po::value< string >(&generator_options),
         "options to be passed to generator function (must use quotes if whitespace is present)")
        ("output-partition", po::value< string >(&dump_component_graph_file),
         "Dump the component partition to this file (default is not to dump information)")
        ("output-config", po::value< string >(&dump_config_graph),
	 "Dump the SST component and link configuration graph to this file (as a Python file), empty string (default) is not to dump anything.")
	("output-dot", po::value <string >(&output_dot),
	 "Dump the SST component and link graph to this file in DOT-format, empty string (default) is not to dump anything.")
	("output-directory", po::value <string >(&output_directory),
	 "Controls where SST will place output files including debug output and simulation statistics, default is for SST to create a unique directory.")
#ifdef HAVE_PYTHON
        ("model-options", po::value< string >(&model_options),
	 "Provide options to the SST Python scripting engine (default is to provide no script options)")
#endif

	;

    var_map = new po::variables_map();
}

uint32_t Config::getVerboseLevel() {
	return verbose;
}

void Config::setTimeBase(std::string timebaseStr) {
	timeBase = timebaseStr;
}

void Config::setStopAt(std::string stopAt) {
	stopAtCycle = stopAt;
}

int
Config::parseCmdLine(int argc, char* argv[]) {
    std::string tempFileName = "sst_output"; 

    run_name = argv[0];
    
    po::options_description cmdline_options;
    cmdline_options.add(*visNoConfigDesc).add(*hiddenNoConfigDesc).add(*mainDesc).add(*legacyDesc);

    try {
	po::parsed_options parsed =
	    po::command_line_parser(argc,argv).options(cmdline_options).positional(*posDesc).run();
	po::store( parsed, *var_map );
	po::notify( *var_map );
    }
    catch (exception& e) {
	cout << "Error: " << e.what() << endl;
	return -1;
    }

    if ( var_map->count("debug") ) {
        if ( DebugSetFlag( (*var_map)[ "debug" ].as< vector<string> >() ) ) {
            return -1;
        }
    }

    if ( var_map->count("debug-file") ) {
        Output::setFileName(debugFile);

        // Rename the debug file until the Output Class takes over
        tempFileName = debugFile + "_DBG"; 
        if ( DebugSetFile(tempFileName) ) {
            return -1;
        }
    }
    else {
        Output::setFileName(tempFileName);
    }

    if ( var_map->count( "help" ) ) {
#ifdef HAVE_MPI
	int this_rank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &this_rank);
	if(this_rank == 0) {
#endif
		cout << "Usage: " << run_name << " [options] config-file" << endl;
	        cout << *visNoConfigDesc;
	        cout << *mainDesc << endl;
#ifdef HAVE_MPI
	}
#endif
        return 1;
    }

	verbose = var_map->count( "verbose" );

    if ( var_map->count( "version" ) ) {
        cout << "SST Release Version (" PACKAGE_VERSION << ", " SST_SVN_REVISION ")" << endl;
        return 1;
    }

    if ( sdlfile == "NONE" && generator == "NONE" ) {
	cout << "ERROR: no sdl-file and no generator specified" << endl;
	cout << "  Usage: " << run_name << " sdl-file [options]" << endl;
	return -1;
    }


    // get the absolute path to the sdl file 
    if ( sdlfile != "NONE" ) {
	if ( sdlfile.compare(0,1,"/" ) ) {
	    #define BUF_LEN PATH_MAX
	    char buf[BUF_LEN];
	    if (getcwd( buf, BUF_LEN ) == NULL) {
	        cerr << "could not find my cwd:" << strerror(errno) << endl;
	        return -1;
	    }
	    std::string cwd = buf;
	    cwd.append("/");
	    sdlfile = cwd + sdlfile;
	}
    }

    return 0;
}

int
Config::parseConfigFile(string config_string)
{
    std::stringbuf sb( config_string );
    std::istream ifs(&sb);

    // std::vector<string> the_rest =
    //         po::collect_unrecognized( parsed.options, po::include_positional );

    try {
        // po::variables_map var_map;
        // po::store( po::command_line_parser(the_rest).options(desc).run(), var_map);
        po::options_description config_options;
        config_options.add(*mainDesc).add(*legacyDesc);

        po::store( po::parse_config_file( ifs, config_options), *var_map);
        po::notify( *var_map );

        if ( var_map->count("run-mode") ) {
            runMode = Config::setRunMode( (*var_map)[ "run-mode" ].as< string >() );
            if ( runMode == Config::UNKNOWN ) {
                // this needs to be improved 
                printf("ERROR: Unknown run mode %s\n", 
                        (*var_map)[ "run-mode" ].as< string >().c_str());
                cout << "Usage: " << run_name << " sdl-file [options]" << endl;
                cout << visNoConfigDesc;
                cout << mainDesc << "\n";
                return -1;
            }
        }
    } catch( exception& e ) {
        cerr << "error: " << e.what() << "\n";
        return -1;
    } catch(...) {
        cerr << "Exception of unknown type!\n";
        return -1;
    }

    // if ( runMode == RUN && ! archive ) {
    //     cerr << "ERROR: you specified \"--run-mode run\" without an archive";
    //     cerr << "\n";
    //     return -1;
    // }

#define BUF_LEN PATH_MAX
    char buf[BUF_LEN];
    if (getcwd( buf, BUF_LEN ) == NULL) {
        cerr << "could not find my cwd:" << strerror(errno) << endl;
        return -1;
    }
    std::string cwd = buf;

    cwd.append("/");

    return 0;
}
    
    
} // namespace SST
