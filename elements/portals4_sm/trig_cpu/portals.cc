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


#include "portals.h"
#include "trig_cpu.h"

#include "elements/portals4_sm/trig_nic/trig_nic_event.h"

using namespace SST;
using namespace std;

portals::portals(trig_cpu* my_cpu) {
    // For now initialize only one portal table entry
    ptl_entry_t* entry = new ptl_entry_t();
    ptl_table[0] = entry;
    entry->priority_list = new me_list_t;

    entry = new ptl_entry_t();
    ptl_table[1] = entry;
    entry->priority_list = new me_list_t;

    entry = new ptl_entry_t();
    ptl_table[2] = entry;
    entry->priority_list = new me_list_t;

    entry = new ptl_entry_t();
    ptl_table[3] = entry;
  
    entry->priority_list = new me_list_t;

    for (int i = 0; i < MAX_CT_EVENTS; i++ ) {
	ptl_ct_cpu_events[i].allocated = false;
    }  

    next_handle_me = 0;

    cpu = my_cpu;
}


// Seems to work
void
portals::PtlMEAppend(ptl_pt_index_t pt_index, ptl_me_t me, ptl_list_t ptl_list, 
			  void *user_ptr, ptl_handle_me_t& me_handle)
{

    //   if ( cpu->my_id == 0 || cpu->my_id == 992 ) {
    //     printf("%5d: PtlMEAppend(%d) @ %lu\n",cpu->my_id,pt_index,cpu->getCurrentSimTimeNano());
    //   }
    // The ME actually gets appended on the NIC, so all we do is
    // create the ptl_int_me_t datatype and send it the NIC
    
    ptl_int_me_t* int_me = new ptl_int_me_t();
    int_me->me = me;
    int_me->active = true;
    int_me->user_ptr = user_ptr;
    int_me->handle_ct = PTL_CT_NONE;
    int_me->pt_index = pt_index;
    int_me->ptl_list = ptl_list;

    // Send this to the NIC
    trig_nic_event *event = new trig_nic_event;
    event->src = cpu->my_id;
    event->ptl_op = PTL_NIC_ME_APPEND;
//     event->operation = new ptl_int_nic_op_t;
//     event->operation->op_type = PTL_NIC_ME_APPEND;
//     event->operation->data.me = int_me;
    event->ptl_op = PTL_NIC_ME_APPEND;
    event->data.me = int_me;

//     cpu->nic->Send(event);
    cpu->writeToNIC(event);
    // The processor is tied up for 1/msgrate
//     cpu->busy += cpu->msg_rate_delay;
    cpu->busy += cpu->delay_host_pio_write;
    
    
    me_handle = int_me;
    return;
}

// FIXME: Need to fix this, can't do it this way anymore since the ME
// is on the NIC
void
portals::PtlMEUnlink(ptl_handle_me_t me_handle) {
    //   ptl_int_me_t* int_me = me_map[me_handle];
    //   me_map.erase(me_handle);
    //   // For now ignore ptl_list
    //   // Need to delete from the list as well.  NOT YET DONE
    // //   me_list_t list = ptl_table[int_me->pt_index]->priority_list;
    me_handle->active = false;
}


