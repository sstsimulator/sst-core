// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, 2009, 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.




#include <sim_trace.h>
#include "fe_debug.h"

FILE *traceFP = NULL;

const char * _traceTypesTable[] = { 
	"MPI_Send", 
	"MPI_Recv",
  	"MPI_Isend",
  	"MPI_Irecv",
  	"MPI_Wait",

	"PtlPut",
	"PtlGet",
	"PtlPutRegion",
	"PtlGetRegion",
	"PtlGetPut",
	"PtlGetPutRegion",
	"PtlMEMDPost",
	"accel_forward_to_firware",
	"accel_forward_trusted_command_to_nic",

	"handle_accel",
	"handle_command",
	"tx_complete",
        "rx_complete",
        "rx_message",
        "memd_post_command",
        "accel_tx_command",
	"extract_ptlhdr",
	"accel_parse_put",
	"accel_parse_ack",
	"accel_parse_get",
	"accel_parse_reply",
        "accel_rx_done",
        "match",
        "application",
        "HTLink",
        "DMA_TX",
        "DMA_RX",
	"mem_write",
        "onic",
        "shmem_int_put",
        "shmem_int_got",
	"ANY",
};

const char * _traceFlagsTable[] = { 
	"ANY",
	"ENTER",
	"RETURN",
        "check",
        "found",
        "req",
        "resp",
};



static double _mhz = 0;

void SIM_MHZ(int mhz )
{
  _mhz = (double) mhz;
}

void SIM_open( const char *name )
{
  if ( ( traceFP = fopen( name, "w+") ) == NULL ) {
    ERROR("can't create trace file %s\n",name);
  }

}
void SIM_close( )
{
  if ( traceFP) {
    fclose(traceFP);
  }
}

void SIM_trace( unsigned long long ts, int node, const char *str,
	unsigned int var1, unsigned int var2, unsigned int var3 )
{
  static unsigned long long prev = 0;
  static int pos = 1;
  
  if ( traceFP ) {

#define __FLOAT__
#ifdef __FLOAT__ 
    fprintf(traceFP, "%d\t%lf\t%lf\tnode%d\t%s\t%s\t%s\t%#x\n",
                pos++, ts/_mhz, (ts-prev)/_mhz,node,str,
		_traceTypesTable[var1], _traceFlagsTable[var2], var3 );
#else
    fprintf(traceFP, "%d\t%lld\t%lld\tnode%d\t%s\t%s\t%s\t%#x\n",
                pos++, ts, (ts-prev),node,str,
		_traceTypesTable[var1], _traceFlagsTable[var2], var3 );
#endif
    prev = ts;
  }
}
