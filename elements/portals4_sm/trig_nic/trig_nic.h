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


#ifndef COMPONENTS_TRIG_NIC_H
#define COMPONENTS_TRIG_NIC_H

#include <queue>
#include <map>

#include "elements/SS_router/SS_router/RtrIF.h"
#include "elements/portals4_sm/trig_cpu/portals_types.h"
#include "trig_nic_event.h"

using namespace SST;

struct MessageStream {
    void* start;
    int current_offset;
    int remaining_length;
    ptl_handle_ct_t ct_handle;
};

class trig_nic : public RtrIF {

private:

    Link*                   cpu_link;
    // Direct link where we send packets destined for the router
    Link*                   self_link;
    // Indirect link where we send packets going to the portals
    // offload unit
    Link*                   ptl_link;
    Link*                   dma_link;

    int                     msg_latency;
    int                     ptl_latency;
    int                     ptl_msg_latency;
    int                     rtr_q_size;
    int                     rtr_q_max_size;

    int                     timing_set;
    int                     latency_ct_post;
    int                     latency_ct_host_update;
    int                     ptl_unit_latency;
    
    // Data structures to support portals on NIC
    #define MAX_PORTAL_TABLE_ENTRY 32
    ptl_entry_t* ptl_table[MAX_PORTAL_TABLE_ENTRY];

    #define MAX_CT_EVENTS 32
    ptl_int_ct_t ptl_ct_events[MAX_CT_EVENTS];

    std::queue<ptl_int_trig_op_t*> already_triggered_q;

    // Two queues to handle events coming from the CPU.  On is for
    // PIO, the other for DMA.
    std::queue<trig_nic_event*> pio_q;
    std::queue<trig_nic_event*> dma_q;
    std::queue<ptl_int_dma_t*> dma_req_q;
    std::queue<trig_nic_event*> dma_hdr_q;
    // Here's the maximum size of the dma_q.  We need this so we know
    // when we have room to issue more DMAs.  PIO_Q doesn't need one
    // because we are doing credits from the host, which mimicks
    // having a max size for the PIO_Q, but we'll never check that
    // here.
    int dma_q_max_size;

    std::map<int,MessageStream*> streams;

    ptl_int_dma_t* dma_req;
    bool dma_in_progress;

    bool rr_dma;
    bool new_dma;
    bool send_recv;
    
public:
    trig_nic( ComponentId_t id, Params_t& params );

    virtual int Finish() {
        return 0;
    }

    virtual int Setup() {
        return 0;
    }

private:

    // Next event to go to the router
    RtrEvent* nextToRtr;

    bool clock_handler(Cycle_t cycle);
    bool processCPUEvent( Event* e);
    bool processPtlEvent( Event* e);
    bool processDMAEvent( Event* e);
    void setTimingParams(int set);

    inline bool PtlCTCheckThresh(ptl_handle_ct_t ct_handle, ptl_size_t test) {
	if ( (ptl_ct_events[ct_handle].ct_event.success +
	      ptl_ct_events[ct_handle].ct_event.failure ) >= test ) {
// 	    printf("%5d: PtlCTCheckThresh(): success = %d failure = %d ct_handle = %d\n",
// 		   m_id,ptl_ct_events[ct_handle].ct_event.success,
// 		   ptl_ct_events[ct_handle].ct_event.failure,ct_handle);
	    return true;
	}
	return false;
	
    }

    void scheduleUpdateHostCT(ptl_handle_ct_t ct_handle);
    void scheduleCTInc(ptl_handle_ct_t ct_handle);
    
};

#endif
