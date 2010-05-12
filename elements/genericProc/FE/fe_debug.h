// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _FE_DEBUG_H_
#define _FE_DEBUG_H_

extern int __print_end_timestamp;
extern int __dprint_level;
extern int __print_warn;
extern int __print_info;


#define USE_DPRINT


#include <stdlib.h>

#ifdef USE_DPRINT
#define DPRINT(x,fmt,args...) if ( __dprint_level >= x ) \
		printf( "%s:%s(), "fmt, __FILE__,__FUNCTION__, ## args )
#else
#define DPRINT(x,fmt,args...)
#endif

#define ERROR(fmt,args...) {\
	printf( "ERROR:%s:%s(), "fmt, __FILE__,__FUNCTION__, ## args );\
	exit(1);}

#define WARN(fmt,args...) if ( __print_warn) \
	printf( "WARNING:%s:%s(), "fmt, __FILE__,__FUNCTION__, ## args )

#define MSG(fmt,args...) \
	printf( "MSG:%s:%s(), "fmt, __FILE__,__FUNCTION__, ## args )

#define INFO(fmt,args...) \
	if ( __print_info ) printf( "INFO: "fmt, ## args )
#define INFO2(fmt,args...) \
	if ( __print_info) printf( fmt, ## args )

#define PRINTF(fmt,args...) \
	printf( "%s() "fmt, __FUNCTION__, ## args )

#endif  // _FE_DEBUG_H_
