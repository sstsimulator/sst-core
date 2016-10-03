// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/config.h"
#include "sst/core/part/sstpart.h"

#include <errno.h>
#include <iostream>
#include <boost/program_options.hpp>

#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif

#include "sst/core/build_info.h"
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
    
Config::Config(RankInfo rankInfo)
{
    debugFile   = "/dev/null";
    runMode     = Simulation::BOTH;
    libpath     = SST_INSTALL_PREFIX"/lib/sst";
    addlLibPath = "";
    sdlfile     = "NONE";
    stopAtCycle = "0 ns";
    timeBase    = "1 ps";
    heartbeatPeriod = "N";
    partitioner = "linear";
    generator   = "NONE";
    generator_options   = "";
    dump_component_graph_file = "";
    output_directory = "";
#ifdef SST_CONFIG_HAVE_PYTHON
    model_options = "";
#endif
    verbose     = 0;
    world_size.rank = rankInfo.rank;
    world_size.thread = 1;
    no_env_config = false;
    enable_sig_handling = true;
    output_core_prefix = "@x SST Core: ";
    print_timing = false;

#ifdef __SST_DEBUG_EVENT_TRACKING__
    event_dump_file = "";
#endif
    
    // Some config items can be initialized from either the command line or
    // the config file. The command line has precedence. We need to initialize
    // the items found in the sdl file first. They will be overridden later.
    // We need to find the sdlfile amongst the command line arguments

    visNoConfigDesc = new po::options_description( "Allowed options" );
    visNoConfigDesc->add_options()
        ("help,h", "print help message")
        ("verbose,v", "print information about core runtimes")
        ("disable-signal-handlers", "disable SST automatic dynamic library environment configuration")
        ("no-env-config", "disable SST environment configuration")
        ("print-timing-info", "print SST timing information")
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
#ifdef SST_CONFIG_HAVE_MPI
        ("partitioner", po::value< string >(&partitioner),
         part_desc.c_str())
#endif

        ("generator", po::value< string >(&generator),
         "generator to be used to build simulation <lib.generator_name>")
        ("gen-options", po::value< string >(&generator_options),
         "options to be passed to generator function (must use quotes if whitespace is present)")
        ("output-config", po::value< string >(&output_config_graph),
         "Dump the SST component and link configuration graph to this file (as a Python file), empty string (default) is not to dump anything.")
        ("output-directory", po::value <string >(&output_directory),
         "Controls where SST will place output files including debug output and simulation statistics, default is for SST to create a unique directory.")
        ("output-dot", po::value <string >(&output_dot),
         "Dump the SST component and link graph to this file in DOT-format, empty string (default) is not to dump anything.")
        ("num_threads,n", po::value <uint32_t>(&world_size.thread),
         "Number of parallel threads to use per rank.")
#ifdef SST_CONFIG_HAVE_PYTHON
        ("model-options", po::value< string >(&model_options),
         "Provide options to the SST Python scripting engine (default is to provide no script options)")
#endif
        ("output-partition", po::value< string >(&dump_component_graph_file),
         "Dump the component partition to this file (default is not to dump information)")
        ("output-prefix-core", po::value< string >(&output_core_prefix),
         "Sets the SST::Output prefix for the core during execution")
#ifdef USE_MEMPOOL
        ("output-undeleted-events", po::value<string>(&event_dump_file),
         "Outputs information about all undeleted events to the specified file at end of simulation (STDOUT and STDERR can be used to output to console on stdout and stderr")
#endif
        ("output-xml", po::value< string >(&output_xml),
         "Dump the SST component and link configuration graph to this file (as an XML file), empty string (default) is not to dump anything.")
        ("output-json", po::value< string >(&output_json),
         "Dump the SST component and link configuration graph to this file (as an JSON file), empty string (default) is not to dump anything.")
        ;

    	var_map = new po::variables_map();
}

bool Config::printTimingInfo() {
	return print_timing;
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

    if ( var_map->count("debug-file") ) {
        Output::setFileName(debugFile);
    }
    else {
        Output::setFileName(tempFileName);
    }

    if ( var_map->count( "help" ) ) {
#ifdef SST_CONFIG_HAVE_MPI
	int this_rank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &this_rank);
	if(this_rank == 0) {
#endif
		cout << "Usage: " << run_name << " [options] config-file" << endl;
	        cout << *visNoConfigDesc;
	        cout << *mainDesc << endl;
#ifdef SST_CONFIG_HAVE_MPI
	}
#endif
        return 1;
    }

    verbose = var_map->count( "verbose" );
    enable_sig_handling = (var_map->count("disable-signal-handlers") > 0) ? false : true;
    print_timing = (var_map->count("print-timing-info") > 0);

    if ( var_map->count( "version" ) ) {
        if (SSTCORE_GIT_HEADSHA == PACKAGE_VERSION)
            cout << "SST-Core Release Version (" PACKAGE_VERSION << ")" << endl;
        else
            cout << "SST-Core Release Version (" PACKAGE_VERSION << ", Branch SHA: " SSTCORE_GIT_HEADSHA << ")" << endl;
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

    if ( 0 == world_size.thread ) {
        cerr << "ERROR: Must have at least 1 thread.\n";
        return -1;
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
            if ( runMode == Simulation::UNKNOWN ) {
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
