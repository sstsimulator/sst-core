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

namespace SST {
class Config {
public:
    typedef enum { UNKNOWN, INIT, RUN, BOTH } Mode_t;

    Config();
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
	
    inline Mode_t
    RunMode( std::string mode ) 
    {
        if( ! mode.compare( "init" ) ) return INIT; 
        if( ! mode.compare( "run" ) ) return RUN; 
        if( ! mode.compare( "both" ) ) return BOTH; 
        return UNKNOWN;
    }
};

} // namespace SST

#endif // SST_CONFIG_H