// Works for all message sizes
void
portals::PtlPut ( ptl_handle_md_t md_handle, ptl_size_t local_offset, 
		       ptl_size_t length, ptl_ack_req_t ack_req, 
		       ptl_process_id_t target_id, ptl_pt_index_t pt_index,
		       ptl_match_bits_t match_bits, ptl_size_t remote_offset, 
		       void *user_ptr, ptl_hdr_data_t hdr_data)
{
    
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->dest = target_id;
    event->ptl_op = PTL_NO_OP;
    
    event->portals = true;
    event->latency = cpu->latency/2;
    event->head_packet = true;
    
    ptl_header_t header;
    header.pt_index = pt_index;
    header.op = PTL_OP_PUT;
    header.length = length;
    header.match_bits = match_bits;
    header.remote_offset = remote_offset;
    
    // Copy the header into the first half of the packet payload
    memcpy(event->ptl_data,&header,sizeof(ptl_header_t));

    // If this is a single packet message, just send it.  Otherwise,
    // we need to set up a multi-packet PIO or DMA (for now just PIO.
    if ( length <= 32 ) { // Single packet
	// Copy over the payload, we'll reserve 32-bytes for the
	// ptl_header.  If it grows too big, the sim will abort to warn
	// us.
	event->stream = PTL_HDR_STREAM_PIO;
	memcpy(&event->ptl_data[8],(void*)((unsigned long)md_handle->start+(unsigned long)local_offset),length);
// 	if ( cpu->my_id == 0) printf("Writing packet to BUS @ %llu\n",cpu->getCurrentSimTimeNano());
      	cpu->writeToNIC(event);
// 	cpu->busy += cpu->msg_rate_delay;
	cpu->busy += cpu->delay_host_pio_write;

	// Need to send a CTInc if there is a ct_handle specified
	if ( md_handle->ct_handle != PTL_CT_NONE ) {
	    // Treat it like a PIO, but set the length the 0 and all
	    // that will happen is the CTInc will be sent
  	    cpu->pio_in_progress = true;
	    pio_length_rem = 0;
	    pio_ct_handle = md_handle->ct_handle;
	}
    }
    else if ( length <= 2048 /*100000000*/ ) {  // PIO
// 	if ( cpu->my_id == 0 ) printf("Starting PIO\n");
	memcpy(&event->ptl_data[8],(void*)((unsigned long)md_handle->start+(unsigned long)local_offset),32);
	cpu->pio_in_progress = true;

	pio_start = md_handle->start;
	pio_current_offset = 32;
	pio_length_rem = length - 32;
	pio_dest = target_id;
	pio_ct_handle = md_handle->ct_handle;
	
	event->stream = PTL_HDR_STREAM_PIO;
// 	printf("%5d: Writing PIO <= 64 bytes to NIC\n",cpu->my_id);
	cpu->writeToNIC(event);
	// FIXME.  Needs to match rate of interface (serialization delay)
	cpu->busy += cpu->delay_host_pio_write;
    }
    else {  // DMA
// 	if ( cpu->my_id == 0 ) printf("Starting DMA\n");
	ptl_int_dma_t* dma_req = new ptl_int_dma_t;

	// FIXME: Cheat for now, push header, data and DMA request all
	// at once.  This should really take two transfers.  Should
	// fix this.
	memcpy(&event->ptl_data[8],(void*)((unsigned long)md_handle->start+(unsigned long)local_offset),32);

	// Change event type to DMA
	event->ptl_op = PTL_DMA;
	event->stream = PTL_HDR_STREAM_DMA;

	// Fill in dma structure
	dma_req->start = md_handle->start;
	dma_req->length = length - 32;
	dma_req->offset = local_offset + 32;
	dma_req->target_id = target_id;
	dma_req->ct_handle = md_handle->ct_handle;
	dma_req->stream = PTL_HDR_STREAM_DMA;
	
// 	event->operation = new ptl_int_nic_op_t;
// 	event->operation->data.dma = dma_req;
	event->data.dma = dma_req;

	cpu->writeToNIC(event);
	cpu->busy += cpu->delay_host_pio_write;
	
    }
  
}

bool
portals::progressPIO(void)
{
//   printf("portals::progressPIO\n");
  if ( pio_length_rem == 0 && pio_ct_handle != PTL_CT_NONE) {
//     printf("%5d: Sending a CTInc to %d at end of PIO\n",cpu->my_id,pio_ct_handle);
	// Need to send an increment to the CT attached to the MD
	trig_nic_event* event = new trig_nic_event;
	event->src = cpu->my_id;
	event->ptl_op = PTL_NIC_CT_INC;
// 	event->operation = new ptl_int_nic_op_t;
// 	event->operation->data.ct_handle = pio_ct_handle;
// 	event->operation->op_type = PTL_NIC_CT_INC;
	event->data.ct_handle = pio_ct_handle;
	event->ptl_op = PTL_NIC_CT_INC;
	cpu->writeToNIC(event);
 	cpu->busy += cpu->delay_host_pio_write;
	cpu->pio_in_progress = false;
	return true;
    }
    
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->dest = pio_dest;
    event->ptl_op = PTL_NO_OP;

    event->portals = true;
    event->head_packet = false;
    event->stream = PTL_HDR_STREAM_PIO;

    int copy_length = pio_length_rem < 64 ? pio_length_rem : 64; 
    memcpy(event->ptl_data,(void*)((unsigned long)pio_start+(unsigned long)pio_current_offset),copy_length);
    cpu->writeToNIC(event);

    // Delay is just serialization delay at this point
    cpu->busy += 16;
    //     cpu->busy += cpu->delay_host_pio_write;
    
    pio_length_rem -= copy_length;
    pio_current_offset += copy_length;

    if ( pio_length_rem == 0 && pio_ct_handle == PTL_CT_NONE ) {
      cpu->pio_in_progress = false;
      return true;
    }
    return false;
}

