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

#ifndef SST_CORE_CONFIG_H
#define SST_CORE_CONFIG_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <string>

namespace boost {
    namespace program_options {

	class options_description;
	class positional_options_description;
	class variables_map;
    }
}

namespace SST {

/**
 * Class to contain SST Simulation Configuration variables
 */
class Config {
public:
    /** Type of Run Modes */
    typedef enum {
        UNKNOWN,    /*!< Unknown mode - Invalid for running */
        INIT,       /*!< Initialize-only.  Useful for debugging initialization and graph generation */
        RUN,        /*!< Run-only.  Useful when restoring from a checkpoint */
        BOTH        /*!< Default.  Both initialize and Run the simulation */
    } Mode_t;

    /** Create a new Config object.
     * @param my_rank - parallel rank of this instance
     * @param world_size - number of parallel ranks in the simulation
     */
    Config(int my_rank, int world_size);
    ~Config();

    /** Parse command-line arguments to update configuration values */
    int parseCmdLine( int argc, char* argv[] );
    /** Parse a configuration string to update configuration values */
    int parseConfigFile( std::string config_string );
    /** Return the current Verbosity level */
    uint32_t getVerboseLevel();

    /** Set the cycle at which to stop the simulation */
    void setStopAt(std::string stopAtStr);
    /** Sets the default timebase of the simulation */
    void setTimeBase(std::string timeBase);
    /** Print the current configuration to stdout */
    void Print();

    std::string     debugFile;          /*!< File to which debug information should be written */
    Mode_t          runMode;            /*!< Run Mode (Init, Both, Run-only) */
    std::string     sdlfile;            /*!< Graph generation file */
    std::string     stopAtCycle;        /*!< When to stop the simulation */
    std::string     heartbeatPeriod;    /*!< Sets the heartbeat period for the simulation */
    std::string     timeBase;           /*!< Timebase of simulation */
    std::string     partitioner;        /*!< Partitioner to use */
    std::string     generator;          /*!< Generator to use */
    std::string     generator_options;  /*!< Options to pass to the generator */
    std::string     output_config_graph;  /*!< File to dump configuration graph */
    std::string     output_dot;         /*!< File to dump dot output */
    std::string     output_xml;         /*!< File to dump XML output */
    std::string     output_json;        /*!< File to dump JSON output */
    std::string     output_directory;   /*!< Output directory to dump all files to */
    std::string     model_options;      /*!< Options to pass to Python Model generator */
    std::string     dump_component_graph_file; /*!< File to dump component graph */
    std::string     output_core_prefix;  /*!< Set the SST::Output prefix for the core */

    uint32_t        verbose;            /*!< Verbosity */
    bool	    no_env_config;      /*!< Bypass compile-time environmental configuration */
    bool            enable_sig_handling; /*!< Enable signal handling */

#ifdef USE_MEMPOOL
    std::string     event_dump_file;    /*!< File to dump undeleted events to */
#endif
    /** Set the run-mode
     * @param mode - string "init" "run" "both"
     * @return the Mode_t corresponding
     */
    inline Mode_t setRunMode( std::string mode )
    {
        if( ! mode.compare( "init" ) ) return INIT;
        if( ! mode.compare( "run" ) ) return RUN;
        if( ! mode.compare( "both" ) ) return BOTH;
        return UNKNOWN;
    }

    Mode_t getRunMode() {
	return runMode;
    }

    /** Print to stdout the current configuration */
	void print() {
		std::cout << "debugFile = " << debugFile << std::endl;
		std::cout << "runMode = " << runMode << std::endl;
		std::cout << "libpath = " << getLibPath() << std::endl;
		std::cout << "sdlfile = " << sdlfile << std::endl;
		std::cout << "stopAtCycle = " << stopAtCycle << std::endl;
		std::cout << "timeBase = " << timeBase << std::endl;
		std::cout << "partitioner = " << partitioner << std::endl;
		std::cout << "generator = " << generator << std::endl;
		std::cout << "gen_options = " << generator_options << std::endl;
		std::cout << "output_config_graph = " << output_config_graph << std::endl;
		std::cout << "output_xml = " << output_xml << std::endl;
		std::cout << "no_env_config = " << no_env_config << std::endl;
		std::cout << "output_directory = " << output_directory << std::endl;
		std::cout << "output_json = " << output_json << std::endl;
		std::cout << "model_options = " << model_options << std::endl;
		std::cout << "enable_sig_handling = " << enable_sig_handling << std::endl;
		std::cout << "output_core_prefix = " << output_core_prefix << std::endl;
	}

    /** Return the library search path */
    std::string getLibPath(void) const {
        char *envpath = getenv("SST_LIB_PATH");
        if ( !addlLibPath.empty() ) {
            return addlLibPath + ":" + libpath;
        } else if ( NULL != envpath ) {
            return envpath;
        }
        return libpath;
    }

    /** Return the current Parallel Rank */
	int getRank() const { return rank; }
    /** Return the number of parallel ranks in the simulation */
	int getNumRanks() const { return numRanks; }

private:
    boost::program_options::options_description* visNoConfigDesc;
    boost::program_options::options_description* hiddenNoConfigDesc;
    boost::program_options::options_description* legacyDesc;
    boost::program_options::options_description* mainDesc;
    boost::program_options::positional_options_description* posDesc;
    boost::program_options::variables_map* var_map;
    std::string run_name;
    std::string libpath;
    std::string addlLibPath;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_NVP(debugFile);
        ar & BOOST_SERIALIZATION_NVP(runMode);
        ar & BOOST_SERIALIZATION_NVP(libpath);
        ar & BOOST_SERIALIZATION_NVP(addlLibPath);
        ar & BOOST_SERIALIZATION_NVP(sdlfile);
        ar & BOOST_SERIALIZATION_NVP(stopAtCycle);
        ar & BOOST_SERIALIZATION_NVP(timeBase);
        ar & BOOST_SERIALIZATION_NVP(partitioner);
        ar & BOOST_SERIALIZATION_NVP(generator);
        ar & BOOST_SERIALIZATION_NVP(generator_options);
        ar & BOOST_SERIALIZATION_NVP(dump_component_graph_file);
        ar & BOOST_SERIALIZATION_NVP(output_config_graph);
	ar & BOOST_SERIALIZATION_NVP(output_xml);
	ar & BOOST_SERIALIZATION_NVP(output_json);
        ar & BOOST_SERIALIZATION_NVP(no_env_config);
        ar & BOOST_SERIALIZATION_NVP(model_options);
	ar & BOOST_SERIALIZATION_NVP(enable_sig_handling);
        ar & BOOST_SERIALIZATION_NVP(output_core_prefix);
    }
    
    int rank;
	int numRanks;

};

} // namespace SST

#endif // SST_CORE_CONFIG_H
