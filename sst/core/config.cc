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

Config::Config( )
{
    archive     = false;
    archiveType = "bin";
    archiveFile = "sst_checkpoint";
    runMode     = BOTH;
    libpath     = SST_ELEMLIB_DIR;
    sdlfile     = "sdl.xml";
    stopAtCycle = "0 ns";
    timeBase    = "1 ps";
#ifdef HAVE_ZOLTAN
    partitioner = "zoltan";
#else
    partitioner = "single";
#endif
}


void Config::Print( )
{
    printf("archive %d\n",archive);
    printf("atype %s\n", archiveType.c_str());
    printf("archiveFile %s\n",archiveFile.c_str());
    printf("runMode %d\n",runMode);
    printf("libpath %s\n",libpath.c_str());
    printf("sdlfile %s\n",sdlfile.c_str());
    printf("stopAtCycle %s\n",stopAtCycle.c_str());
    printf("timeBase %s\n",timeBase.c_str());
    printf("partitioner %s\n",partitioner.c_str());
}

int Config::Init( int argc, char *argv[], int rank )
{
    // Some config items can be initialized from either the command line or
    // the config file. The command line has precedence. We need to initialize
    // the items found in the sdl file first. They will be overridden later.
    // We need to find the sdlfile amongst the command line arguments

    po::options_description helpDesc( "Allowed options:" );
    helpDesc.add_options()
        ("help", "print help message")
    ; 

    po::options_description hiddenDesc( "" );
    hiddenDesc.add_options()
        ("sdl-file", po::value< string >( &sdlfile ), 
                                "system description file")
    ; 

    po::positional_options_description posDesc;
    posDesc.add("sdl-file", 1);
    
    po::options_description desc( "" );
    desc.add_options()
        ("debug", po::value< vector<string> >()->multitoken(), 
                "{ all | cache | queue | archive | clock | sync | link |\
                 linkmap | linkc2m | linkc2c | linkc2s | comp | factory |\
                 stop | compevent | sim | clockevent | sdl | graph | zolt }")
        ("archive-type", po::value< string >(), 
                                "archive type [ xml | text | bin ]")
        ("archive-file", po::value< string >( &archiveFile ), 
                                "archive file name")
        ("lib-path", po::value< string >( &libpath ), 
                                "component library path")
        ("run-mode", po::value< string >(), 
                                "run mode [ init | run | both ]")
        ("stopAtCycle", po::value< string >(&stopAtCycle), 
	                        "how long should the simulation run")
        ("timeBase", po::value< string >(&timeBase), 
                                "the base time of the simulation")
#ifdef HAVE_ZOLTAN
        ("partitioner", po::value< string >(&partitioner), 
                                "partitioner to be used <single | zoltan | self>")
#else
        ("partitioner", po::value< string >(&partitioner), 
                                "partitioner to be used <single | self>")
#endif
	;
    
    po::options_description cmdline_options;
    cmdline_options.add(helpDesc).add(hiddenDesc).add(desc);

    po::variables_map vm;

    try {
	po::parsed_options parsed =
	    po::command_line_parser(argc,argv).options(cmdline_options).positional(posDesc).
	    run();
	po::store( parsed, vm );
	po::notify( vm );
    }
    catch (exception& e) {
	cout << "Error: " << e.what() << endl;
	return -1;
    }

    if ( vm.count("debug") ) {
        if ( DebugSetFlag( vm[ "debug" ].as< vector<string> >() ) ) {
            return -1;
        }
    }

    if ( vm.count( "help" ) ) {
	cout << "Usage: " << argv[0] << " sdl-file [options]" << endl;
        cout << helpDesc;
        cout << desc << endl;
        return 1;
    }

    if ( !vm.count( "sdl-file" ) ) {
	cout << "Error: no sdl-file specified" << endl;
	cout << "  Usage: " << argv[0] << " sdl-file [options]" << endl;
        return -1;
    }

    std::string xmlConfigStr;
    try {
	sdl_version = xmlGetVersion(sdlfile);
	xmlConfigStr = xmlGetConfig(sdlfile);
    } catch ( const char * e ) {
        printf("ERROR: invalid sdl file\n");
	printf("\t=> Attempted to load \"%s\"\n", sdlfile.c_str());
	printf("\t=> XML Parser said: %s\n", e);
	cout << "Usage: " << argv[0] << " sdl-file [options]" << endl;
        cout << helpDesc;
        cout << desc << "\n";
        return -1;
    }
    std::stringbuf sb( xmlConfigStr );
    std::istream ifs(&sb);

    // std::vector<string> the_rest = 
    //         po::collect_unrecognized( parsed.options, po::include_positional );
    
    try {
        // po::variables_map vm;
        // po::store( po::command_line_parser(the_rest).options(desc).run(), vm);
        po::store( po::parse_config_file( ifs, desc), vm);
        po::notify( vm );

        if ( vm.count( "archive-type" ) ) {
            archiveType = vm[ "archive-type" ].as< string >(); 
            archive = true;
        }
        if ( vm.count( "archive-file" ) ) {
            archive = true;
        }

        if ( vm.count("run-mode") ) {
            runMode = Config::RunMode( vm[ "run-mode" ].as< string >() ); 
            if ( runMode == Config::UNKNOWN ) {
                // this needs to be improved 
                printf("ERROR: Unknown run mode %s\n", 
                            vm[ "run-mode" ].as< string >().c_str());
		cout << "Usage: " << argv[0] << " sdl-file [options]" << endl;
                cout << helpDesc;
                cout << desc << "\n";
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

    #define BUF_LEN 100
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
    if ( sdlfile.compare(0,1,"/" ) ) {
        sdlfile = cwd + sdlfile;
    }

    // get the absolute path to the sdl file 
    if ( libpath.compare(0,1,"/" ) ) {
        libpath = cwd + libpath;
    }

    return 0;
}

} // namespace SST