// FIXME:  Not working with NIC offload yet
void
portals::PtlAtomic(ptl_handle_md_t md_handle, ptl_size_t local_offset,
			ptl_size_t length, ptl_ack_req_t ack_req,
			ptl_process_id_t target_id, ptl_pt_index_t pt_index,
			ptl_match_bits_t match_bits, ptl_size_t remote_offset,
			void *user_ptr, ptl_hdr_data_t hdr_data,
			ptl_op_t operation, ptl_datatype_t datatype) {
    
    //   if ( cpu->my_id == 0 ) {
    //     printf("%5d:  PtlAtomic @ %lu to %d\n",cpu->my_id,cpu->getCurrentSimTimeNano()-cpu->start_time,target_id);
    //   }
    // Ignoring most of the parameters.  No actual data movement and
    // only faking 8-byte moves.

    // For now, only doing persistent all-wildcard me, so don't need
    // to send much of the information.  Do need to send the
    // following:
    // target_id, pt_index

    
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->dest = target_id;
    event->ptl_op = PTL_NO_OP;

    event->portals = true;
    event->latency = cpu->latency/2;

    event->ptl_data[0] = pt_index;
    event->ptl_data[1] = PTL_OP_ATOMIC;
    //     event->ptl_data[2] = match_bits;
    event->ptl_data[2] = match_bits & 0xffffffff;
    event->ptl_data[3] = (match_bits >> 32) & 0xffffffff;

//     cpu->nic->Send(cpu->busy,event);
    cpu->writeToNIC(event);
    cpu->busy += cpu->msg_rate_delay;
}


// Counting event methods

// Seems to work
void
portals::PtlCTAlloc(ptl_ct_type_t ct_type, ptl_handle_ct_t& ct_handle) {
    // Find the first counter that is free
    for (int i = 0; i < MAX_CT_EVENTS; i++ ) {
        if ( !ptl_ct_cpu_events[i].allocated ) {
// 	  printf("found a free ct\n");
	  ct_handle = i;
            ptl_ct_cpu_events[i].allocated = true;
            ptl_ct_cpu_events[i].ct_type = ct_type;
            ptl_ct_cpu_events[i].ct_event.success = 0;
            ptl_ct_cpu_events[i].ct_event.failure = 0;

	    // Need to send a message to the NIC to clear the counter
	    trig_nic_event* event = new trig_nic_event;
	    event->ptl_op = PTL_NIC_CT_SET;
	    event->ptl_data[0] = i;  // ct_handle
	    event->ptl_data[1] = 0;  // success value
	    event->ptl_data[2] = 0;  // failure value
	    event->ptl_data[3] = true;  // Clear op_list
// 	    printf("sending message to nic\n");
	    cpu->writeToNIC(event);
	    cpu->busy += cpu->delay_host_pio_write;
	    
            break;
        }
    }
}

// FIXME: Leaves dangling resources (though they will be cleaned up if
// the counter is reallocated.
void
portals::PtlCTFree(ptl_handle_ct_t ct_handle) {
    // Should check to make sure there are not operations posted to the
    // event, but not going to for now.  OK, the spec says that is user
    // responsibility, so I'll just delete everything out of the list.
    ptl_ct_cpu_events[ct_handle].allocated = false;
//     ptl_ct_events[ct_handle].trig_op_list.clear(); 
}

void
portals::PtlCTGet(ptl_handle_ct_t ct_handle, ptl_ct_event_t *ev)
{
    ev->success = ptl_ct_cpu_events[ct_handle].ct_event.success;
    ev->failure = ptl_ct_cpu_events[ct_handle].ct_event.failure;
}

bool
portals::PtlCTWait(ptl_handle_ct_t ct_handle, ptl_size_t test) {
//   printf("%5d: PTLCTWait: %d, %d\n",cpu->my_id,test,ptl_ct_cpu_events[ct_handle].ct_event.success);
    if ( (ptl_ct_cpu_events[ct_handle].ct_event.success +
          ptl_ct_cpu_events[ct_handle].ct_event.failure ) >= test ) {
        cpu->waiting = false;
	return true;
    }
//     printf("waiting...\n");
    cpu->waiting = true;
    return false;
}

