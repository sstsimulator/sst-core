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

#include "sst/core/simulation.h"

#include <string>

/* Forward declare for Friendship */
extern int main(int argc, char** argv);

namespace SST {

class ConfigHelper;
class SSTModelDescription;
/**
 * Class to contain SST Simulation Configuration variables
 */
class Config : public SST::Core::Serialization::serializable
{
private:
    // Main creates the config object
    friend int ::main(int argc, char** argv);
    friend class ConfigHelper;
    friend class SSTModelDescription;

    /**
       Config constructor.  Meant to only be created by main function
     */
    Config(RankInfo rank_info);

    /**
       Default constructor used for serialization
     */
    Config() {}

    //// Functions for use in main

    /**
       Parse command-line arguments to update configuration values.

       @return Returns 0 if execution should continue.  Returns -1
       if there was an error.  Returns 1 if run command line only
       asked for information to be print (e.g. --help or -V).
     */
    int parseCmdLine(int argc, char* argv[]);

    /**
       Checks for the existance of the config file.  This needs to be
       called after adding any rank numbers to the file in the case of
       parallel loading.
    */
    bool checkConfigFile();

    /**
       Get the final library path for loading element libraries
    */
    std::string getLibPath(void) const;

    // Functions to be called from ModelDescriptions

    /** Set a configuration string to update configuration values */
    bool setOptionFromModel(const std::string& entryName, const std::string& value);


public:
    /** Create a new Config object.
     * @param world_size - number of parallel ranks in the simulation
     */
    ~Config() {}

    // Functions to access config options. Declared in order they show
    // up in the options array

    // Information options
    // No variable associated with help
    // No variable associated with version

    // Basic options

    /**
       Level of verbosity to use in the core prints using
       Output.verbose or Output.debug.
    */
    uint32_t verbose() const { return verbose_; }


    /**
       Number of threads requested
    */
    uint32_t num_threads() const { return world_size_.thread; }

    /**
       Number of ranks in the simulation
     */
    uint32_t num_ranks() const { return world_size_.rank; }

    /**
       Name of the SDL file to use to genearte the simulation
    */
    const std::string& configFile() const { return configFile_; }

    /**
       Model options to pass to the SDL file
    */
    const std::string& model_options() const { return model_options_; }

    /**
       Print SST timing information after the run
    */
    bool print_timing() const { return print_timing_; }

    /**
       Simulated cycle to stop the simulation at
    */
    const std::string& stop_at() const { return stop_at_; }

    /**
       Wall clock time (approximiate) in seconds to stop the simulation at
    */
    uint32_t exit_after() const { return exit_after_; }

    /**
       Partitioner to use for parallel simualations
    */
    const std::string& partitioner() const { return partitioner_; }

    /**
       Simulation period at which to print out a "heartbeat" message
    */
    const std::string& heartbeatPeriod() const { return heartbeatPeriod_; }

    /**
       The directory to be used for writting output files
    */
    const std::string& output_directory() const { return output_directory_; }

    /**
       Prefix to use for the default SST::Output object in core
    */
    const std::string output_core_prefix() const { return output_core_prefix_; }

    // Configuration output

    /**
       File to output python formatted  config graph to (empty string means no
       output)
    */
    const std::string& output_config_graph() const { return output_config_graph_; }

    /**
       File to output json formatted config graph to (empty string means no
       output)
    */
    const std::string& output_json() const { return output_json_; }

    /**
       If true, and a config graph output option is specified, write
       each ranks graph separately
    */
    bool parallel_output() const { return parallel_output_; }

    /**
       File to output xml formatted config graph to (empty string means no
       output)

       @deprecated Does not support all of the current SST
       configuration features
    */
    const std::string& output_xml() const { return output_xml_; }

    // Graph output

    /**
       File to output dot formatted config graph to (empty string means no
       output).  Note, this is not a format that can be used as input for simulation

    */
    const std::string& output_dot() const { return output_dot_; }

    /**
       Level of verbosity to use for the dot output.
    */
    uint32_t dot_verbosity() const { return dot_verbosity_; }

    /**
       File to output component partition info to (empty string means no output)
    */
    const std::string& component_partition_file() const { return component_partition_file_; }

    /**
       Controls whether partition info is output as part of configuration output
     */
    bool output_partition() const { return output_partition_; }

    // Advanced options

    /**
       Core timebase to use as the atomic time unit for the
       simulation.  It is usually best to just leave this at the
       default (1ps)
    */
    const std::string& timeBase() const { return timeBase_; }

    /**
       Controls whether graph constuction will be done in parallel.
       If it is, then the SDL file name is modified to add the rank
       number to the file name right before the file extension, if
       parallel_load_mode_multi is true.
    */
    bool parallel_load() const { return parallel_load_; }

    /**
       If graph constuction will be done in parallel, will use a
       file per rank if true, and the same file for each rank if
       false.
    */
    bool parallel_load_mode_multi() const { return parallel_load_mode_multi_; }

    /**
       TimeVortex implementation to use
    */
    const std::string& timeVortex() const { return timeVortex_; }

    /**
       Use links that connect directly to ActivityQueue in receiving thread
    */
    bool interthread_links() const { return interthread_links_; }

    /**
       File to which core debug information should be written
    */
    const std::string& debugFile() const { return debugFile_; }

