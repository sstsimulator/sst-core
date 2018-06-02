// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/config.h"
#include "sst/core/part/sstpart.h"

#include <errno.h>
#ifndef E_OK
#define E_OK 0
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <iostream>
#include <cstdlib>

#include <sst/core/warnmacros.h>
#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include "sst/core/build_info.h"
#include "sst/core/output.h"
//#include "sst/core/sdl.h"

using namespace std;

namespace SST {

Config::~Config() {
}

Config::Config(RankInfo rankInfo)
{
    debugFile   = "/dev/null";
    runMode     = Simulation::BOTH;
    libpath     = SST_INSTALL_PREFIX "/lib/sst";
    addlLibPath = "";
    configFile     = "NONE";
    stopAtCycle = "0 ns";
    stopAfterSec = 0;
    timeBase    = "1 ps";
    heartbeatPeriod = "N";
    partitioner = "sst.linear";
    generator   = "NONE";
    generator_options   = "";
    timeVortex  = "sst.timevortex.priority_queue";
    dump_component_graph_file = "";

    char* wd_buf = (char*) malloc( sizeof(char) * PATH_MAX );
    getcwd(wd_buf, PATH_MAX);

    output_directory = "";
    if( NULL != wd_buf ) {
	output_directory.append(wd_buf);
        free(wd_buf);
    }

    model_options = "";
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
    // We need to find the configFile amongst the command line arguments

}

struct sstLongOpts_s {
    struct option opt;
    const char *argName;
    const char *desc;
    Config::flagFunction flagFunc;
    Config::argFunction  argFunc;
};


#define DEF_FLAGOPTVAL(longName, shortName, text, func, valFunc) \
    {{longName, no_argument, 0, shortName}, NULL, text, func, valFunc}

#define DEF_FLAGOPT(longName, shortName, text, func) \
    DEF_FLAGOPTVAL(longName, shortName, text, func, NULL)


#define DEF_ARGOPT_SHORT(longName, shortName, argName, text, func) \
    {{longName, required_argument, 0, shortName}, argName, text, NULL, func}

#define DEF_ARGOPT(longName, argName, text, func) \
    DEF_ARGOPT_SHORT(longName, 0, argName, text, func)

static const struct sstLongOpts_s sstOptions[] = {
    /* visNoConfigDesc */
    DEF_FLAGOPT("help",                     'h',    "print help message", &Config::usage),
    DEF_FLAGOPTVAL("verbose",                  'v',    "print information about core runtime", &Config::incrVerbose, &Config::setVerbosity),
    DEF_FLAGOPT("version",                  'V',    "print SST Release Version", &Config::printVersion),
    DEF_FLAGOPT("disable-signal-handlers",  0,      "disable SST automatic dynamic library environment configuration", &Config::disableSigHandlers),
    DEF_FLAGOPT("no-env-config",            0,      "disable SST environment configuration", &Config::disableEnvConfig),
    DEF_FLAGOPT("print-timing-info",        0,      "print SST timing information", &Config::enablePrintTiming),
    /* HiddenNoConfigDesc */
    DEF_ARGOPT("sdl-file",          "FILE",         "SST Configuration file", &Config::setConfigFile),
    DEF_ARGOPT("stopAtCycle",       "TIME",         "set time at which simulation will end execution", &Config::setStopAt),
    DEF_ARGOPT("stopAfter",         "TIME",         "set maximum wall time after which simulation will end execution", &Config::setStopAfter),
    /* MainDesc */
    DEF_ARGOPT("debug-file",        "FILE",         "file where debug output will go", &Config::setDebugFile),
    DEF_ARGOPT("lib-path",          "LIBPATH",      "component library path (overwrites default)", &Config::setLibPath),
    DEF_ARGOPT("add-lib-path",      "LIBPATH",      "component library path (appends to main path)", &Config::addLibPath),
    DEF_ARGOPT("run-mode",          "MODE",         "run mode [ init | run | both]", &Config::setRunMode),
    DEF_ARGOPT("stop-at",           "TIME",         "set time at which simulation will end execution", &Config::setStopAt),
    DEF_ARGOPT("heartbeat-period",  "PERIOD",       "set time for heartbeats to be published (these are approximate timings, published by the core, to update on progress), default is every 10000 simulated seconds", &Config::setHeartbeat),
    DEF_ARGOPT("timebase",          "TIMEBASE",     "sets the base time step of the simulation (default: 1ps)", &Config::setTimebase),
    DEF_ARGOPT("partitioner",       "PARTITIONER",  "select the partitioner to be used. <lib.partitionerName>", &Config::setPartitioner),
    DEF_ARGOPT("generator",         "GENERATOR",    "select the generator to be used to build simulation <lib.generatorName>", &Config::setGenerator),
    DEF_ARGOPT("gen-options",       "OPTSTIRNG",    "options to be passed to generator function", &Config::setGeneratorOptions),
    DEF_ARGOPT("timeVortex ",       "MODULE",       "select TimeVortex implementation <lib.timevortex>", &Config::setTimeVortex),
    DEF_ARGOPT("output-directory",  "DIR",          "directory into which all SST output files should reside", &Config::setOutputDir),
    DEF_ARGOPT("output-config",     "FILE",         "file to write SST configuration (in Python format)", &Config::setWriteConfig),
    DEF_ARGOPT("output-dot",        "FILE",         "file to write SST configuration graph (in GraphViz format)", &Config::setWriteDot),
    DEF_ARGOPT("output-xml",        "FILE",         "file to write SST configuration graph (in XML format)", &Config::setWriteXML),
    DEF_ARGOPT("output-json",       "FILE",         "file to write SST configuration graph (in JSON format)", &Config::setWriteJSON),
    DEF_ARGOPT("output-partition",  "FILE",         "file to write SST component partitioning information", &Config::setWritePartition),
    DEF_ARGOPT("output-prefix-core","STR",          "set the SST::Output prefix for the core", &Config::setOutputPrefix),
#ifdef USE_MEMPOOL
    DEF_ARGOPT("output-undeleted-events",   "FILE", "file to write information about all undeleted events at the end of simulation (STDOUT and STDERR can be used to output to console)", &Config::setWriteUndeleted),
#endif
    DEF_ARGOPT("model-options",     "STR",          "provide options to the python configuration script", &Config::setModelOptions),
    DEF_ARGOPT_SHORT("num_threads", 'n',   "NUM",   "number of parallel threads to use per rank", &Config::setNumThreads),
    {{NULL, 0, 0, 0}, NULL, NULL, NULL, NULL}
};
static const size_t nLongOpts = (sizeof(sstOptions) / sizeof(sstLongOpts_s)) -1;


bool Config::usage() {
#ifdef SST_CONFIG_HAVE_MPI
	int this_rank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &this_rank);
	if(this_rank != 0)  return true;
#endif

