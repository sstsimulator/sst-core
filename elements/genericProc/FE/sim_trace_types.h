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



#ifndef SIM_TRACE_TYPES_H
#define SIM_TRACE_TYPES_H

#define USE_PIM_TRACE
#ifdef USE_PIM_TRACE
#define PIM_TRACE(x,y,z) PIM_trace(x,y,z)
#else
#define PIM_TRACE(x,y,z)
#endif


typedef enum {
  SIM_MPI_SEND,
  SIM_MPI_RECV,
  SIM_MPI_ISEND,
  SIM_MPI_IRECV,
  SIM_MPI_WAIT,

  SIM_PTL_PUT,
  SIM_PTL_GET,
  SIM_PTL_PUT_REGION,
  SIM_PTL_GET_REGION,
  SIM_PTL_GET_PUT,
  SIM_PTL_GET_PUT_REGION,
  SIM_PTL_MEMD_POST,
  SIM_PTL_FWD_USER,
  SIM_PTL_FWD_KERNEL,

  SIM_FIRM_handle_accel,
  SIM_FIRM_handle_command,
  SIM_FIRM_tx_complete,
  SIM_FIRM_rx_complete,
  SIM_FIRM_rx_message,
  SIM_FIRM_memd_post_command,
  SIM_FIRM_accel_tx_command,
  SIM_FIRM_extract_ptlhdr,
  SIM_FIRM_accel_parse_put,
  SIM_FIRM_accel_parse_ack,
  SIM_FIRM_accel_parse_get,
  SIM_FIRM_accel_parse_reply,  
  SIM_FIRM_accel_rx_done,
  SIM_FIRM_match,
  SIM_APP,
  SIM_HTLINK,
  SIM_SS_DMA_TX,
  SIM_SS_DMA_RX,
  SIM_MEM_WRITE,
  SIM_ONIC,
  SIM_SHMEM_INT_PUT,
  SIM_SHMEM_INT_GOT,
  SIM_FUNC_ANY,

} TraceType_t;

typedef enum {
  SIM_ANY,
  SIM_ENTER,
  SIM_RETURN,
  SIM_CHECK,
  SIM_FOUND,
  SIM_REQ,
  SIM_RESP,
} TraceFlag_t;

#endif