bool
portals::PtlCTCheckThresh(ptl_handle_ct_t ct_handle, ptl_size_t test) {
//     if ( (ptl_ct_events[ct_handle].ct_event.success +
//           ptl_ct_events[ct_handle].ct_event.failure ) >= test ) {
//         return true;
//     }
    return false;
}


// Seems to work
void
portals::PtlTriggeredPut( ptl_handle_md_t md_handle, ptl_size_t local_offset, 
			  ptl_size_t length, ptl_ack_req_t ack_req, 
			  ptl_process_id_t target_id, ptl_pt_index_t pt_index,
			  ptl_match_bits_t match_bits, ptl_size_t remote_offset, 
			  void *user_ptr, ptl_hdr_data_t hdr_data,
			  ptl_handle_ct_t trig_ct_handle, ptl_size_t threshold) {

    // Create an op structure
    ptl_int_op_t* op = new ptl_int_op_t;
    op->op_type = PTL_OP_PUT;
    op->target_id = target_id;
    op->pt_index = pt_index;
    op->match_bits = match_bits;

    // Add the dma request that is used when triggered
    op->dma = new ptl_int_dma_t;
    op->dma->start = md_handle->start;
    op->dma->length = length;
    op->dma->offset = local_offset;
    op->dma->target_id = target_id;
    op->dma->stream = PTL_HDR_STREAM_TRIG;
    op->dma->ct_handle = md_handle->ct_handle;
    
    // Add the header which will be sent when triggered
    op->ptl_header = new ptl_header_t;
    op->ptl_header->pt_index = pt_index;
    op->ptl_header->op = PTL_OP_PUT;
    op->ptl_header->length = length;
    op->ptl_header->match_bits = match_bits;
    op->ptl_header->remote_offset = remote_offset;

    
    // Create a triggered op structure
    ptl_int_trig_op_t* trig_op = new ptl_int_trig_op_t;
    trig_op->op = op;
    trig_op->trig_ct_handle = trig_ct_handle;
    trig_op->threshold = threshold;

    // Need to send this object to the NIC with latency = 30% latency
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->ptl_op = PTL_NIC_TRIG;
//     event->operation = new ptl_int_nic_op_t;
//     event->operation->op_type = PTL_NIC_TRIG;
//     event->operation->data.trig = trig_op;
    event->ptl_op = PTL_NIC_TRIG;
    event->data.trig = trig_op;
    //     cpu->nic->Send(event);
    cpu->writeToNIC(event);
    
    // The processor is tied up for 1/msgrate
    //     if (cpu->my_id == 0 ) {
    // 	printf("%5d: msg_rate_delay = %d, busy = %d\n",cpu->my_id,cpu->msg_rate_delay,cpu->busy);
    //     }
    cpu->busy += cpu->delay_host_pio_write;

    //     printf("%d:  posting triggered put\n",cpu->my_id);
    //     ptl_ct_events[trig_ct_handle].trig_op_list.push_back(trig_op);
    
}

void
portals::PtlTriggeredAtomic(ptl_handle_md_t md_handle, ptl_size_t local_offset,
			    ptl_size_t length, ptl_ack_req_t ack_req,
			    ptl_process_id_t target_id, ptl_pt_index_t pt_index,
			    ptl_match_bits_t match_bits, ptl_size_t remote_offset,
			    void *user_ptr, ptl_hdr_data_t hdr_data,
			    ptl_op_t operation, ptl_datatype_t datatype,
			    ptl_handle_ct_t trig_ct_handle, ptl_size_t threshold) {

    //   if ( cpu->my_id == 0 )
    //     printf("%5d:  PtlTriggeredAtomic @ %lu to %d\n",cpu->my_id,cpu->getCurrentSimTimeNano()-cpu->start_time,target_id);
    //     if ( cpu->my_id == 0 ) {
    // 	printf("%5d:  PostingTriggeredAtomic(%d,%d,%d,%d) @ %lu\n",cpu->my_id,target_id,pt_index,trig_ct_handle,threshold,
    // 	       cpu->getCurrentSimTimeNano());
    //     }
    // Create an op structure
    ptl_int_op_t* op = new ptl_int_op_t;
    op->op_type = PTL_OP_ATOMIC;
    op->target_id = target_id;
    op->pt_index = pt_index;
    op->match_bits = match_bits;

    // Create a triggered op structure
    ptl_int_trig_op_t* trig_op = new ptl_int_trig_op_t;
    trig_op->op = op;
    trig_op->trig_ct_handle = trig_ct_handle;
    trig_op->threshold = threshold;

    
    // Need to send this object to the NIC with latency = 30% latency
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->ptl_op = PTL_NIC_TRIG;
//     event->operation = new ptl_int_nic_op_t;
//     event->operation->op_type = PTL_NIC_TRIG;
//     event->operation->data.trig = trig_op;
    event->ptl_op = PTL_NIC_TRIG;
    event->data.trig = trig_op;
    cpu->nic->Send(event);
    
    // The processor is tied up for 1/msgrate
    cpu->busy += cpu->delay_host_pio_write;

    //     printf("%d:  posting triggered atomic\n",cpu->my_id);
    //     ptl_ct_events[trig_ct_handle].trig_op_list.push_back(trig_op);
}

