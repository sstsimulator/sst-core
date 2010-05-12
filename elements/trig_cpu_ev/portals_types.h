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


#ifndef COMPONENTS_TRIG_CPU_PORTALS_TYPES_H
#define COMPONENTS_TRIG_CPU_PORTALS_TYPES_H

typedef uint32_t ptl_size_t;

typedef int32_t ptl_handle_ct_t;
typedef int32_t ptl_handle_eq_t;
typedef int16_t ptl_pt_index_t;
typedef uint32_t ptl_ack_req_t;

typedef uint64_t ptl_process_id_t;

typedef uint64_t ptl_hdr_data_t;

typedef uint64_t ptl_match_bits_t;

typedef struct {
    ptl_size_t success;
    ptl_size_t failure;
} ptl_ct_event_t;

typedef enum {
    PTL_CT_OPERATION, PTL_CT_BYTE
} ptl_ct_type_t;

// Operation types
#define PTL_OP_PUT      0
#define PTL_OP_GET      1
#define PTL_OP_GET_RESP 2
#define PTL_OP_ATOMIC   3
#define PTL_OP_CT_INC   4

#define PTL_EQ_NONE (-1)
#define PTL_CT_NONE (-1)

typedef enum { 
    PTL_PRIORITY_LIST, PTL_OVERFLOW, PTL_PROBE_ONLY 
} ptl_list_t;

typedef enum {
    PTL_MIN, PTL_MAX,
    PTL_SUM, PTL_PROD,
    PTL_LOR, PTL_LAND,
    PLT_BOR, PTL_BAND,
    PTL_LXOR, PTL_BXOR,
    PTL_SWAP, PTL_CSWAP, PTL_MSWAP
} ptl_op_t;

typedef enum {
    PTL_CHAR, PTL_UCHAR,
    PTL_SHORT, PTL_USHORT,
    PTL_INT, PTL_UINT,
    PTL_LONG, PTL_ULONG,
    PTL_FLOAT, PTL_DOUBLE
} ptl_datatype_t;

// Here's the MD
typedef struct { 
    void *start;
    ptl_size_t length;
    unsigned int options;  // not used
    ptl_handle_eq_t eq_handle; 
    ptl_handle_ct_t ct_handle; 
} ptl_md_t;

typedef ptl_md_t* ptl_handle_md_t;


// Here's the ME
typedef struct { 
    void *start; 
    ptl_size_t length; 
    ptl_handle_ct_t ct_handle; 
    ptl_size_t min_free;
    //  ptl_ac_id_t ac_id;
    unsigned int options; 
    //  ptl_process_id_t match_id; 
    ptl_match_bits_t match_bits; 
    ptl_match_bits_t ignore_bits; 
} ptl_me_t;


// Internal data structures
typedef uint32_t ptl_op_type_t;

typedef struct {
    ptl_me_t me;
    bool active;
    void *user_ptr;
    ptl_handle_ct_t handle_ct;
    ptl_pt_index_t pt_index;
    ptl_list_t ptl_list;
} ptl_int_me_t;

typedef ptl_int_me_t* ptl_handle_me_t;

typedef struct {
    void* start;
    ptl_size_t length;
    ptl_size_t offset;
    ptl_process_id_t target_id;
    ptl_handle_ct_t ct_handle;
    bool end;
    int stream;
} ptl_int_dma_t;

typedef struct {
  ptl_pt_index_t   pt_index;
  uint16_t         op;
  uint32_t         length;
  ptl_match_bits_t match_bits;
  ptl_size_t       remote_offset;
  ptl_handle_ct_t  get_ct_handle;
  void *           get_start;
} ptl_header_t;

typedef struct {
    ptl_op_type_t op_type;
    ptl_process_id_t target_id;
    ptl_pt_index_t pt_index;
    ptl_match_bits_t match_bits;
    ptl_handle_ct_t ct_handle;
    ptl_size_t increment;
    ptl_int_dma_t* dma;
    ptl_header_t* ptl_header;
} ptl_int_op_t;

typedef struct {
    ptl_size_t threshold;
    ptl_handle_ct_t trig_ct_handle;
    ptl_int_op_t* op;
} ptl_int_trig_op_t;

// Structs, etc needed by internals
typedef std::list<ptl_int_me_t*> me_list_t;
typedef std::list<ptl_int_trig_op_t*> trig_op_list_t;

typedef struct {
    bool allocated;
    ptl_ct_event_t ct_event;
    ptl_ct_type_t ct_type;
    trig_op_list_t trig_op_list;
} ptl_int_ct_t;

typedef struct {
    ptl_ct_event_t ct_event;
    ptl_handle_ct_t ct_handle;
} ptl_update_ct_event_t;
  
typedef struct {
    me_list_t* priority_list;
    me_list_t* overflow;
} ptl_entry_t;

// Data structure to pass stuff to the NIC
typedef enum {
    PTL_NO_OP, PTL_DMA, PTL_DMA_RESPONSE,
    PTL_CREDIT_RETURN,
    PTL_NIC_PROCESS_MSG,
    PTL_NIC_ME_APPEND, PTL_NIC_TRIG,
    PTL_NIC_PROCESS_TRIG, PTL_NIC_POST_CT,
    PTL_NIC_CT_SET, PTL_NIC_CT_INC,
    PTL_NIC_UPDATE_CPU_CT, PTL_NIC_INIT_FOR_SEND_RECV
} ptl_int_nic_op_type_t;


typedef struct {
    ptl_int_nic_op_type_t op_type;
    union {
	ptl_int_me_t* me;
	ptl_int_trig_op_t* trig;
        ptl_update_ct_event_t* ct;
	ptl_handle_ct_t ct_handle;
	ptl_int_dma_t* dma;
    } data;
} ptl_int_nic_op_t;


// defines to put flags in the header flit of the packet
#define PTL_HDR_PORTALS     0x1
#define PTL_HDR_HEAD_PACKET 0x2
#define PTL_HDR_STREAM_PIO  0x10000000
#define PTL_HDR_STREAM_DMA  0x20000000
#define PTL_HDR_STREAM_TRIG 0x30000000
#define PTL_HDR_STREAM_GET  0x40000000

#endif // COMPONENTS_TRIG_CPU_PORTALS_TYPES_H
