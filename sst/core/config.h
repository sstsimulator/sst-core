// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
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
class Config {
public:
    typedef enum { UNKNOWN, INIT, RUN, BOTH } Mode_t;

    Config(int my_rank);
    ~Config();

    int parseCmdLine( int argc, char* argv[] );
    int parseConfigFile( std::string config_string );

    void setStopAt(std::string stopAtStr);
    void setTimeBase(std::string timeBase);
    void Print();

    bool            archive;
    std::string     debugFile;
    std::string     archiveType;
    std::string     archiveFile;
    Mode_t          runMode;
    std::string     sdlfile;
    std::string     stopAtCycle;
    std::string     timeBase;
    std::string     partitioner;
    std::string     generator;
    std::string     generator_options;
    std::string     dump_config_graph;
    std::string     output_dot;
    std::string     output_directory;
#ifdef HAVE_PYTHON
    std::string     model_options;
#endif
    std::string     dump_component_graph_file;

    bool            all_parse;
    uint32_t        verbose;
    bool 	    no_env_config;
    
    inline Mode_t
    setRunMode( std::string mode ) 
    {
        if( ! mode.compare( "init" ) ) return INIT; 
        if( ! mode.compare( "run" ) ) return RUN; 
        if( ! mode.compare( "both" ) ) return BOTH; 
        return UNKNOWN;
    }

    void print() {
	std::cout << "debugFile = " << debugFile << std::endl;
	std::cout << "archive = " << archive << std::endl;
	std::cout << "archiveType = " << archiveType << std::endl;
	std::cout << "archiveFile = " << archiveFile << std::endl;
	std::cout << "runMode = " << runMode << std::endl;
	std::cout << "libpath = " << getLibPath() << std::endl;
	std::cout << "sdlfile = " << sdlfile << std::endl;
	std::cout << "stopAtCycle = " << stopAtCycle << std::endl;
	std::cout << "timeBase = " << timeBase << std::endl;
	std::cout << "partitioner = " << partitioner << std::endl;
	std::cout << "generator = " << generator << std::endl;
	std::cout << "gen_options = " << generator_options << std::endl;
        std::cout << "dump_config_graph = " << dump_config_graph << std::endl;
	std::cout << "no_env_config = " << no_env_config << std::endl;
        std::cout << "output_directory = " << output_directory << std::endl;
#ifdef HAVE_PYTHON
        std::cout << "model_options = " << model_options << std::endl;
#endif
    }

    std::string getLibPath(void) const {
        char *envpath = getenv("SST_LIB_PATH");
        if ( !addlLibPath.empty() ) {
            return addlLibPath + ":" + libpath;
        } else if ( NULL != envpath ) {
            return envpath;
        }
        return libpath;
    }

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
	ar & BOOST_SERIALIZATION_NVP(archive);
	ar & BOOST_SERIALIZATION_NVP(archiveType);
	ar & BOOST_SERIALIZATION_NVP(archiveFile);
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
        ar & BOOST_SERIALIZATION_NVP(dump_config_graph);
        ar & BOOST_SERIALIZATION_NVP(no_env_config);
#ifdef HAVE_PYTHON
        ar & BOOST_SERIALIZATION_NVP(model_options);
#endif
    }

    int rank;

};

} // namespace SST

#endif // SST_CORE_CONFIG_H