void
portals::PtlCTInc(ptl_handle_ct_t ct_handle, /*ptl_ct_event_t*/ ptl_size_t increment) {
    // Need to send an increment to the CT attached to the MD
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->ptl_op = PTL_NIC_CT_INC;
    cpu->writeToNIC(event);
}

void
portals::PtlTriggeredCTInc(ptl_handle_ct_t ct_handle, ptl_size_t increment,
			   ptl_handle_ct_t trig_ct_handle, ptl_size_t threshold) {
    
    // Create an op structure
    ptl_int_op_t* op = new ptl_int_op_t;
    op->op_type = PTL_OP_CT_INC;
    op->ct_handle = ct_handle;
    op->increment = increment;

    // Create a triggered op structure
    ptl_int_trig_op_t* trig_op = new ptl_int_trig_op_t;
    trig_op->op = op;
    trig_op->trig_ct_handle = trig_ct_handle;
    trig_op->threshold = threshold;

    // Need to send this object to the NIC with latency = 30% latency
    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->ptl_op = PTL_NIC_TRIG;
//     event->operation = new ptl_int_nic_op_t;
//     event->operation->op_type = PTL_NIC_TRIG;
//     event->operation->data.trig = trig_op;
    event->ptl_op = PTL_NIC_TRIG;
    event->data.trig = trig_op;
    cpu->nic->Send(event);
    
    // The processor is tied up for 1/msgrate
    cpu->busy += cpu->delay_host_pio_write;
}

void
portals::PtlMDBind(ptl_md_t md, ptl_handle_md_t *md_handle)
{
    // Need to copy the information to a new md that we keep around
    ptl_md_t* new_md = new ptl_md_t(md);
    *md_handle = new_md;

    // Add some busy time
    cpu->busy += cpu->defaultTimeBase->convertFromCoreTime(cpu->registerTimeBase("100ns", false)->getFactor());
}

void
portals::PtlMDRelease(ptl_handle_md_t md_handle)
{
    delete md_handle;
}

void
portals::PtlGet ( ptl_handle_md_t md_handle, ptl_size_t local_offset, 
		  ptl_size_t length, ptl_process_id_t target_id, 
		  ptl_pt_index_t pt_index, ptl_match_bits_t match_bits, 
		  void *user_ptr, ptl_size_t remote_offset ) {

    trig_nic_event* event = new trig_nic_event;
    event->src = cpu->my_id;
    event->dest = target_id;
    event->ptl_op = PTL_NO_OP;
    
    event->portals = true;
    event->latency = cpu->latency/2;
    event->head_packet = true;
    
    ptl_header_t header;
    header.pt_index = pt_index;
    header.op = PTL_OP_GET;
    header.length = length;
    header.match_bits = match_bits;
    header.remote_offset = remote_offset;
    header.get_ct_handle = md_handle->ct_handle;
    header.get_start = (void*)((unsigned long)md_handle->start+(unsigned long)local_offset);
    
    // Copy the header into the first half of the packet payload
    memcpy(event->ptl_data,&header,sizeof(ptl_header_t));

    cpu->writeToNIC(event);
}

