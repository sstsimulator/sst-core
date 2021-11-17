// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CONFIG_H
#define SST_CORE_CONFIG_H

#include "sst/core/env/envconfig.h"
#include "sst/core/env/envquery.h"
#include "sst/core/rankInfo.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/simulation.h"
#include "sst/core/sst_types.h"

#include <string>

namespace SST {

/**
 * Class to contain SST Simulation Configuration variables
 */
class Config : public SST::Core::Serialization::serializable
{
public:
    /** Create a new Config object.
     * @param world_size - number of parallel ranks in the simulation
     */
    Config(RankInfo world_size);
    Config() {} // For serialization
    ~Config();

    /**
       Parse command-line arguments to update configuration values.

       @return Returns 0 if execution should continue.  Returns -1
       if there was an error.  Returns 1 if run command line only
       asked for information to be print (e.g. --help or -V).
     */
    int      parseCmdLine(int argc, char* argv[]);
    /** Set a configuration string to update configuration values */
    bool     setConfigEntryFromModel(const std::string& entryName, const std::string& value);
    /** Return the current Verbosity level */
    uint32_t getVerboseLevel();

    /** Print the SST core timing information */
    bool printTimingInfo();

    /** Print the current configuration to stdout */
    void Print();

    std::string        debugFile;                 /*!< File to which debug information should be written */
    Simulation::Mode_t runMode;                   /*!< Run Mode (Init, Both, Run-only) */
    std::string        configFile;                /*!< Graph generation file */
    std::string        stopAtCycle;               /*!< When to stop the simulation */
    uint32_t           stopAfterSec;              /*!< When (wall-time) to stop the simulation */
    std::string        heartbeatPeriod;           /*!< Sets the heartbeat period for the simulation */
    std::string        timeBase;                  /*!< Timebase of simulation */
    std::string        partitioner;               /*!< Partitioner to use */
    std::string        timeVortex;                /*!< TimeVortex implementation to use */
    bool               inter_thread_links;        /*!< Use interthread links */
    std::string        output_config_graph;       /*!< File to dump configuration graph */
    std::string        output_dot;                /*!< File to dump dot output */
    uint32_t           dot_verbosity;             /*!< Amount of detail to include in the dot graph output */
    std::string        output_xml;                /*!< File to dump XML output */
    std::string        output_json;               /*!< File to dump JSON output */
    std::string        output_directory;          /*!< Output directory to dump all files to */
    std::string        model_options;             /*!< Options to pass to Python Model generator */
    std::string        dump_component_graph_file; /*!< File to dump component graph */
    bool               output_partition;   /*!< If set to true, output paritition information in config outputs */
    std::string        output_core_prefix; /*!< Set the SST::Output prefix for the core */

    RankInfo world_size;          /*!< Number of ranks, threads which should be invoked per rank */
    uint32_t verbose;             /*!< Verbosity */
    bool     parallel_load;       /*!< Load simulation graph in parallel */
    bool     parallel_output;     /*!< Output simulation graph in parallel */
    bool     no_env_config;       /*!< Bypass compile-time environmental configuration */
    bool     enable_sig_handling; /*!< Enable signal handling */
    bool     print_timing;        /*!< Print SST timing information */
    bool     print_env;           /*!< Print SST environment */

#ifdef USE_MEMPOOL
    std::string event_dump_file; /*!< File to dump undeleted events to */
#endif

    typedef bool (Config::*flagFunction)(void);
    typedef bool (Config::*argFunction)(const std::string& arg);

    bool usage();
    bool printVersion();
    bool incrVerbose()
    {
        verbose++;
        return true;
    }
    bool setVerbosity(const std::string& arg);
    bool disableSigHandlers()
    {
        enable_sig_handling = false;
        return true;
    }
    bool disableEnvConfig()
    {
        no_env_config = true;
        return true;
    }
    bool enablePrintTiming()
    {
        print_timing = true;
        return true;
    }
    bool enablePrintEnv()
    {
        print_env = true;
        return true;
    }

#ifdef SST_CONFIG_HAVE_MPI
    bool enableParallelLoad()
    {
        parallel_load = true;
        return true;
    }

    bool enableParallelOutput()
    {
        // If this is only a one rank job, then we can ignore, except
        // that we will turn on output_partition
        if ( world_size.rank != 1 ) parallel_output = true;
        // For parallel output, we always need to output the partition
        // info as well
        output_partition = true;
        return true;
    }
#endif

