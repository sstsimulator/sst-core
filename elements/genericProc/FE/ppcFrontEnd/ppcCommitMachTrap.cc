// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2007, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include "ppcFront.h"
#include <mach/mach.h>
/*
#include <mach/port.h>
*/
// wcm: struct sizes are ok in 64-bit atm.
typedef struct {
    mach_msg_header_t Head;
    NDR_record_t NDR;
    host_flavor_t flavor;
    mach_msg_type_number_t host_info_outCnt;
} Request;

typedef struct {
    mach_msg_header_t Head;
    NDR_record_t NDR;
    kern_return_t RetCode;
    mach_msg_type_number_t host_info_outCnt;
    integer_t host_info_out[12];
    mach_msg_trailer_t trailer;
} Reply;

union Mess {
    Request In;
    Reply Out;
};


void ppcInstruction::do_host_info(processor* proc, simRegister *regs) 
{
  Mess message;

  //parent->CopyFromSIM(&message, regs[3], sizeof(message));
  parent->home->CopyFromSIM(&message, htonl(regs[3]), parent->pid(), sizeof(message));

  if (message.In.flavor != HOST_BASIC_INFO) {
    printf("request for hostinfo type %d\n", message.In.flavor);
  } else {
    host_basic_info basic_info;
    basic_info.max_cpus    = ntohl(1);
    basic_info.avail_cpus  = ntohl(1);
    basic_info.memory_size = ntohl(1024*1024*1024); // 1 gig, why not?
    basic_info.cpu_type    = ntohl(CPU_TYPE_POWERPC);
    basic_info.cpu_subtype = ntohl(CPU_SUBTYPE_POWERPC_750); // g3

    memcpy(message.Out.host_info_out, &basic_info, 4*HOST_BASIC_INFO_COUNT);

    message.Out.host_info_outCnt = ntohl(HOST_BASIC_INFO_COUNT);
    message.Out.RetCode = ntohl(KERN_SUCCESS);
    message.Out.Head.msgh_id = ntohl(300);
    message.Out.Head.msgh_size = ntohl(sizeof(message.Out)-sizeof(mach_msg_trailer_t));

    for (unsigned int i = 0; i < sizeof(message)/4; ++i) {
      printf(" %10x %x\n", ((uint*)&message.Out)[i], 
	     regs[3]+(i*4));
    }

    //parent->CopyToSIM(regs[3], &message, sizeof(message));
    parent->home->CopyToSIM(htonl(regs[3]), parent->pid(), &message, sizeof(message));
  }

  regs[3] = 0;
}


void ppcInstruction::do_clock_get_time(processor* proc, simRegister *regs) 
{

Mess message;


    parent->home->CopyFromSIM(&message, ntohl(regs[3]), parent->pid(), sizeof(message));

    printf("Head.msgh_size %d\n", ntohl(message.In.Head.msgh_size));
    printf("Head.msgh_id %d\n", ntohl(message.In.Head.msgh_id));
    printf("host_flavor_t %d\n", ntohl(message.In.flavor));
    printf("host_info_outCnt %d\n", ntohl(message.In.host_info_outCnt));

    message.Out.host_info_outCnt = htonl(HOST_BASIC_INFO_COUNT);
    message.Out.RetCode = htonl(KERN_SUCCESS);
    message.Out.Head.msgh_id = message.In.Head.msgh_id;
    message.Out.Head.msgh_size = htonl(sizeof(message.Out)-sizeof(mach_msg_trailer_t));
    parent->home->CopyToSIM(ntohl(regs[3]), parent->pid(), &message, sizeof(message));

    regs[3] = 0;
}



bool ppcInstruction::Perform_SYS_mach_msg_trap(processor*   proc, 
	                                       simRegister* regs)
{
    mach_msg_header_t head;
    //parent->CopyFromSIM(&head, regs[3], sizeof(head));
    parent->home->CopyFromSIM(&head, ntohl(regs[3]), parent->pid(), sizeof(head));
    int msgID = ntohl(head.msgh_id);

    switch(msgID) {
    case 200:
      // host info - ignore
      do_host_info(proc, regs);
      break;
    case 1000: // clock_get_time
      do_clock_get_time(proc, regs);
      break;
    case 3206:
      // mach_port_deallocate
    case 3801:
      // vm_protect - ignore
      regs[3] = 0;
      break;
    case 3810:
      // vm_map - ignore
      regs[3] = 0;
      break;
    case 3812:
      // vm_remap - ignore
      regs[3] = 0;
      break;
    case 3412:
      // thread_create_running
      printf("mach thread_create_running called!\n");
      break;
    case 3617: // thread_policy_set
      printf("mach thread_policy_set called\n");
      regs[3] = 0;
      break;

    case 1001: // clock_get_attributes
    case 1002: // clock_alarm
    default:
      printf("Unknown mach msg id: %d\n", msgID);
    }

    return true;
}



bool ppcInstruction::Perform_SYS_mach(processor* proc, simRegister *regs)
{
  int trapNum = ntohl(regs[0]);

  switch (trapNum) {
    case -59:	// priority switch
    case -61:	// thread switch
      { 
        parent->yieldCount = !parent->yieldCount;
        if (parent->yieldCount) {	
  	// yields    
  	static int c = 0;
  	c++;
  	if (c == 100) {
  	  printf("100 yields (Mach trap type %i)\n", trapNum);
  	  c = 0;
  	}       	
  	break;
        } else {
  	_exception = YIELD_EXCEPTION;
  	return false;
        }
      }
      break;
  
    case -31: //mach_msg_trap
      return Perform_SYS_mach_msg_trap(proc, regs);
      break;
  
    case -26: // mach reply port
      return true;
      break;

    case -27: // mach_thread_self
      /* KBW: faking it (lie) */
      //regs[0] = htonl(MACH_PORT_NULL);
      regs[0] = 0;
      return true;
      break;
  
    default:
      printf("%p: Unknown mach trap %d\n", parent, trapNum);
  }

  return true;
}
// EOF
