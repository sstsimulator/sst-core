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

#include <sst/core/archive.h>
#include <string>
#include <sst/core/sst.h>

namespace SST {
class Config {
public:
    typedef enum { UNKNOWN, INIT, RUN, BOTH } Mode_t;

    Config();
    int Init( int argc, char *argv[], int rank );
    void Print();

    bool            archive;
#if WANT_CHECKPOINT_SUPPORT
    Archive::Type_t atype;
    std::string     archiveFile;
#endif
    Mode_t          runMode;
    std::string     libpath;
    std::string     sdlfile;
    Cycle_t         stopAtCycle;
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