    /**
       Library path to use for finding element libraries (will replace
       the libpath in the sstsimulator.conf file)
    */
    const std::string& libpath() const { return libpath_; }

    /**
       Paths to add to library search (adds to libpath found in
       sstsimulator.conf file)
    */
    const std::string& addLibPath() const { return addLibPath_; }

    // Advanced options - Debug

    /**
       Run mode to use (Init, Both, Run-only).  Note that Run-only is
       not currently supported because there is not component level
       checkpointing.
    */
    Simulation::Mode_t runMode() const { return runMode_; }


#ifdef USE_MEMPOOL
    /**
       File to output list of events that remain undeleted at the end
       of the simulation.
    */
    const std::string& event_dump_file() const { return event_dump_file_; }
#endif

    /**
       Run simulation initialization stages one rank at a time for
       debug purposes
     */
    bool rank_seq_startup() const { return rank_seq_startup_; }

    // Advanced options - envrionment

    /**
       Controls whether the environment variables that SST sees are
       printed out
    */
    bool print_env() const { return print_env_; }

    /**
       Controls whether signal handlers are enable or not.  NOTE: the
       sense of this variable is opposite of the command line option
       (--disable-signal-handlers)
    */
    bool enable_sig_handling() const { return enable_sig_handling_; }

    // This option is used by the SST wrapper found in
    // bootshare.{h,cc} and is never actually accessed once sst.x
    // executes.
    bool no_env_config() const { return no_env_config_; }


    /** Print to stdout the current configuration */
    void print();

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& verbose_;
        ser& world_size_;
        ser& configFile_;
        ser& model_options_;
        ser& print_timing_;
        ser& stop_at_;
        ser& exit_after_;
        ser& partitioner_;
        ser& heartbeatPeriod_;
        ser& output_directory_;
        ser& output_core_prefix_;

        ser& output_config_graph_;
        ser& output_json_;
        ser& parallel_output_;
        ser& output_xml_;

        ser& output_dot_;
        ser& dot_verbosity_;
        ser& component_partition_file_;
        ser& output_partition_;

        ser& timeBase_;
        ser& parallel_load_;
        ser& parallel_load_mode_multi_;
        ser& timeVortex_;
        ser& interthread_links_;
        ser& debugFile_;
        ser& libpath_;
        ser& addLibPath_;
        ser& runMode_;

        ser& print_env_;
        ser& enable_sig_handling_;
        ser& no_env_config_;
    }
    ImplementSerializable(SST::Config)


private:
    //// Items private to Config

    std::string run_name;

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

    // Variables to hold the options.  Declared in order they show up
    // in the options array

    // Information options
    // No variable associated with help
    // No variable associated with version

    // Basic options
    uint32_t    verbose_; /*!< Verbosity */
    // Num threads held in RankInfo.thread
    RankInfo    world_size_;         /*!< Number of ranks, threads which should be invoked per rank */
    std::string configFile_;         /*!< Graph generation file */
    std::string model_options_;      /*!< Options to pass to Python Model generator */
    bool        print_timing_;       /*!< Print SST timing information */
    std::string stop_at_;            /*!< When to stop the simulation */
    uint32_t    exit_after_;         /*!< When (wall-time) to stop the simulation */
    std::string partitioner_;        /*!< Partitioner to use */
    std::string heartbeatPeriod_;    /*!< Sets the heartbeat period for the simulation */
    std::string output_directory_;   /*!< Output directory to dump all files to */
    std::string output_core_prefix_; /*!< Set the SST::Output prefix for the core */

    // Configuration output
    std::string output_config_graph_; /*!< File to dump configuration graph */
    std::string output_json_;         /*!< File to dump JSON output */
    bool        parallel_output_;     /*!< Output simulation graph in parallel */
    std::string output_xml_;          /*!< File to dump XML output */

    // Graph output
    std::string output_dot_;               /*!< File to dump dot output */
    uint32_t    dot_verbosity_;            /*!< Amount of detail to include in the dot graph output */
    std::string component_partition_file_; /*!< File to dump component graph */
    bool        output_partition_;         /*!< Output paritition info when writing config output */

    // Advanced options
    std::string timeBase_;                 /*!< Timebase of simulation */
    bool        parallel_load_;            /*!< Load simulation graph in parallel */
    bool        parallel_load_mode_multi_; /*!< If true, load using multiple files */
    std::string timeVortex_;               /*!< TimeVortex implementation to use */
    bool        interthread_links_;        /*!< Use interthread links */
    std::string debugFile_;                /*!< File to which debug information should be written */
    std::string libpath_;
    std::string addLibPath_;

    // Advanced options - debug
    Simulation::Mode_t runMode_; /*!< Run Mode (Init, Both, Run-only) */
#ifdef USE_MEMPOOL
    std::string event_dump_file_; /*!< File to dump undeleted events to */
#endif
    bool rank_seq_startup_; /*!< Run simulation initialization phases one rank at a time */

    // Advanced options - envrionment
    bool print_env_;           /*!< Print SST environment */
    bool enable_sig_handling_; /*!< Enable signal handling */
    bool no_env_config_;       /*!< Bypass compile-time environmental configuration */
};

} // namespace SST

#endif // SST_CORE_CONFIG_H
