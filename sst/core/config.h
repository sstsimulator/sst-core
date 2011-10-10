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


#ifndef SST_CONFIG_H
#define SST_CONFIG_H

#include <string>
#include <sst/core/sst_types.h>

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

    int parse_cmd_line( int argc, char* argv[] );
    int parse_config_file( std::string config_string );
    
    int Init( int argc, char *argv[], int rank );
    void Print();

    bool            archive;
    std::string     archiveType;
    std::string     archiveFile;
    Mode_t          runMode;
    std::string     libpath;
    std::string     sdlfile;
    std::string     stopAtCycle;
    std::string     timeBase;
    std::string     partitioner;

    std::string     sdl_version;
    inline Mode_t
    RunMode( std::string mode ) 
    {
        if( ! mode.compare( "init" ) ) return INIT; 
        if( ! mode.compare( "run" ) ) return RUN; 
        if( ! mode.compare( "both" ) ) return BOTH; 
        return UNKNOWN;
    }

    void print() {
	std::cout << "archive = " << archive << std::endl;
	std::cout << "archiveType = " << archiveType << std::endl;
	std::cout << "archiveFile = " << archiveFile << std::endl;
	std::cout << "runMode = " << runMode << std::endl;
	std::cout << "libpath = " << libpath << std::endl;
	std::cout << "sdlfile = " << sdlfile << std::endl;
	std::cout << "stopAtCycle = " << stopAtCycle << std::endl;
	std::cout << "timeBase = " << timeBase << std::endl;
	std::cout << "partitioner = " << partitioner << std::endl;
	std::cout << "sdl_version = " << sdl_version << std::endl;
    }

private:
    boost::program_options::options_description* helpDesc;
    boost::program_options::options_description* hiddenDesc;
    boost::program_options::options_description* mainDesc;
    boost::program_options::positional_options_description* posDesc;
    boost::program_options::variables_map* var_map;
    std::string run_name;

    int rank;

};

} // namespace SST

#endif // SST_CONFIG_H