    /* Determine screen / description widths */
    uint32_t MAX_WIDTH = 80;

    struct winsize size;
    if ( ioctl(STDERR_FILENO,TIOCGWINSZ,&size) == 0 ) {
        MAX_WIDTH = size.ws_col;
    }

    if ( getenv("COLUMNS") ) {
        errno = E_OK;
        uint32_t x = strtoul(getenv("COLUMNS"), 0, 0);
        if ( errno == E_OK ) MAX_WIDTH = x;
    }

    const uint32_t desc_start = 32;
    const uint32_t desc_width = MAX_WIDTH - desc_start;


    /* Print usage */
    fprintf(stderr,
            "Usage: sst [options] config-file\n"
            "\n");
    for ( size_t i = 0 ; i < nLongOpts ; i++ ) {
        uint32_t npos = 0;
        if ( sstOptions[i].opt.val ) {
            npos += fprintf(stderr, "  -%c, ", (char)sstOptions[i].opt.val);
        } else {
            npos += fprintf(stderr, "      ");
        }
        npos += fprintf(stderr, "--%s", sstOptions[i].opt.name);
        if ( sstOptions[i].opt.has_arg != no_argument ) {
            npos += fprintf(stderr, "=%s", sstOptions[i].argName);
        }
        if ( npos >= desc_start ) { fprintf(stderr, "\n"); npos = 0; }



        const char *text = sstOptions[i].desc;
        while ( text != NULL  && *text != '\0' ) {
            /* Advance to offset */
            while ( npos < desc_start ) npos += fprintf(stderr, " ");

            if ( strlen(text) <= desc_width ) {
                fprintf(stderr, "%s", text);
                break;
            } else {
                /* TODO:  Handle hyphenation more intelligently */
                int nwritten = fprintf(stderr, "%.*s-\n", desc_width-1, text);
                text += (nwritten -2);
                npos = 0;
            }
        }
        fprintf(stderr, "\n");
    }

