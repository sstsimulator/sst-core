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

#ifndef SST_CORE_CONFIG_H
#define SST_CORE_CONFIG_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>
#include <sst/core/simulation.h>
#include <sst/core/rankInfo.h>
#include <sst/core/env/envquery.h>
#include <sst/core/env/envconfig.h>

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

    /** Create a new Config object.
     * @param my_rank - parallel rank of this instance
     * @param world_size - number of parallel ranks in the simulation
     */
    Config(RankInfo world_size);
    ~Config();

    /** Parse command-line arguments to update configuration values */
    int parseCmdLine( int argc, char* argv[] );
    /** Parse a configuration string to update configuration values */
    int parseConfigFile( std::string config_string );
    /** Return the current Verbosity level */
    uint32_t getVerboseLevel();

    /** Print the SST core timing information */
    bool printTimingInfo();

    /** Set the cycle at which to stop the simulation */
    void setStopAt(std::string stopAtStr);
    /** Sets the default timebase of the simulation */
    void setTimeBase(std::string timeBase);
    /** Print the current configuration to stdout */
    void Print();

    std::string     debugFile;          /*!< File to which debug information should be written */
    Simulation::Mode_t          runMode;            /*!< Run Mode (Init, Both, Run-only) */
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

    RankInfo        world_size;         /*!< Number of ranks, threads which should be invoked per rank */
    uint32_t        verbose;            /*!< Verbosity */
    bool	    no_env_config;      /*!< Bypass compile-time environmental configuration */
    bool            enable_sig_handling; /*!< Enable signal handling */
    bool            print_timing;       /*!< Print SST timing information */

#ifdef USE_MEMPOOL
    std::string     event_dump_file;    /*!< File to dump undeleted events to */
#endif
    /** Set the run-mode
     * @param mode - string "init" "run" "both"
     * @return the Mode_t corresponding
     */
    inline Simulation::Mode_t setRunMode( std::string mode )
    {
        if( ! mode.compare( "init" ) ) return Simulation::INIT;
        if( ! mode.compare( "run" ) ) return Simulation::RUN;
        if( ! mode.compare( "both" ) ) return Simulation::BOTH;
        return Simulation::UNKNOWN;
    }

    Simulation::Mode_t getRunMode() {
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
        	std::cout << "num_threads = " << world_size.thread << std::endl;
		std::cout << "enable_sig_handling = " << enable_sig_handling << std::endl;
		std::cout << "output_core_prefix = " << output_core_prefix << std::endl;
      		std::cout << "print_timing=" << print_timing << std::endl;
	}

    /** Return the library search path */
    std::string getLibPath(void) const {
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

	if(verbose) {
		std::cout << "SST-Core: Configuration Library Path will read from: " << fullLibPath << std::endl;
	}

        return fullLibPath;
    }

    uint32_t getNumRanks() { return world_size.rank; }
    uint32_t getNumThreads() { return world_size.thread; }

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
        ar & BOOST_SERIALIZATION_NVP(world_size);
	ar & BOOST_SERIALIZATION_NVP(enable_sig_handling);
        ar & BOOST_SERIALIZATION_NVP(output_core_prefix);
        ar & BOOST_SERIALIZATION_NVP(print_timing);
    }
    
    int rank;
	int numRanks;

};

} // namespace SST

#endif // SST_CORE_CONFIG_H