void
portals::PtlTriggeredGet ( ptl_handle_md_t md_handle, ptl_size_t local_offset, 
			   ptl_size_t length, ptl_process_id_t target_id, 
			   ptl_pt_index_t pt_index, ptl_match_bits_t match_bits, 
			   void *user_ptr, ptl_size_t remote_offset,
			   ptl_handle_ct_t ct_handle, ptl_size_t threshold) {
  if ( length >0x80000000 ) {
    printf("%5d: Bad length passed into PtlTriggeredGet(): %d\n",cpu->my_id,length);
    abort();
  }

  // Create an op structure
    ptl_int_op_t* op = new ptl_int_op_t;
    op->op_type = PTL_OP_GET;
    op->target_id = target_id;
    op->pt_index = pt_index;
    op->match_bits = match_bits;

    // Add the header which will be sent when triggered
    op->ptl_header = new ptl_header_t;
    op->ptl_header->pt_index = pt_index;
    op->ptl_header->op = PTL_OP_GET;
    op->ptl_header->length = length;
//     printf("ptltriggeredget: return header length = %d\n",length);
    op->ptl_header->match_bits = match_bits;
    op->ptl_header->remote_offset = remote_offset;    
    op->ptl_header->get_ct_handle = md_handle->ct_handle;
    op->ptl_header->get_start = (void*)((unsigned long)md_handle->start+(unsigned long)local_offset);

    // Create a triggered op structure
    ptl_int_trig_op_t* trig_op = new ptl_int_trig_op_t;
    trig_op->op = op;
    trig_op->trig_ct_handle = ct_handle;
    trig_op->threshold = threshold;
    
    // Need to send the objectto the NIC
    trig_nic_event* event = new trig_nic_event;
    event->portals = true;
    event->head_packet = true;
    event->src = cpu->my_id;
    event->dest = target_id;
    event->ptl_op = PTL_NIC_TRIG;
//     event->operation = new ptl_int_nic_op_t;
//     event->operation->op_type = PTL_NIC_TRIG;
//     event->operation->data.trig = trig_op;
    event->ptl_op = PTL_NIC_TRIG;
    event->data.trig = trig_op;
        
    cpu->writeToNIC(event);
}



void
portals::scheduleUpdateHostCT(ptl_handle_ct_t ct_handle) {
}

// This is where incoming messages are processed
// bool portals::processMessage(int src, uint32_t* ptl_data) {
bool
portals::processMessage(trig_nic_event* ev) {
//      printf("portals::processMessage()\n");

    // May be returning credits
    if ( ev->ptl_op == PTL_CREDIT_RETURN ) {
      cpu->returnCredits(ev->data_length);
	delete ev;
    }
    else if ( ev->ptl_op == PTL_NIC_UPDATE_CPU_CT ) {
      ptl_ct_cpu_events[ev->data.ct->ct_handle].ct_event.success = 
	    ev->data.ct->ct_event.success;
	ptl_ct_cpu_events[ev->data.ct->ct_handle].ct_event.failure = 
	    ev->data.ct->ct_event.failure;
	delete ev->data.ct;
	delete ev;
	if ( cpu->waiting ) {
// 	    printf("Calling wakeup @ %llu\n",cpu->getCurrentSimTimeNano());
	    // Need to wakeup the CPU;
	    cpu->wakeUp();
	}
    }
    else if ( ev->ptl_op == PTL_DMA_RESPONSE ) {
      // Fill data into the request and send it back
	void* start = ev->data.dma->start;
	ptl_size_t offset = ev->data.dma->offset;
	ptl_size_t length = ev->data.dma->length;
	memcpy(ev->ptl_data,(void*)((unsigned long)start+(unsigned long)offset),length);
	
	// Fill in the rest of the event
	ev->src = cpu->my_id;
	ev->dest = ev->data.dma->target_id;
	ev->portals = true;
	ev->head_packet = false;
	
	// Need to add latency to memory
	cpu->dma_return_link->Send(cpu->latency_dma_mem_access,ev);
    }
    else {
      // In the case of send/recv (cpu->use_portals == false), we need
      // to just put the data into the received messages buffer.
      if ( cpu->use_portals ) {
	// The list is on the NIC, so all we get here is requests to put
	// data in memory.
	memcpy(ev->start,ev->ptl_data,ev->data_length);
	delete ev;
      }
      else {
	cpu->pending_msg.push(ev);
	if ( cpu->waiting )
	  cpu->wakeUp();
      }
    }

    return false;
    
}