    return false; /* Should not continue */
}


int
Config::parseCmdLine(int argc, char* argv[]) {
    static const char* sst_short_options = "hvVn:";
    struct option sst_long_options[nLongOpts + 1];
    for ( size_t i = 0 ; i < nLongOpts ; i++ ) {
        sst_long_options[i] = sstOptions[i].opt;
    }
    sst_long_options[nLongOpts] = {NULL, 0 ,0, 0};

    run_name = argv[0];

    bool ok = true;
    while (ok) {
        int option_index = 0;
        const int intC = getopt_long(argc, argv, sst_short_options, sst_long_options, &option_index);

        if ( intC == -1 ) /* We're done */
            break;

	const char c = static_cast<char>(intC);

        switch (c) {
        case 0:
            /* Long option, no short option */
            if ( optarg == NULL ) {
                ok = (this->*sstOptions[option_index].flagFunc)();
            } else {
                ok = (this->*sstOptions[option_index].argFunc)(optarg);
            }
            break;

        case 'v':
            ok = incrVerbose();
            break;

        case 'n': {
            ok = setNumThreads(optarg);
            break;
        }

        case 'V':
            ok = printVersion();
            break;
        case 'h':
        case '?':
        default:
	    
            ok = usage();
        }
    }

    if ( !ok ) return 1;

    /* Handle non-positional arguments */
    if ( optind < argc ) {
        ok = setConfigFile(argv[optind++]);
        /* Support further additional arguments to be args to the model */
        while ( ok && optind < argc ) {
            ok = setModelOptions(argv[optind++]);
        }
    }

    if ( !ok ) return 1;

    /* Sanity check, and other duties */
    Output::setFileName( debugFile != "/dev/null" ? debugFile : "sst_output" );

    if ( configFile == "NONE" && generator == "NONE" ) {
        cout << "ERROR: no sdl-file and no generator specified" << endl;
        cout << "  Usage: " << run_name << " sdl-file [options]" << endl;
        return -1;
    }

    // Ensure output directory ends with a directory separator
    if( output_directory.size() > 0 ) {
	if( '/' != output_directory[output_directory.size() - 1] ) {
		output_directory.append("/");
	}
    }

    // Now make sure all the files we are generating go into a directory
    if( output_config_graph.size() > 0 && isFileNameOnly(output_config_graph) ) {
	output_config_graph.insert( 0, output_directory );
    }

    if( output_dot.size() > 0 && isFileNameOnly(output_dot) ) {
	output_dot.insert( 0, output_directory );
    }

    if( output_xml.size() > 0 && isFileNameOnly(output_xml) ) {
	output_xml.insert( 0, output_directory );
    }

    if( output_json.size() > 0 && isFileNameOnly(output_json) ) {
	output_json.insert( 0, output_directory );
    }

    if( debugFile.size() > 0 && isFileNameOnly(debugFile) ) {
	debugFile.insert( 0, output_directory );
    }

    return 0;
}


bool Config::setConfigEntryFromModel(const string &entryName, const string &value)
{
    for ( size_t i = 0 ; i < nLongOpts ; i++ ) {
        if ( !entryName.compare(sstOptions[i].opt.name) ) {
            if ( NULL != sstOptions[i].argFunc ) {
                return (this->*sstOptions[i].argFunc)(value);
            } else {
                return (this->*sstOptions[i].flagFunc)();
            }
        }
    }
    fprintf(stderr, "Unknown configuration entry [%s]\n", entryName.c_str());
    return false;
}



