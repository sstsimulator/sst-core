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


#ifndef COMPONENTS_TRIG_CPU_TEST_MPI_H
#define COMPONENTS_TRIG_CPU_TEST_MPI_H

#include "algorithm.h"
#include "trig_cpu.h"

#define TEST_MPI_BUF_SIZE 32

class test_mpi :  public algorithm {
public:
    test_mpi(trig_cpu *cpu) : algorithm(cpu)
    {
        ptl = cpu->getPortalsHandle();
    }

    bool
    operator()(Event *ev)
    {
	int handle;
// 	printf("state = %d\n",state); */
        switch (state) {
        case 0:
	    printf("%5d: Inializing...\n",my_id);
	    recv_buffer = new uint64_t[TEST_MPI_BUF_SIZE];
	    send_buffer = new uint64_t[TEST_MPI_BUF_SIZE];

	    {
		int temp = my_id;
		for ( int i = 0; i < TEST_MPI_BUF_SIZE; i++ ) {
		    recv_buffer[i] = temp;
		    send_buffer[i] = temp++;
		}
	    }

	    state = 1;
	    break;

	case 1:
	  cpu->isend((my_id + 1) % num_nodes,send_buffer,TEST_MPI_BUF_SIZE*8/4);
	  start_time = cpu->getCurrentSimTimeNano();
	    state = 2;
	    break;
	    
	case 2:
	  cpu->irecv((my_id + num_nodes - 1) % num_nodes,recv_buffer,handle);
	    state = 3;
	    break;

	case 3:
	  cpu->irecv((my_id + num_nodes - 1) % num_nodes,&recv_buffer[TEST_MPI_BUF_SIZE/2],handle);
	  start_time = cpu->getCurrentSimTimeNano();
	    state = 4;
	    break;
	    
	case 4:
	  cpu->isend((my_id + 1) % num_nodes,send_buffer,TEST_MPI_BUF_SIZE*8/4);
	    state = 5;
	    break;

	case 5:
	  if ( cpu->waitall() )
	    state = 6;
	  break;
	    
	case 6:
	{
	  for ( int i = 0; i < 32; i++ ) {
	    printf("%5d: end -> send_buffer[%d] = %llu   recv_buffer[%d] = %llu\n",my_id,i,send_buffer[i],i,recv_buffer[i]);
	  }
	  uint64_t elapsed_time = cpu->getCurrentSimTimeNano()-start_time;
	  trig_cpu::addTimeToStats(elapsed_time);
	  return true;
	  state = 7;
	}
	  break;
	    
        case 7:
	    state = 7;
	    break;

	case 8:
	    return false;
            break;
	default:
	  break;
	}
	return false;
    }

private:
    test_mpi();
    test_mpi(const algorithm& a);
    void operator=(test_mpi const&);

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

#endif // COMPONENTS_TRIG_CPU_TEST_MPI_H
