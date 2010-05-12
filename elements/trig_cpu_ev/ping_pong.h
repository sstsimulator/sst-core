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


#ifndef COMPONENTS_TRIG_CPU_PING_PONG_H
#define COMPONENTS_TRIG_CPU_PING_PONG_H

#include "algorithm.h"
#include "trig_cpu.h"

#define PP_BUF_SIZE 1

class ping_pong :  public algorithm {
public:
    ping_pong(trig_cpu *cpu) : algorithm(cpu)
    {
        ptl = cpu->getPortalsHandle();
    }

    bool
    operator()(Event *ev)
    {
	ptl_md_t md;
	ptl_me_t me;

	ptl_handle_me_t me_handle;

	printf("state = %d\n",state);
        switch (state) {
        case 0:
	    printf("%5d: Inializing...\n",my_id);

	    ptl->PtlCTAlloc(PTL_CT_OPERATION,ct_handle);
	    state = 1;
	    break;

	case 1:
	    recv_buffer = new uint64_t[PP_BUF_SIZE];
	    send_buffer = new uint64_t[PP_BUF_SIZE];

	    {
		int temp = my_id;
		for ( int i = 0; i < PP_BUF_SIZE; i++ ) {
		    recv_buffer[i] = temp;
		    send_buffer[i] = temp++;
		}
	    }

            md.start = send_buffer;
            md.length = 8*PP_BUF_SIZE;
            md.eq_handle = PTL_EQ_NONE;
	    md.ct_handle = PTL_CT_NONE;
// 	    md.ct_handle = ct_handle;
            ptl->PtlMDBind(md, &md_handle);

	    state = 2;
	    break;
	    
	case 2:
	    
            // initialize things
            me.start = recv_buffer;
            me.length = 8*PP_BUF_SIZE;
            me.ignore_bits = ~0x0;
            me.ct_handle = ct_handle;
            ptl->PtlMEAppend(0, me, PTL_PRIORITY_LIST, NULL, me_handle);

	    for ( int i = 0; i < PP_BUF_SIZE; i++ ) {
		printf("%5d: start -> send_buffer[%d] = %llu   recv_buffer[%d] = %llu\n",my_id,i,send_buffer[i],i,recv_buffer[i]);
	    }

	    
	    state = 3;
	    break;

	case 3:
	  start_time = cpu->getCurrentSimTimeNano();
	  ptl->PtlPut(md_handle,0,PP_BUF_SIZE*8,0,(my_id + 1) % num_nodes,0,0,0,NULL,0);  
	  state = 4;
	    break;

	case 4:
	  if ( ptl->PtlCTWait(ct_handle,1) ) {
	    state = 5;
	  }
	    break;
	    
        case 5:
	  ptl->PtlPut(md_handle,0,PP_BUF_SIZE*8,0,(my_id + (num_nodes - 1)) % num_nodes,0,0,0,NULL,0);  
	  state = 6;
	    break;

	case 6:
	    if ( ptl->PtlCTWait(ct_handle,2) ) {
		for ( int i = 0; i < PP_BUF_SIZE; i++ ) {
		    printf("%5d: end -> send_buffer[%d] = %llu   recv_buffer[%d] = %llu\n",my_id,i,send_buffer[i],i,recv_buffer[i]);
		}
		uint64_t elapsed_time = cpu->getCurrentSimTimeNano()-start_time;
		trig_cpu::addTimeToStats(elapsed_time);
		return true;
	    }
	    return false;
            break;
	default:
	  break;
	}
	return false;
    }

private:
    ping_pong();
    ping_pong(const algorithm& a);
    void operator=(ping_pong const&);

    trig_cpu *cpu;
    portals *ptl;
    ptl_handle_ct_t ct_handle;
    int state;
    int my_id;
    int num_nodes;
    
    ptl_handle_md_t md_handle;

    uint64_t* send_buffer;
    uint64_t* recv_buffer;

    SimTime_t start_time;
    
};

#endif // COMPONENTS_TRIG_CPU_TEST_PORTALS_H
