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


#ifndef _SST_CORE_DEBUG_H
#define _SST_CORE_DEBUG_H

#include <stdio.h>
#include <sst/core/serialization.h>

namespace SST {

extern unsigned long _debug_flags;
extern int _debug_rank;
extern int _debug_nnodes;
extern FILE* _dbg_stream;

extern void DebugInit( int rank, int nnodes );
extern int DebugSetFlag( std::vector<std::string> strFlags );
extern int DebugSetFile( const std::string& filename );

} // namespace SST

#define DBG_UNKNOWN    (-1)
#define DBG_ANY        (1<<0)
#define DBG_CACHE      (1<<1)
#define DBG_QUEUE      (1<<2)
#define DBG_ARCHIVE    (1<<3)
#define DBG_CLOCK      (1<<4)
#define DBG_SYNC       (1<<5)
#define DBG_LINK       (1<<6)
#define DBG_LINKMAP    (1<<7)
#define DBG_LINKC2M    (1<<8)
#define DBG_LINKC2C    (1<<9)
#define DBG_LINKC2S    (1<<10)
#define DBG_COMP       (1<<11)
#define DBG_FACTORY    (1<<12)
#define DBG_STOP       (1<<13)
#define DBG_COMPEVENT  (1<<14)
#define DBG_SIM        (1<<15)
#define DBG_CLOCKEVENT (1<<16)
#define DBG_SDL        (1<<17)
#define DBG_GRAPH      (1<<18)
#define DBG_PARTITION  (1<<19)
#define DBG_EXIT       (1<<20)
#define DBG_MEMORY     (1<<21)
#define DBG_NETWORK    (1<<22)
#define DBG_ONESHOT    (1<<23)
#define DBG_ALL        (DBG_ANY | \
			DBG_CACHE| \
			DBG_QUEUE| \
			DBG_ARCHIVE| \
			DBG_CLOCK| \
			DBG_SYNC| \
			DBG_LINK| \
			DBG_LINKMAP| \
			DBG_LINKC2M| \
			DBG_LINKC2C| \
			DBG_LINKC2S| \
			DBG_COMP| \
			DBG_FACTORY| \
			DBG_STOP| \
			DBG_COMPEVENT| \
			DBG_SIM| \
			DBG_CLOCKEVENT| \
			DBG_SDL| \
			DBG_GRAPH| \
			DBG_PARTITION | \
			DBG_EXIT| \
			DBG_MEMORY| \
			DBG_NETWORK| \
			DBG_ONESHOT| \
                        0)

#define __DBG( flag, name, fmt, args...) \
    do { \
        if (flag & SST::_debug_flags) {\
            fprintf(_dbg_stream, "%d:%s::%s():%d: " fmt, SST::_debug_rank, #name, __FUNCTION__,__LINE__, ## args ); \
            fflush(_dbg_stream); \
        } \
    } while(0)

#define _DBG( name, fmt, args...) \
         fprintf(_dbg_stream, "%d:%s::%s():%d: " fmt, SST::_debug_rank, #name, __FUNCTION__,__LINE__, ## args )

#define _AR_DBG( name, fmt, args...) __DBG( DBG_ARCHIVE, name, fmt, ## args )

#define _abort( name, fmt, args...)\
{\
    fprintf(stderr, "%d:%s::%s():%d:ABORT: " fmt, SST::_debug_rank, #name, __FUNCTION__,__LINE__, ## args ); \
    exit(-1); \
}\
/**/

#define _ABORT _abort

#endif // SST_CORE_DEBUG_H