bool Config::printVersion() {
    printf("SST-Core Version (" PACKAGE_VERSION);
    if (strcmp(SSTCORE_GIT_HEADSHA, PACKAGE_VERSION)) { 
        printf(", git branch : " SSTCORE_GIT_BRANCH);
        printf(", SHA: " SSTCORE_GIT_HEADSHA);
    }
    printf(")\n");

    return false; /* Should not continue */
}


bool Config::setConfigFile(const std::string &arg) {
    struct stat sb;
    char *fqpath = realpath(arg.c_str(), NULL);
    if ( NULL == fqpath ) {
        fprintf(stderr, "Failed to canonicalize path [%s]:  %s\n", arg.c_str(), strerror(errno));
        return false;
    }
    configFile = fqpath;
    free(fqpath);
    if ( 0 != stat(configFile.c_str(), &sb) ) {
        fprintf(stderr, "File [%s] cannot be found: %s\n", configFile.c_str(), strerror(errno));
        return false;
    }
    if ( ! S_ISREG(sb.st_mode) ) {
        fprintf(stderr, "File [%s] is not a regular file.\n", configFile.c_str());
        return false;
    }

    if ( 0 != access(configFile.c_str(), R_OK) ) {
        fprintf(stderr, "File [%s] is not readable.\n", configFile.c_str());
        return false;
    }

    return true;
}


bool Config::setDebugFile(const std::string &arg) { debugFile = arg; return true; }

/* TODO: Error checking */
bool Config::setLibPath(const std::string &arg) { libpath = arg; return true; }
/* TODO: Error checking */
bool Config::addLibPath(const std::string &arg) { libpath += std::string(":") + arg; return true; }


bool Config::setRunMode(const std::string &arg) {
    if( ! arg.compare( "init" ) ) runMode =  Simulation::INIT;
    else if( ! arg.compare( "run" ) )  runMode =  Simulation::RUN;
    else if( ! arg.compare( "both" ) ) runMode =  Simulation::BOTH;
    else runMode =  Simulation::UNKNOWN;

    return runMode != Simulation::UNKNOWN;
}


/* TODO: Error checking */
bool Config::setStopAt(const std::string &arg) { stopAtCycle = arg;  return true; }
/* TODO: Error checking */
bool Config::setStopAfter(const std::string &arg) {
    errno = 0;

    static const char *templates[] = {
        "%H:%M:%S",
        "%M:%S",
        "%S",
        "%Hh",
        "%Mm",
        "%Ss"
    };
    const size_t n_templ = sizeof(templates) / sizeof(templates[0]);
    struct tm res = {}; /* This warns on GCC 4.8 due to a bug in GCC */
    char *p;

    for ( size_t i = 0 ; i < n_templ ; i++ ) {
        memset(&res, '\0', sizeof(res));
        p = strptime(arg.c_str(), templates[i], &res);
        fprintf(stderr, "**** [%s]  p = %p ; *p = '%c', %u:%u:%u\n", templates[i], p, (p) ? *p : '\0', res.tm_hour, res.tm_min, res.tm_sec);
        if ( p != NULL && *p == '\0' ) {
            stopAfterSec = res.tm_sec;
            stopAfterSec += res.tm_min * 60;
            stopAfterSec += res.tm_hour * 60 * 60;
            return true;
        }
    }

    fprintf(stderr, "Failed to parse stop time [%s]\n"
            "Valid formats are:\n", arg.c_str());
    for ( size_t i = 0 ; i < n_templ ; i++ ) {
        fprintf(stderr, "\t%s\n", templates[i]);
    }

    return false;
}
/* TODO: Error checking */
bool Config::setHeartbeat(const std::string &arg) { heartbeatPeriod = arg;  return true; }
/* TODO: Error checking */
bool Config::setTimebase(const std::string &arg) { timeBase = arg;  return true; }

/* TODO: Error checking */
bool Config::setPartitioner(const std::string &arg) {
    partitioner = arg;
    if ( partitioner.find('.') == partitioner.npos ) {
        partitioner = "sst." + partitioner;
    }
    return true;
}
/* TODO: Error checking */
bool Config::setGenerator(const std::string &arg) { generator = arg; return true; }

