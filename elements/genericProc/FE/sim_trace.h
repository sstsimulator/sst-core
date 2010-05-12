// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



#ifndef SIM_TRACE_H
#define SIM_TRACE_H

#include <stdio.h>

#include <sim_trace_types.h>

void SIM_trace( unsigned long long ts, int node, const char *str,
        unsigned int var1, unsigned int var2, unsigned int var3 );

void SIM_MHZ( int mhz);
void SIM_open( const char *name );
void SIM_close();


#endif
