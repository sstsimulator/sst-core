// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2010, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef PIMSYSCALLTYES_H
#define PIMSYSCALLTYES_H

//: Identifiers for NIC commands
typedef enum {NC_NONE = 0, 
	      NC_INIT, NC_IRECV, NC_ISEND, NC_UBUF, NC_IPROBE, NC_ISSEND} NICCmdType;

//: Identifiers for Coprocessors
typedef enum {PIM_MAINPROC = 0,
	      PIM_NIC = 1,
	      PIM_ANY_PIM = 2,
	      PIM_SMPPROC = 3,
	      PIM_MAX_COPROC=4} PIM_coProc;

typedef enum {PIM_ADDR_LOCAL = 0,
	      PIM_ADDR_NONLOCAL = 1,
	      PIM_ADDR_NET_IN = 2,
	      PIM_ADDR_NET_OUT = 3,
	      PIM_ADDR_PIO = 4} PIM_addrMode;

typedef enum {PIM_CMD_PROC_NUM=0, /* which processor currently? */
	      PIM_CMD_THREAD_ID=1,
	      PIM_CMD_SET_THREAD_ID=2,
	      PIM_CMD_THREAD_SEQ=3,
	      PIM_CMD_NUM_PROC=4, /* how many processors? */
	      PIM_CMD_LOCAL_ALLOC,
	      PIM_CMD_CYCLE,
	      PIM_CMD_NUM_SYS,
	      PIM_CMD_INTERRUPT_FOR_THREAD,

	      PIM_CMD_NIC_CMD,
	      PIM_CMD_NIC_CMD_READ,

	      PIM_CMD_NET_READ,
	      PIM_CMD_NET_READ_BLOCK,
	      PIM_CMD_NET_READ_DONE,
	      PIM_CMD_NET_WRITE,
	      PIM_CMD_NET_WRITE_BLOCK,
	      PIM_CMD_NET_SEND_DONE,

	      PIM_CMD_NET_CHECK_INCOMING,
	      PIM_CMD_NET_CHECK_OUTGOING,
	      PIM_CMD_NET_SET_OUTGOING_DEST,

	      PIM_CMD_WIDGET_START_INSERT,
	      PIM_CMD_WIDGET_STOP_INSERT,
	      PIM_CMD_WIDGET_INSERT,
	      PIM_CMD_WIDGET_CHECK,
	      PIM_CMD_WIDGET_INSERT_HEADER,

	      PIM_CMD_LU_POST_RECV,
	      PIM_CMD_LU_READ_LU_Q_1,
	      PIM_CMD_LU_READ_LU_Q_2,

	      PIM_CMD_DMA_INIT,	     
	      PIM_CMD_DMA_START,
	      PIM_CMD_DMA_STATUS,
	      PIM_CMD_QDMA_INIT_PTX,	     
	      PIM_CMD_QDMA_INIT,	     
	      PIM_CMD_QDMA_START,
	      PIM_CMD_QDMA_STATUS,

	      PIM_CMD_LOC_COUNT,
	      PIM_CMD_LOCAL_CTRL,
	      PIM_CMD_SET_MIGRATE,
	      PIM_CMD_SET_EVICT,
	      PIM_CMD_SET_FUTURE,
	      PIM_CMD_ICOUNT,
	      PIM_CMD_MAX_LOCAL_ALLOC,
	      PIM_CMD_GET_NUM_CORE, /* how many cores? */
	      PIM_CMD_GET_CORE_NUM, /* which core currently? */
	      PIM_CMD_GET_MHZ, /* how many MHz? */
	      PIM_CMD_GET_NUM_NODES,
	      PIM_CMD_GET_NODE_NUM,
	      PIM_CMD_BARRIER_ENTER_SIGNAL,
	      PIM_CMD_BARRIER_ENTER_WAIT,
	      PIM_CMD_BARRIER_LEAVE_SIGNAL,
	      PIM_CMD_BARRIER_LEAVE_WAIT,
	      
	      PIM_CMD_GET_CTOR
} PIM_cmd;

typedef enum {MSG_OK,
	      MSG_EMPTY,
	      NOT_AVAIL} PIM_netStatus;

typedef enum {DMA_AVAIL,
	      DMA_WORKING,
	      DMA_ERROR} PIM_dmaStatus;

typedef enum {PIM_REGION_TEXT,
              PIM_REGION_DATA,
              PIM_REGION_HEAP,
              PIM_REGION_STACK
              } PIM_regions;

typedef enum {PIM_REGION_CACHED,
	     PIM_REGION_UNCACHED,
             PIM_REGION_WC,
             } PIM_region_types;

#endif
