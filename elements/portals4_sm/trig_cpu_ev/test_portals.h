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


#ifndef COMPONENTS_TRIG_CPU_TEST_PORTALS_H
#define COMPONENTS_TRIG_CPU_TEST_PORTALS_H

#include "algorithm.h"
#include "trig_cpu.h"

#define BUF_SIZE 32

class test_portals :  public algorithm {
public:
    test_portals(trig_cpu *cpu) : algorithm(cpu)
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
	    recv_buffer = new uint64_t[BUF_SIZE];
	    send_buffer = new uint64_t[BUF_SIZE];

	    {
		int temp = my_id;
		for ( int i = 0; i < BUF_SIZE; i++ ) {
		    recv_buffer[i] = temp;
		    send_buffer[i] = temp++;
		}
	    }

            md.start = send_buffer;
            md.length = 8*BUF_SIZE;
            md.eq_handle = PTL_EQ_NONE;
	    md.ct_handle = PTL_CT_NONE;
	    md.ct_handle = ct_handle;
            ptl->PtlMDBind(md, &md_handle);

	    state = 2;
	    break;
	    
	case 2:
	    
            // initialize things
            me.start = recv_buffer;
            me.length = 8*BUF_SIZE;
            me.ignore_bits = ~0x0;
            me.ct_handle = ct_handle;
            start_time = cpu->getCurrentSimTimeNano();
            ptl->PtlMEAppend(0, me, PTL_PRIORITY_LIST, NULL, me_handle);

	    for ( int i = 0; i < 16; i++ ) {
		printf("%5d: start -> send_buffer[%d] = %llu   recv_buffer[%d] = %llu\n",my_id,i,send_buffer[i],i,recv_buffer[i]);
	    }

	    
	    state = 3;
	    break;

	case 3:
  	    ptl->PtlTriggeredCTInc(ct_handle,1,ct_handle,1);
	    state = 4;
	    break;

	case 4:
	    
//  	    ptl->PtlTriggeredGet(md_handle,0,32,(my_id + 1) % num_nodes,0,0,NULL,128,ct_handle,2);
 	  ptl->PtlTriggeredPut(md_handle,0,0,0,(my_id + 1) % num_nodes,0,0,0,NULL,0,ct_handle,3);
	    state = 5;
	    break;
	    
        case 5:
	    // Send to my neighbor one bigger
// 	    ptl->PtlPut(md_handle,0,128,0,( my_id + 1 ) % num_nodes,0,0,0,NULL,0);
	    ptl->PtlPut(md_handle,0,128,0,( my_id + 1 ) % num_nodes,0,0,0,NULL,0);
	    state = 6;
	    break;

	case 6:
	    if ( ptl->PtlCTWait(ct_handle,5) ) {
		for ( int i = 0; i < 32; i++ ) {
		    printf("%5d: end -> send_buffer[%d] = %llu   recv_buffer[%d] = %llu\n",my_id,i,send_buffer[i],i,recv_buffer[i]);
		}
		uint64_t elapsed_time = cpu->getCurrentSimTimeNano()-start_time;
		trig_cpu::addTimeToStats(elapsed_time);
// 		if (my_id == 0) {
// 		    double bw = ((double)(BUF_SIZE * 8)) / ((double)(elapsed_time));
// 		    printf("Approximate BW = %3.2lf GB/s\n",my_id,bw);
// 		}
		return true;
	    }
	    return false;
//             // Now send a message to the guy just bigger than me
//             ptl->PtlTriggeredCTInc(ct_handle,4,ct_handle,3);
//             ptl->PtlTriggeredPut(1,1,1,0,(my_id + 2) % num_nodes,0,0xdeadbeafabcdefabLL,0,NULL,0,ct_handle,1);
//             ptl->PtlTriggeredAtomic(1,1,1,0,(my_id + 3) % num_nodes,0,0,0,NULL,0,PTL_MIN,PTL_LONG,ct_handle,2);
//             ptl->PtlPut(1,1,1,0,(my_id + 1) % num_nodes,0,0,0,NULL,0);
//             state = 3;
            break;
//         case 3:
//             if ( ptl->PtlCTWait(ct_handle,7) ) {
//                 trig_cpu::addTimeToStats(1);
//                 state = 4;
//                 return true;
//             }
//             cpu->addBusyTime("100ns");
//             break;
//         default:
//             cpu->addBusyTime("100ns");
//             break;
	}
	return false;
    }

private:
    test_portals();
    test_portals(const algorithm& a);
    void operator=(test_portals const&);

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
