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


#ifndef COMPONENTS_TRIG_CPU_BANDWIDTH_H
#define COMPONENTS_TRIG_CPU_BANDWIDTH_H

#include "algorithm.h"
#include "trig_cpu.h"

#define BW_BUF_SIZE (128*1024)

class bandwidth :  public algorithm {
public:
    bandwidth(trig_cpu *cpu) : algorithm(cpu)
    {
        ptl = cpu->getPortalsHandle();
    }
  /*
    bool
    operator()(Event *ev)
    {
	ptl_md_t md;
	ptl_me_t me;

	ptl_handle_me_t me_handle;

// 	printf("state = %d\n",state);
        switch (state) {
        case 0:
	    printf("%5d: Inializing...\n",my_id);

	    ptl->PtlCTAlloc(PTL_CT_OPERATION,ct_handle);
	    state = 1;
	    break;

	case 1:
	    recv_buffer = new uint64_t[BW_BUF_SIZE];
	    send_buffer = new uint64_t[BW_BUF_SIZE];

	    {
		int temp = my_id;
		for ( int i = 0; i < BW_BUF_SIZE; i++ ) {
		    recv_buffer[i] = temp;
		    send_buffer[i] = temp++;
		}
	    }

            md.start = send_buffer;
            md.length = 8*BW_BUF_SIZE;
            md.eq_handle = PTL_EQ_NONE;
	    md.ct_handle = PTL_CT_NONE;
// 	    md.ct_handle = ct_handle;
            ptl->PtlMDBind(md, &md_handle);

	    state = 2;
	    break;
	    
	case 2:
	    
            // initialize things
            me.start = recv_buffer;
            me.length = 8*BW_BUF_SIZE;
            me.ignore_bits = ~0x0;
            me.ct_handle = ct_handle;
            ptl->PtlMEAppend(0, me, PTL_PRIORITY_LIST, NULL, me_handle);

// 	    for ( int i = 0; i < BW_BUF_SIZE; i++ ) {
// 		printf("%5d: start -> send_buffer[%d] = %llu   recv_buffer[%d] = %llu\n",my_id,i,send_buffer[i],i,recv_buffer[i]);
// 	    }

	    
	    state = 3;
	    break;

	case 3:
	  start_time = cpu->getCurrentSimTimeNano();
	  ptl->PtlTriggeredPut(md_handle,0,BW_BUF_SIZE*8,0,(my_id + 1) % num_nodes,0,0,0,NULL,0,ct_handle,1);
	  state = 4;
	    break;

	case 4:
	  ptl->PtlPut(md_handle,0,BW_BUF_SIZE*8,0,(my_id + 1) % num_nodes,0,0,0,NULL,0);
	  state = 6;
	  break;

// 	case 5:
// 	   	  if ( ptl->PtlCTWait(ct_handle,1) ) {
// 	    state = 5;
// 	  }
// 	    break;
	    
//         case 5:
// 	  ptl->PtlPut(md_handle,0,BW_BUF_SIZE*8,0,(my_id + (num_nodes - 1)) % num_nodes,0,0,0,NULL,0);  
// 	  state = 6;
// 	    break;

	case 6:
	    if ( ptl->PtlCTWait(ct_handle,2) ) {
		uint64_t elapsed_time = cpu->getCurrentSimTimeNano()-start_time;
		double bw = (double)(BW_BUF_SIZE*8*2) / (double)elapsed_time;
		if ( my_id == 0 ) printf("Estimated BW = %lf GB/s\n",bw);
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
  */
    bool
    operator()(Event *ev)
    {
	ptl_md_t md;
	ptl_me_t me;

	ptl_handle_me_t me_handle;

// 	printf("state = %d\n",state);
        switch (state) {
        case 0:
	    printf("%5d: Inializing...\n",my_id);

	    ptl->PtlCTAlloc(PTL_CT_OPERATION,ct_handle);
	    ptl->PtlCTAlloc(PTL_CT_OPERATION,ct_handle2);
	    state = 1;
	    break;

	case 1:
	    recv_buffer = new uint64_t[2*1024];
	    send_buffer = new uint64_t[2*1024];

	    {
 	      for ( int i = 0; i < (2*1024); i++ ) {
		    recv_buffer[i] = 0;
		    send_buffer[i] = i;
		}
	    }

	    if ( my_id == 0 ) {
	      md.start = NULL;
	      md.length = 0;
	      md.eq_handle = PTL_EQ_NONE;
	      md.ct_handle = PTL_CT_NONE;
	      ptl->PtlMDBind(md, &md_handle);
	    }
	    else {
	      md.start = recv_buffer;
	      md.length = 8*2*1024;
	      md.eq_handle = PTL_EQ_NONE;
	      md.ct_handle = ct_handle2;
	      ptl->PtlMDBind(md, &md_handle);
	    }
	    
	    state = 2;
	    break;
	    
	case 2:
	    
            // initialize things
	  if ( my_id == 0 ) {
	    me.start = send_buffer;
            me.length = 8*2*1024;
            me.ignore_bits = ~0x0;
            me.ct_handle = ct_handle;
            ptl->PtlMEAppend(0, me, PTL_PRIORITY_LIST, NULL, me_handle);
	  }
	  else {
	    me.start = NULL;
            me.length = 0;
            me.ignore_bits = ~0x0;
            me.ct_handle = ct_handle;
            ptl->PtlMEAppend(0, me, PTL_PRIORITY_LIST, NULL, me_handle);
	  }
	  if ( my_id == 0 )  {
	    state = 6;
	  }
	  else {
	    state = 3;
	  }
	  break;

	case 3:
	  start_time = cpu->getCurrentSimTimeNano();
	  loop_var_i = 0;
	  state = 4;
	  break;
	case 4:
	  if ( loop_var_i < 4 ) {
	    ptl->PtlTriggeredGet(md_handle,8*512*loop_var_i,8*512,0,0,0,NULL,8*512*loop_var_i,ct_handle,loop_var_i+1);
	    loop_var_i++;
	  }
	  else {
	    state = 5;
	  }
	  break;

	case 5:
	  if ( ptl->PtlCTWait(ct_handle2,4) ) {
	    int bad = 0;
	    for ( int i = 0; i < (2*1024); i++ ) {
                if ( (int) recv_buffer[i] != i ) bad++;
	    }
	    if ( bad ) {
	      printf("Bad results\n");
	    }
	    else {
	      printf("Good to go\n");
	    }
	    uint64_t elapsed_time = cpu->getCurrentSimTimeNano()-start_time;
	    trig_cpu::addTimeToStats(elapsed_time);
	    state = 1;
	    return true;
	  }
	  break;
	  
        // Rank 0
	case 6:
	  start_time = cpu->getCurrentSimTimeNano();
	  loop_var_i = 0;
	  state = 7;
	  break;

	case 7:
	  loop_var_j = 1;
	  state = 8;
	  break;

	case 8:
	  if ( loop_var_j < num_nodes ) {
	    ptl->PtlPut(md_handle,0,0,0,loop_var_j,0,0,0,NULL,0);
	    loop_var_j++;
	  }
	  else {
	    state = 9;
	  }
	  break;

	case 9:
	  if ( loop_var_i < 4 ) {
	    state = 7;
	    loop_var_i++;
	  }
	  else {
	    // All done
	    uint64_t elapsed_time = cpu->getCurrentSimTimeNano()-start_time;
	    trig_cpu::addTimeToStats(elapsed_time);
	    return true;
	  }
	    
	default:
	  break;
	}
	return false;
    }

private:
    bandwidth();
    bandwidth(const algorithm& a);
    void operator=(bandwidth const&);

    trig_cpu *cpu;
    portals *ptl;
    ptl_handle_ct_t ct_handle;
    ptl_handle_ct_t ct_handle2;
    int state;
    int my_id;
    int num_nodes;

  int loop_var_i;
  int loop_var_j;
  
    ptl_handle_md_t md_handle;

    uint64_t* send_buffer;
    uint64_t* recv_buffer;

    SimTime_t start_time;
    
};

#endif // COMPONENTS_TRIG_CPU_TEST_PORTALS_H
