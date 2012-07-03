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

#include <errno.h>
#include <iostream>
#include <boost/program_options.hpp>

#include "sst/core/config.h"
#include "sst/core/debug.h"
#include "sst/core/sdl.h"
#include "sst/core/build_info.h"

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
    
Config::Config( int my_rank )
{
    rank        = my_rank;
    archive     = false;
    archiveType = "bin";
    archiveFile = "sst_checkpoint";
    runMode     = BOTH;
    libpath     = SST_ELEMLIB_DIR;
    sdlfile     = "NONE";
    stopAtCycle = "0 ns";
    timeBase    = "1 ps";
#ifdef HAVE_ZOLTAN
    partitioner = "zoltan";
#else
    partitioner = "single";
#endif
    generator   = "NONE";
    generator_options   = "";
    dump_component_graph_file = "";
    all_parse   = true;
    verbose     = false;
    
    // Some config items can be initialized from either the command line or
    // the config file. The command line has precedence. We need to initialize
    // the items found in the sdl file first. They will be overridden later.
    // We need to find the sdlfile amongst the command line arguments

    visNoConfigDesc = new po::options_description( "Allowed options" );
    visNoConfigDesc->add_options()
        ("help", "print help message")
        ("verbose", "print information about core runtimes")
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
    
    mainDesc = new po::options_description( "" );
    mainDesc->add_options()
        ("debug", po::value< vector<string> >()->multitoken(), 
                "{ all | cache | queue | archive | clock | sync | link |\
                 linkmap | linkc2m | linkc2c | linkc2s | comp | factory |\
                 stop | compevent | sim | clockevent | sdl | graph | zolt }")
        ("archive-type", po::value< string >( &archiveType ), 
                                "archive type [ xml | text | bin ]")
        ("archive-file", po::value< string >( &archiveFile ), 
                                "archive file name")
        ("lib-path", po::value< string >( &libpath ), 
                                "component library path")
        ("run-mode", po::value< string >(), 
                                "run mode [ init | run | both ]")
        ("stop-at", po::value< string >(&stopAtCycle), 
	                        "set time at which simulation will end execution")
        ("timebase", po::value< string >(&timeBase), 
                                "sets the base time step of the simulation (default: 1ps)")
        ("all-parse", po::value< bool >(&all_parse), 
                                "determine whether all ranks parse the sdl file, or only rank 0 [ true (default) | false ].  All-parse true is generally faster, but requires more memory.")
#ifdef HAVE_ZOLTAN
        ("partitioner", po::value< string >(&partitioner), 
	 "partitioner to be used <zoltan | self | lib.partitioner_name> (option ignored for serial jobs)" )
#else
        ("partitioner", po::value< string >(&partitioner), 
         "partitioner to be used <self | lib.partitioner_name> (option ignored for serial jobs)")
#endif
        ("generator", po::value< string >(&generator), 
         "generator to be used to build simulation <lib.generator_name>")
        ("gen-options", po::value< string >(&generator_options), 
         "options to be passed to generator function (must use quotes if whitespace is present)")
        ("dump_partition", po::value< string >(&dump_component_graph_file), 
         "dump component partition to this file (default is not to dump information)")
	
	;

    var_map = new po::variables_map();
}

int
Config::parse_cmd_line(int argc, char* argv[]) {
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

    if ( var_map->count( "help" ) ) {
	cout << "Usage: " << run_name << " [options] sdl-file" << endl;
        cout << *visNoConfigDesc;
        cout << *mainDesc << endl;
        return 1;
    }

    if ( var_map->count( "verbose" ) ) {
        verbose = true;
    }

    if ( sdlfile == "NONE" && generator == "NONE" ) {
	cout << "ERROR: no sdl-file and no generator specified" << endl;
	cout << "  Usage: " << run_name << " sdl-file [options]" << endl;
	return -1;
    }

    if ( var_map->count( "archive-type" ) ) {
	archiveType = (*var_map)[ "archive-type" ].as< string >(); 
	archive = true;
    }
    if ( var_map->count( "archive-file" ) ) {
	archive = true;
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
Config::parse_config_file(string config_string)
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

        if ( var_map->count( "archive-type" ) ) {
            archiveType = (*var_map)[ "archive-type" ].as< string >(); 
            archive = true;
        }
        if ( var_map->count( "archive-file" ) ) {
            archive = true;
        }

        if ( var_map->count("run-mode") ) {
            runMode = Config::RunMode( (*var_map)[ "run-mode" ].as< string >() ); 
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

    if ( runMode == RUN && ! archive ) {
        cerr << "ERROR: you specified \"--run-mode run\" without an archive";
        cerr << "\n";
        return -1;
    }

    #define BUF_LEN PATH_MAX
    char buf[BUF_LEN];
    if (getcwd( buf, BUF_LEN ) == NULL) {
	cerr << "could not find my cwd:" << strerror(errno) << endl;
	return -1;
    }
    std::string cwd = buf;

    cwd.append("/");

    // create a unique archive file name for each rank
    sprintf(buf,".%d",rank);
    archiveFile.append( buf );
    
    // get the absolute path to the sdl file 
    if ( libpath.compare(0,1,"/" ) ) {
        libpath = cwd + libpath;
    }

    return 0;
}
    
    
} // namespace SST