bool Config::setGeneratorOptions(const std::string &arg) {
    if ( generator_options.empty() )
        generator_options = arg;
    else
        generator_options += std::string(" \"") + arg + std::string("\"");
    return true;
}

bool Config::setTimeVortex(const std::string &arg) {
    timeVortex = arg;
    return true;
}

bool Config::setOutputDir(const std::string &arg) { output_directory = arg ;  return true; }
bool Config::setWriteConfig(const std::string &arg) { output_config_graph = arg;  return true; }
bool Config::setWriteDot(const std::string &arg) { output_dot = arg; return true; }
bool Config::setWriteXML(const std::string &arg){ output_xml = arg; return true; }
bool Config::setWriteJSON(const std::string &arg) { output_json = arg; return true; }
bool Config::setWritePartition(const std::string &arg) { dump_component_graph_file = arg; return true; }
bool Config::setOutputPrefix(const std::string &arg) { output_core_prefix = arg; return true; }
#ifdef USE_MEMPOOL
bool Config::setWriteUndeleted(const std::string &arg) { event_dump_file = arg; return true; }
#endif

bool Config::setModelOptions(const std::string &arg) {
    if ( model_options.empty() )
        model_options = arg;
    else
        model_options += std::string(" \"") + arg + std::string("\"");
    return true;
}


bool Config::setVerbosity(const std::string &arg) {
    errno = E_OK;
    unsigned long val = strtoul(arg.c_str(), NULL, 0);
    if ( errno == E_OK ) {
        verbose = val;
        return true;
    }
    fprintf(stderr, "Failed to parse [%s] as number\n", arg.c_str());
    return false;
}


bool Config::setNumThreads(const std::string &arg) {
    errno = E_OK;
    unsigned long nthr = strtoul(arg.c_str(), NULL, 0);
    if ( errno == E_OK ) {
        world_size.thread = nthr;
        return true;
    }
    fprintf(stderr, "Failed to parse [%s] as number of threads\n", arg.c_str());
    return false;
}



/* Getters */

bool Config::printTimingInfo() {
	return print_timing;
}

uint32_t Config::getVerboseLevel() {
	return verbose;
}


std::string Config::getLibPath(void) const {
    char *envpath = getenv("SST_LIB_PATH");

    // Get configuration options from the user config
    std::vector<std::string> overrideConfigPaths;
    SST::Core::Environment::EnvironmentConfiguration* envConfig =
        SST::Core::Environment::getSSTEnvironmentConfiguration(overrideConfigPaths);

    std::string fullLibPath = libpath;
    std::set<std::string> configGroups = envConfig->getGroupNames();

    // iterate over groups of settings
    for(auto groupItr = configGroups.begin(); groupItr != configGroups.end(); groupItr++) {
        SST::Core::Environment::EnvironmentConfigGroup* currentGroup =
            envConfig->getGroupByName(*groupItr);
        std::set<std::string> groupKeys = currentGroup->getKeys();

        // find which keys have a LIBDIR at the END of the key
        // we recognize these may house elements
        for(auto keyItr = groupKeys.begin(); keyItr != groupKeys.end(); keyItr++) {
            const std::string key = *keyItr;
            const std::string value = currentGroup->getValue(key);

            if("BOOST_LIBDIR" != key) {
                if(key.size() > 6) {
                    if("LIBDIR" == key.substr(key.size() - 6)) {
                        fullLibPath.append(":");
                        fullLibPath.append(value);
                    }
                }
            }
        }
    }

    // Clean up and delete the configuration we just loaded up
    delete envConfig;

    if(NULL != envpath) {
        fullLibPath.clear();
        fullLibPath.append(envpath);
    }

    if ( !addlLibPath.empty() ) {
        fullLibPath.append(":");
        fullLibPath.append(addlLibPath);
    }

    if(verbose) {
        std::cout << "SST-Core: Configuration Library Path will read from: " << fullLibPath << std::endl;
    }

    return fullLibPath;
}

} // namespace SST
