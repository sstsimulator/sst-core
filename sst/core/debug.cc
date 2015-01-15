// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include "sst/core/debug.h"

namespace SST {

unsigned long _debug_flags = 0;
int _debug_rank = -1;
int _debug_nnodes = 0;

FILE* _dbg_stream = stdout;

void DebugInit( int rank, int nnodes )
{
    _debug_rank = rank;
    _debug_nnodes = nnodes;
}

int Str2Type( std::string name ) {
    if ( ! name.compare("cache") ) return DBG_CACHE;
    if ( ! name.compare("queue") ) return DBG_QUEUE;
    if ( ! name.compare("archive") ) return DBG_ARCHIVE;
    if ( ! name.compare("clock") ) return DBG_CLOCK;
    if ( ! name.compare("sync") ) return DBG_SYNC;
    if ( ! name.compare("link") ) return DBG_LINK;
    if ( ! name.compare("linkmap") ) return DBG_LINKMAP;
    if ( ! name.compare("linkc2m") ) return DBG_LINKC2M;
    if ( ! name.compare("linkc2c") ) return DBG_LINKC2C;
    if ( ! name.compare("linkc22") ) return DBG_LINKC2S;
    if ( ! name.compare("comp") ) return DBG_COMP;
    if ( ! name.compare("factory") ) return DBG_FACTORY;
    if ( ! name.compare("stop") ) return DBG_STOP;
    if ( ! name.compare("compevent") ) return DBG_COMPEVENT;
    if ( ! name.compare("sim") ) return DBG_SIM;
    if ( ! name.compare("clockevent") ) return DBG_CLOCKEVENT;
    if ( ! name.compare("sdl") ) return DBG_SDL;
    if ( ! name.compare("graph") ) return DBG_GRAPH;
    if ( ! name.compare("partition") ) return DBG_PARTITION;
    if ( ! name.compare("exit") ) return DBG_EXIT;
    if ( ! name.compare("memory") ) return DBG_MEMORY;
    if ( ! name.compare("network") ) return DBG_NETWORK;
    if ( ! name.compare("oneshot") ) return DBG_ONESHOT;
    if ( ! name.compare("all") ) return DBG_ALL;
    return DBG_UNKNOWN;
}

int DebugSetFlag( std::vector<std::string> strFlags )
{
    int dbgFlag;

    _debug_flags = 0;
    for (unsigned int i = 0; i < strFlags.size(); i++ ) {
	// fprintf(stderr, "*** Option %d is \"%s\"\n", i, strFlags.at(i).c_str());
	dbgFlag = Str2Type( strFlags.at(i).c_str() );
	if ( dbgFlag != DBG_UNKNOWN ) {
	    _debug_flags = _debug_flags | dbgFlag;
	} else {
	    fprintf(stderr, "Unknown debug flag \"%s\"\n", strFlags.at(i).c_str());
	    return -1;
	}
    }

    return 0;
}

int DebugSetFile( const std::string& filename )
{
    FILE *fp = fopen(filename.c_str(), "w");
    if ( !fp ) {
        fprintf(stderr, "Unable to set debug file to '%s'\n", filename.c_str());
        perror("fopen");
        return -1;
    }
    _dbg_stream = fp;
    return 0;
}

} // namespace SST