    bool setConfigFile(const std::string& arg);
    bool checkConfigFile();
    bool setDebugFile(const std::string& arg);
    bool setLibPath(const std::string& arg);
    bool addLibPath(const std::string& arg);
    bool setRunMode(const std::string& arg);
    bool setStopAt(const std::string& arg);
    bool setStopAfter(const std::string& arg);
    bool setHeartbeat(const std::string& arg);
    bool setTimebase(const std::string& arg);
    bool setPartitioner(const std::string& arg);
    bool setTimeVortex(const std::string& arg);
    bool setInterThreadLinks(const std::string& arg);
    bool setOutputDir(const std::string& arg);
    bool setWriteConfig(const std::string& arg);
    bool setWriteDot(const std::string& arg);
    bool setDotVerbosity(const std::string& arg);
    bool setWriteXML(const std::string& arg);
    bool setWriteJSON(const std::string& arg);
    bool setWritePartitionFile(const std::string& arg);
    bool setWritePartition();
    bool setOutputPrefix(const std::string& arg);
#ifdef USE_MEMPOOL
    bool setWriteUndeleted(const std::string& arg);
#endif
    bool setModelOptions(const std::string& arg);
    bool setNumThreads(const std::string& arg);

    Simulation::Mode_t getRunMode() { return runMode; }

    /** Print to stdout the current configuration */
    void print()
    {
        std::cout << "debugFile = " << debugFile << std::endl;
        std::cout << "runMode = " << runMode << std::endl;
        std::cout << "libpath = " << getLibPath() << std::endl;
        std::cout << "configFile = " << configFile << std::endl;
        std::cout << "stopAtCycle = " << stopAtCycle << std::endl;
        std::cout << "stopAfterSec = " << stopAfterSec << std::endl;
        std::cout << "timeBase = " << timeBase << std::endl;
        std::cout << "partitioner = " << partitioner << std::endl;
        std::cout << "output_config_graph = " << output_config_graph << std::endl;
        std::cout << "output_dot = " << output_dot << std::endl;
        std::cout << "dot_verbosity = " << dot_verbosity << std::endl;
        std::cout << "output_xml = " << output_xml << std::endl;
        std::cout << "no_env_config = " << no_env_config << std::endl;
        std::cout << "output_directory = " << output_directory << std::endl;
        std::cout << "output_json = " << output_json << std::endl;
        std::cout << "model_options = " << model_options << std::endl;
        std::cout << "dump_component_graph_file = " << dump_component_graph_file << std::endl;
        std::cout << "output_partition = " << output_partition << std::endl;
        std::cout << "num_threads = " << world_size.thread << std::endl;
        std::cout << "enable_sig_handling = " << enable_sig_handling << std::endl;
        std::cout << "output_core_prefix = " << output_core_prefix << std::endl;
        std::cout << "parallel_load=" << parallel_load << std::endl;
        std::cout << "parallel_output=" << parallel_output << std::endl;
        std::cout << "print_timing=" << print_timing << std::endl;
        std::cout << "print_env" << print_env << std::endl;
    }

    /** Return the library search path */
    std::string getLibPath(void) const;

    uint32_t getNumRanks() { return world_size.rank; }
    uint32_t getNumThreads() { return world_size.thread; }
    uint32_t setNumThreads(uint32_t nthr)
    {
        uint32_t old      = world_size.thread;
        world_size.thread = nthr;
        return old;
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& debugFile;
        ser& runMode;
        ser& libpath;
        ser& addlLibPath;
        ser& configFile;
        ser& stopAtCycle;
        ser& stopAfterSec;
        ser& timeBase;
        ser& partitioner;
        ser& dump_component_graph_file;
        ser& output_partition;
        ser& output_config_graph;
        ser& output_dot;
        ser& dot_verbosity;
        ser& output_xml;
        ser& output_json;
        ser& no_env_config;
        ser& model_options;
        ser& world_size;
        ser& enable_sig_handling;
        ser& output_core_prefix;
        ser& parallel_load;
        ser& parallel_output;
        ser& print_timing;
    }

private:
    std::string run_name;
    std::string libpath;
    std::string addlLibPath;

    int rank;
    int numRanks;

    bool isFileNameOnly(const std::string& name)
    {
        bool nameOnly = true;

        for ( size_t i = 0; i < name.size(); ++i ) {
            if ( '/' == name[i] ) {
                nameOnly = false;
                break;
            }
        }

        return nameOnly;
    }

    ImplementSerializable(SST::Config)
};

} // namespace SST

#endif // SST_CORE_CONFIG_H
