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


// #include <sys/stat.h>
// #include <fcntl.h>
#include "sst_config.h"

#include "trig_nic.h"

using namespace SST;

trig_nic::trig_nic( ComponentId_t id, Params_t& params ) :
    RtrIF(id,params),
    ptl_latency(20),
    ptl_msg_latency(10),
    msg_latency(40),
    rtr_q_size(0),
    rtr_q_max_size(4),
    dma_in_progress(false),
    rr_dma(false),
    new_dma(true),
    latency_ct_post(10),
    latency_ct_host_update(20)
{

    ClockHandler_t*  clockHandler = new EventHandler< trig_nic, bool, Cycle_t >
        ( this, &trig_nic::clock_handler );

    if ( ! registerClock( frequency, clockHandler ) ) {
        _abort(trig_nic,"couldn't register clock handler");
    }
 
    if ( params.find("latency") == params.end() ) {
	_abort(trig_nic,"couldn't find NIC latency\n");
    }
//     msg_latency = str2long( params[ "latency" ] ) / 2;

    if ( params.find("timing_set") == params.end() ) {
	_abort(trig_nic,"couldn't find timing set\n");
    }
    timing_set = strtol( params[ "timing_set" ].c_str(), NULL, 0 );

    setTimingParams(timing_set);

    nextToRtr = NULL;

    // Set up the link to the CPU
    Event::Handler_t* handler = new EventHandler<
    trig_nic, bool, Event* >
	( this, &trig_nic::processCPUEvent );
    
    cpu_link = LinkAdd( "cpu", handler );
    // End link to CPU setup

    // Setup a direct self link for messages headed to the router
    self_link = selfLink( "self");
    
    // Set up the portals link to delay incoming commands the
    // appropriate time.
    handler = new EventHandler<
	trig_nic, bool, Event * >
	(this, &trig_nic::processPtlEvent );

    ptl_link = selfLink("self_ptl", handler);
    // End portals link setup

    // Set up the dma link
    handler = new EventHandler<
	trig_nic, bool, Event * >
	(this, &trig_nic::processDMAEvent );

    dma_link = selfLink("self_dma", handler);
    // End dma link setup
    
    // The links will use 1 ns as their timebase
    cpu_link->setDefaultTimeBase(registerTimeBase("1ns",false));
    self_link->setDefaultTimeBase(registerTimeBase("1ns",false));
    ptl_link->setDefaultTimeBase(registerTimeBase("1ns",false));
    dma_link->setDefaultTimeBase(registerTimeBase("1ns",false));

    // Initialize all the portals elements

    // Portals table entries
    // For now initialize four portal table entries
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
	ptl_ct_events[i].allocated = false;
    }  
    send_recv = false;
}


bool trig_nic::clock_handler ( Cycle_t cycle ) {
//     printf("clock_handler()\n");
    // All we do is look at the two queues and copy stuff across
    if ( !toNicQ_empty( 0 ) ) {
        RtrEvent* event = toNicQ_front( 0 );
        toNicQ_pop( 0 );

// 	if (m_id == 1) printf("Received packet @ %llu\n",getCurrentSimTimeNano());
	
	// OK, need to figure out how to route this.  If this is a
	// portals packet, it needs to go to the portals unit.  If
	// not, it goes straight to the host.

	// The first entry tells us if this a portals packet, which will
	// change the latency through the NIC
        bool portals = event->u.packet.payload[0] & PTL_HDR_PORTALS;
        bool head_packet = event->u.packet.payload[0] & PTL_HDR_HEAD_PACKET;

	// Either way we have to create a trig_nic_event, so do that
	// now.
	trig_nic_event* nic_event = new trig_nic_event();
	nic_event->src = event->u.packet.srcNum();
	nic_event->dest = event->u.packet.destNum();
	nic_event->ptl_op = PTL_NIC_PROCESS_MSG;
	nic_event->portals = portals;
	nic_event->head_packet = head_packet;
	
	nic_event->stream = event->u.packet.payload[1];
	
	// Copy over the payload
	memcpy(nic_event->ptl_data,&event->u.packet.payload[2],16*sizeof(uint32_t));
	
	delete event;

	if (portals) {
	    ptl_link->Send(ptl_unit_latency,nic_event);
	}
	else {
	    cpu_link->Send(portals ? 0 : msg_latency,nic_event);
	}
    }  
    // Need to send out any packets that need to go to the network.
    // We'll round robin the dma and pio to the network.
    bool adv_pio = false;
    bool adv_dma = false;

    // FIXME.  Works, but needs to be optimized.  Figure out which to
    // advance.  If the pio is headed to the router than we can
    // advance the dma regardless.
    adv_dma = (pio_q.empty() || rr_dma) && !dma_q.empty() && (rtr_q_size < rtr_q_max_size);
    adv_pio = (dma_q.empty() || !rr_dma) && !pio_q.empty() && (rtr_q_size < rtr_q_max_size);

    rr_dma = !rr_dma;


    if ( adv_pio ) {
        trig_nic_event* ev = pio_q.front();
	if ( ev->ptl_op == PTL_NO_OP ) {
	    // This is a message destined for the router
	    // See if there is room in the output q
 	    if (rtr_q_size < rtr_q_max_size) {
		rtr_q_size++;
		pio_q.pop();

		self_link->Send(ptl_msg_latency,ev);

		// Need to return credits to the host
		trig_nic_event* event = new trig_nic_event;
		event->ptl_op = PTL_CREDIT_RETURN;
		event->data_length = 1;
		cpu_link->Send(10,event);
	    }
// 	    else {
// 		printf("qqq: %5d: no room in rtr queue, doing nothing\n",m_id);
// 	    }
	}
	else if ( ev->ptl_op == PTL_DMA ) {
	    // Start a DMA operation

	    // The header (and first 32 bytes of data) comes with the
	    // PIO packet.  I'm going to cheat a bit and just send
	    // this to the dma_q at let it be sent when it hits the
	    // front.  This maintains ordering, but makes the logic
	    // easier.  We also need to send the dma request to the
	    // dma_req_q.
	    pio_q.pop();

	    // Need to get the DMA structure from the event
	    bool empty = dma_req_q.empty();
// 	    dma_req_q.push(ev->operation->data.dma);
 	    dma_req_q.push(ev->data.dma);
	    if ( empty ) {
		dma_link->Send(1,NULL);
	    }
//  	    delete ev->operation;
	    
	    // Now clean up the event and send to dma_q
	    ev->ptl_op = PTL_NO_OP;
	    ev->portals = true;

	    dma_hdr_q.push(ev);
	    
// 		self_link->Send(ptl_msg_latency,ev);

	    // Need to return credits to the host
	    trig_nic_event* event = new trig_nic_event;
	    event->ptl_op = PTL_CREDIT_RETURN;
	    event->data_length = 1;
	    cpu_link->Send(10,event);

	}	    
	else {
	    // This is a portals command, send it to the "portals unit"
	    // through the ptl_link
	    pio_q.pop();
	    ptl_link->Send(ptl_latency,ev);

	    // Need to return credits to the host
	    trig_nic_event* event = new trig_nic_event;
	    event->ptl_op = PTL_CREDIT_RETURN;
	    event->data_length = 1;
	    cpu_link->Send(10,event);
	}
	
    }

    
    // Check to see if we have anything in the dma_q to send out
    if ( adv_dma ) {
      // See if there is room in the output q
      if (rtr_q_size < rtr_q_max_size) {
	rtr_q_size++;

	trig_nic_event* ev;
	// If new_dma is true, then we need to send the header first
	if ( new_dma ) {
	  if ( dma_hdr_q.empty() ) {
	    printf("There should be a dma header, but there isn't\n");
	    abort();
	  }
	  ev = dma_hdr_q.front();
	    dma_hdr_q.pop();
            ev->ptl_op = PTL_NO_OP;
	    self_link->Send(ptl_msg_latency,ev);
	    new_dma = false;
	}
	else {
	    ev = dma_q.front();
	    dma_q.pop();
            ev->ptl_op = PTL_NO_OP;

// 	    if ( ev->operation->data.dma->end ) {
	    if ( ev->data.dma->end ) {
	        // Since this is the end, the next one through will
	        // need to send it's header first
	        new_dma = true;
// 	        if ( ev->operation->data.dma->ct_handle != PTL_CT_NONE ) {
// 		  scheduleCTInc(ev->operation->data.dma->ct_handle);
// 		}
	        if ( ev->data.dma->ct_handle != PTL_CT_NONE ) {
		  scheduleCTInc(ev->data.dma->ct_handle);
		}
	    }
	    // Don't need the dma data structure any more
// 	    delete ev->operation->data.dma;
// 	    delete ev->operation;
 	    delete ev->data.dma;

	    self_link->Send(ptl_msg_latency,ev);
	}
	

	// If this is the end of the dma and there's a CT attached, we
	// need to increment the CT
	if ( !ev->head_packet ) {
	}
	

      }
    }

    
    // Check to see if something is ready to go out to the rtr, but only
    // if there isn't an event left over because the buffer was full
    if ( nextToRtr == NULL ) {
        trig_nic_event* to_rtr;
        if ( ( to_rtr = static_cast< trig_nic_event* >( self_link->Recv() ) ) ) {
// 	    printf("%5d:  Got something on my self link\n",m_id);
            nextToRtr = new RtrEvent();
            nextToRtr->type = RtrEvent::Packet;
            nextToRtr->u.packet.vc() = 0;
            nextToRtr->u.packet.srcNum() = m_id;
            nextToRtr->u.packet.destNum() = to_rtr->dest;
            nextToRtr->u.packet.sizeInFlits() = 8;

	    
	    nextToRtr->u.packet.payload[0] = 0;
            if (to_rtr->portals) nextToRtr->u.packet.payload[0] |= PTL_HDR_PORTALS;
            if (to_rtr->head_packet) nextToRtr->u.packet.payload[0] |= PTL_HDR_HEAD_PACKET;
	    nextToRtr->u.packet.payload[1] = to_rtr->stream;

// 	    printf("qqq: %5d:  Sending message to %d; head_packet = %d * %llu\n",
// 		   m_id,nextToRtr->u.packet.destNum(),to_rtr->head_packet,
// 		   getCurrentSimTimeNano());
	    memcpy(&nextToRtr->u.packet.payload[2],to_rtr->ptl_data,16*sizeof(uint32_t));
 	    delete to_rtr;
        }
    }
    if ( nextToRtr != NULL ) {
// 	printf("OK, really sending an event to router\n");
        if ( send2Rtr(nextToRtr) ) {
            // Successfully sent, so set nextToRtr to null so it will look
            // for new ones (otherwise, it will try to send the same one
            // again next time)
//  	  printf("Sent packet to router @ %llu\n",getCurrentSimTimeNano());
	  nextToRtr = NULL;
	    rtr_q_size--;
        }
    }
    return false;
}

bool trig_nic::processCPUEvent( Event *e) {
//     printf("processCPUEvent\n");
    trig_nic_event* ev = static_cast<trig_nic_event*>(e);

//      if (m_id == 0 && ev->ptl_op == PTL_NO_OP) printf("NIC received packet @ %llu\n",getCurrentSimTimeNano());
//     if (m_id == 0 ) printf("NIC received a %d type packet @ %llu\n",ev->ptl_op,getCurrentSimTimeNano());
    
    // For now, just seperate into the DMA_Q and PIO_Q.
    if ( ev->ptl_op == PTL_DMA_RESPONSE ) {
	dma_q.push(ev);
    }
    else {
	pio_q.push(ev);
    }
    
    return false;
}

void
trig_nic::setTimingParams(int set) {
    if ( set == 1 ) {
	ptl_msg_latency = 25;
	ptl_unit_latency = 50;
	latency_ct_post = 25;
	latency_ct_host_update = 25;
    }
    if ( set == 2 ) {
	ptl_msg_latency = 75;
	ptl_unit_latency = 150;
	latency_ct_post = 75;
	latency_ct_host_update = 25;
    }
    if ( set == 3 ) {
	ptl_msg_latency = 100;
	ptl_unit_latency = 200;
	latency_ct_post = 100;
	latency_ct_host_update = 25;
    }
}

bool trig_nic::processPtlEvent( Event *e ) {
//     printf("processPtlEvent\n");
    trig_nic_event* ev = static_cast<trig_nic_event*>(e);
//     ptl_int_nic_op_t* operation;
//     if ( ev->operation == NULL ) {
// 	operation = new ptl_int_nic_op_t;
// // 	printf("%5d: NULL operation in trig_nic_event, aborting!\n",m_id);
// // 	abort();
//     }
//     printf("Process event: %d\n",ev->ptl_op);
//     if (ev->ptl_op == PTL_NIC_ME_APPEND) printf("   ME Append\n");
    
    // See what portals command was sent
    switch (ev->ptl_op) {
	
    // Append an ME
    // TESTED
    case PTL_NIC_ME_APPEND:
	// Add the int_me structure to the priority list for the
	// portal table entry
// 	printf("accessing ptl_table[%d]\n",ev->data.me->pt_index);
// 	printf("%p\n",ev->data.me);
// 	ptl_table[operation->data.me->pt_index]->priority_list->push_back(operation->data.me);
      ptl_table[ev->data.me->pt_index]->priority_list->push_back(ev->data.me);
	delete ev;
	break;

    // TESTED
    case PTL_NIC_PROCESS_MSG:
    {
// 	printf("%5d: Got a message\n",m_id);
	if ( ev->head_packet ) {

	    // Need to walk the list and look for a match
	    ptl_header_t header;
	  
	    memcpy(&header,ev->ptl_data,sizeof(ptl_header_t));

	    // need to see if this is a get response, in which case it
	    // is not matched
	    bool found = false;
	    ptl_int_me_t* match_me;
	    if ( header.op == PTL_OP_GET_RESP ) {
		// There won't be any data in the head packet, so
		// just set up the stream entry
		MessageStream* ms = new MessageStream();
		ms->start = header.get_start;
		ms->current_offset = 0;
		ms->remaining_length = header.length;
		ms->ct_handle = header.get_ct_handle;
	      
		int map_key = ev->src | ev->stream;
	      
		streams[map_key] = ms;		  
		delete ev;
		return false;
	    }
	    else {
		me_list_t* list = ptl_table[header.pt_index]->priority_list;
	      
		me_list_t::iterator iter, curr;
		// Compute the match bits
		for ( iter = list->begin(); iter != list->end(); ) {
		    curr = iter++;
		    ptl_int_me_t* me = *curr;
		    // Check to see if me is still active, if not remove it
		    if ( !me->active ) {
			// delete the ME
			delete me;
			list->erase(curr);
			continue;
		    }
		    // Not implementing all the matching semantics.  For now, just
		    // check the match bits.  Oh, and everything is currently persistent
		    if ( (( header.match_bits ^ me->me.match_bits ) & ~me->me.ignore_bits ) == 0 ) {
			found = true;
			match_me = me;
			break;
		    }
		}
	    }

	    if ( found ) {

		if ( header.op == PTL_OP_GET ) {
// 		    printf("received a get operation length = %d\n",header.length);
		    // Need to create a return header and a dma_req
		    ptl_header_t ret_header;

		    ret_header.pt_index = header.pt_index;
		    ret_header.op = PTL_OP_GET_RESP;
// 		    printf("Assign return header.length = %d\n",header.length);
		    ret_header.length = header.length;
		    ret_header.get_ct_handle = header.get_ct_handle;
		    ret_header.get_start = header.get_start;
		  
		    trig_nic_event* event = new trig_nic_event;
		    event->src = m_id;
		    event->dest = ev->src;
		    event->portals = true;
		    event->head_packet = true;
		    event->stream = PTL_HDR_STREAM_GET;

		    memcpy(event->ptl_data,&ret_header,sizeof(ptl_header_t));

		    // Create DMA request
		    ptl_int_dma_t* dma = new ptl_int_dma_t;
		    dma->start = match_me->me.start;
		    dma->length = header.length;
		    dma->offset = header.remote_offset;
		    dma->target_id = ev->src;
		    dma->ct_handle = match_me->me.ct_handle;
		    dma->stream = PTL_HDR_STREAM_GET;
		  
		    // Now put both of these in the correct queues
		    dma_hdr_q.push(event);

		    bool empty = dma_req_q.empty();
		    dma_req_q.push(dma);
		    if ( empty ) {
			dma_link->Send(1,NULL);
		    }
		  
		    delete ev;

		}
		else {
	      
		    // Need to send the data to the cpu
		    int copy_length = header.length <= 32 ? header.length : 32;
		    ptl_size_t remote_offset = header.remote_offset;
		  
		    ev->data_length = copy_length;
		    ev->start = (void*)((unsigned long)match_me->me.start+(unsigned long)remote_offset);
		    // Fix up the data, i.e. move actual data to top
		    if ( !send_recv ) memcpy(ev->ptl_data,&ev->ptl_data[8],copy_length);
		    cpu_link->Send(ptl_msg_latency,ev);
		  
		    // Handle mulit-packet messages
		    if ( (ev->stream == PTL_HDR_STREAM_TRIG || ev->stream == PTL_HDR_STREAM_GET) && copy_length != 0) {
			// No data in head packet for triggered
			MessageStream* ms = new MessageStream();
			ms->start = (void*)((unsigned long)match_me->me.start+(unsigned long)remote_offset);
			ms->current_offset = 0;
			ms->remaining_length = header.length;
			ms->ct_handle = match_me->me.ct_handle;
		      
			int map_key = ev->src | ev->stream;
		      
			streams[map_key]= ms;
		      
		    }
		    else if ( header.length > 32 ) {
			MessageStream* ms = new MessageStream();
			ms->start = (void*)((unsigned long)match_me->me.start+(unsigned long)remote_offset);
			ms->current_offset = copy_length;
			ms->remaining_length = header.length - copy_length;
			ms->ct_handle = match_me->me.ct_handle;
		      
			int map_key = ev->src | ev->stream;
		      
			streams[map_key]= ms;
		    }
		    else {
// 			printf("Single packet message, scheduling CT Inc\n");
			// If this is not multi-packet, then we need to
			// update a CT if one is attached.
			scheduleCTInc( match_me->me.ct_handle );
		    }
	      
		}
	    }
	    else {
		printf("%d: Message arrived with no match in PT Entry %d @ %lu from %d\n",
		       m_id,header.pt_index,getCurrentSimTimeNano(),ev->src);
		abort();
	    }
	  
	}
	// Not a head packet, so we need to figure out where it goes.
	else {
	    // Can distinguish message streams by looking at src.
	    MessageStream* ms;
	    int map_key = ev->src | ev->stream;
	    if ( streams.find(map_key) == streams.end() ) {
		printf("%5d:  Received a packet for muiti-packet message but didn't get a corresponding head packet first: %x\n",
		       m_id,map_key);
		abort();
	    }
	    else {
		ms = streams[map_key];
	    }

	    // Reuse the same event, just forward in with the extra
	    // information
	    int copy_length = ms->remaining_length < 64 ? ms->remaining_length : 64;
	    ev->data_length = copy_length;
	    ev->start = (void*)((unsigned long)ms->start+(unsigned long)ms->current_offset);
	    cpu_link->Send(ptl_msg_latency,ev);

	    ms->current_offset += copy_length;
	    ms->remaining_length -= copy_length;

	    if ( ms->remaining_length == 0 ) {
		// Message is done, get rid of it.
		streams.erase(map_key);
		scheduleCTInc( ms->ct_handle );
		delete ms;
	    }
	  
	}
	break;
    }
      
    
    // Attach a triggered operation
    case PTL_NIC_TRIG:
        // Need to see if event should fire immediately
	if ( PtlCTCheckThresh(ev->data.trig->trig_ct_handle,ev->data.trig->threshold) ) {
	    // Put the operation in the already_triggered_q.  If it
	    // was empty, we also need to send an event to wake it up.
	    bool empty = already_triggered_q.empty();
// 	    already_triggered_q.push(operation->data.trig);
 	    already_triggered_q.push(ev->data.trig);
	    if (empty) {
		// Repurpose the event we got.  Just need to wake up
		// again to process event
		ev->ptl_op = PTL_NIC_PROCESS_TRIG;
// 		operation->op_type = PTL_NIC_PROCESS_TRIG;
		ptl_link->Send(1,ev);
	    }
	    else {
		// Done with the op passed into the function, so delete it
		delete ev;
	    }
	}
	// Doesn't fire immediately, just add to counter
	else {
	    ptl_ct_events[ev->data.trig->trig_ct_handle].trig_op_list.push_back(ev->data.trig);
// 	    delete operation;
	    delete ev;
	}
	break;

    case PTL_NIC_PROCESS_TRIG:
	{ // Need because we declare a variable here
            // Get the head of the queue and do whatever it says to
            ptl_int_trig_op_t* op = already_triggered_q.front();
            already_triggered_q.pop();
	
            if ( op->op->op_type == PTL_OP_PUT || op->op->op_type == PTL_OP_ATOMIC ) {
                trig_nic_event* event = new trig_nic_event;
                event->src = ev->src;
                event->dest = op->op->target_id;
		event->stream = PTL_HDR_STREAM_TRIG;
	    
                event->portals = true;
		event->head_packet = true;
//                 event->latency = cpu->latency*30/100;
	    
		memcpy(event->ptl_data,op->op->ptl_header,sizeof(ptl_header_t));
		if ( op->op->ptl_header->length != 0 ) {
		  dma_hdr_q.push(event);
		
		  // Also need to send the dma data to the dma engine
		  bool empty = dma_req_q.empty();
		  dma_req_q.push(op->op->dma);
		  if ( empty ) {
		    dma_link->Send(1,NULL);
		  }
		}
		else {
		  rtr_q_size++;
		  self_link->Send(ptl_msg_latency,event);

		  // If there's a CT attached to the MD, then we need
		  // to update the CT.
// 		  printf("%d\n",op->op->dma->ct_handle);
		  scheduleCTInc(op->op->dma->ct_handle);
		  delete op->op->dma;
		}
		
                delete op->op;
                delete op;
            }
	    else if ( op->op->op_type == PTL_OP_GET ) {
	        // Need to just create an event, copy the header, and
	        // send it off.
	        trig_nic_event* event = new trig_nic_event;
                event->src = ev->src;
                event->dest = op->op->target_id;
		event->stream = PTL_HDR_STREAM_TRIG;
	    
                event->portals = true;
		event->head_packet = true;
	    
		memcpy(event->ptl_data,op->op->ptl_header,sizeof(ptl_header_t));
		// FIXME: Just send directly.  This may "overflow" the
		// buffer, but it will all work out.  Eventually, will
		// need to do sowething better (probably whatever I do
		// to make zero put triggered put work.
		rtr_q_size++;
 		self_link->Send(ptl_msg_latency,event);
// 		dma_hdr_q.push(event);
		
                delete op->op;
                delete op;
	      
	    }
            else if ( op->op->op_type == PTL_OP_CT_INC ) {
		// FIXME
		scheduleCTInc(op->op->ct_handle);
                delete op->op;
                delete op;
            }

            // See if we need to process anything else
            if ( already_triggered_q.empty() ) {
                // Don't need the op passed into the function, so delete it
// XXX                delete operation;
// XXX		delete ev;
            }
            else {
                // Repurpose the operation passed into the function
//                 operation->op_type = PTL_NIC_PROCESS_TRIG;
                ev->ptl_op = PTL_NIC_PROCESS_TRIG;
                // Can process the next event in 8ns
//                 cpu->ptl_link->Send(8,new ptl_nic_event(operation));
		ptl_link->Send(1,ev);
            }
	}
	break;

    case PTL_NIC_CT_INC:
	{
// 	    ptl_ct_events[operation->data.ct_handle].ct_event.success++;
// 	    scheduleUpdateHostCT(operation->data.ct_handle);
	    ptl_ct_events[ev->data.ct_handle].ct_event.success++;
	    scheduleUpdateHostCT(ev->data.ct_handle);

            // need to see if any triggered operations have hit
//             trig_op_list_t* op_list = &ptl_ct_events[operation->data.ct_handle].trig_op_list;
            trig_op_list_t* op_list = &ptl_ct_events[ev->data.ct_handle].trig_op_list;
            trig_op_list_t::iterator op_iter, curr;
            for ( op_iter = op_list->begin(); op_iter != op_list->end();  ) {
                curr = op_iter++;
                ptl_int_trig_op_t* op = *curr;
                // For now just use the wait command, will need to change it later
//                 if ( PtlCTCheckThresh(operation->data.ct_handle,op->threshold) ) {
                if ( PtlCTCheckThresh(ev->data.ct_handle,op->threshold) ) {
                    // 	  if ( cpu->my_id == 0 ) {
                    // 	  }
                    // Simply move the operation to the already_triggered_q	      
                    bool empty = already_triggered_q.empty();
                    already_triggered_q.push(op);
                    if (empty) {
// 			operation->op_type = PTL_NIC_PROCESS_TRIG;
			ev->ptl_op = PTL_NIC_PROCESS_TRIG;
			ptl_link->Send(1,ev);
                    }
		    else {
// 			delete operation;
// 			delete ev;
		    }
                    op_list->erase(curr);
                }
            }
 	}
	break;
	
    case PTL_NIC_CT_SET:
	{
	    int ct_handle = ev->ptl_data[0];
	    ptl_ct_events[ct_handle].ct_event.success = ev->ptl_data[1];
	    ptl_ct_events[ct_handle].ct_event.failure = ev->ptl_data[2];
	    
	    
	    if ( ev->ptl_data[3] ) { // Clear op_list
		ptl_ct_events[ev->ptl_data[0]].trig_op_list.clear();
	    }
	    
	    scheduleUpdateHostCT(ev->ptl_data[0]);
	}
	break;

    case PTL_NIC_INIT_FOR_SEND_RECV:
    {
	// Need to put in a catch all ME.  Start doesn't matter because
	// it just goes into a queue object for the host to process.
	ptl_int_me_t* int_me = new ptl_int_me_t;
	int_me->active = true;
	int_me->handle_ct = PTL_CT_NONE;
	int_me->pt_index = 0;
	int_me->ptl_list = PTL_PRIORITY_LIST;
	
	int_me->me.start = NULL;
	int_me->me.length = 0;
	int_me->me.ct_handle = PTL_CT_NONE; 
	int_me->me.ignore_bits = ~0x0; 
	
	ptl_table[0]->priority_list->push_back(int_me);
	send_recv = true;
	delete ev;
    }
      break;
    default:
      break;
	
    }
    return false;
}

// Self-timed event handler that processes the DMA request Q.

// FIXME: Still need to add a credit based mechanism to limit
// outstanding DMAs when the dma_q is "full".
bool
trig_nic::processDMAEvent( Event* e)
{
//     printf("processDMAEvent()\n");
    // Look at the incoming queues from the host to see what to do.
    if ( !dma_in_progress ) {
        // Need to start a new DMA
      dma_req = dma_req_q.front();
      if ( dma_req_q.size() == 0 ) return false;
      if ( dma_req == NULL ) {
	  if ( dma_req_q.size() != 0 ) {
	    printf("NULL got passed into dma_req_q, aborting...\n");
	    abort();
	  }
	  // Don't actually have a new dma to start
	  return false;
	}
        dma_in_progress = true;

	dma_req_q.pop();
    }

    bool end = dma_req->length <= 64;
    int copy_length = end ? dma_req->length : 64;
	    
    trig_nic_event* request = new trig_nic_event;
    request->ptl_op = PTL_DMA_RESPONSE;
    request->data.dma = new ptl_int_dma_t;
    request->data.dma->start = dma_req->start;
    request->data.dma->length = copy_length;
    request->data.dma->offset = dma_req->offset;
    request->data.dma->target_id = dma_req->target_id;
    request->data.dma->end = end;
    request->data.dma->ct_handle = dma_req->ct_handle;
    
    request->stream = dma_req->stream;


    cpu_link->Send(1,request);
    
    if ( end ) {
      // Done with this DMA
      delete dma_req;
      dma_in_progress = false;
    }
    else {
      dma_req->offset += copy_length;
      dma_req->length -= copy_length;
    }

    if ( dma_in_progress || !dma_req_q.empty() ) {
      // Need to wake up again to process more DMAs
      dma_link->Send(8,NULL);
    }
    return false;
}


void
trig_nic::scheduleCTInc(ptl_handle_ct_t ct_handle) {
    if ( ct_handle != (ptl_handle_ct_t) PTL_CT_NONE ) {
//       printf("scheduleCTInc(%d)\n",ct_handle);
      trig_nic_event* event = new trig_nic_event;
	event->ptl_op = PTL_NIC_CT_INC;
// 	event->operation = new ptl_int_nic_op_t;
// 	event->operation->op_type = PTL_NIC_CT_INC;
// 	event->operation->data.ct_handle = ct_handle;
	event->ptl_op = PTL_NIC_CT_INC;
	event->data.ct_handle = ct_handle;
	ptl_link->Send(latency_ct_post,event);
    }
}

void
trig_nic::scheduleUpdateHostCT(ptl_handle_ct_t ct_handle) {
    ptl_update_ct_event_t* ct_update = new ptl_update_ct_event_t;
    ct_update->ct_event.success = ptl_ct_events[ct_handle].ct_event.success;
    ct_update->ct_event.failure = ptl_ct_events[ct_handle].ct_event.failure;
    ct_update->ct_handle = ct_handle;
    
//     ptl_int_nic_op_t* update_event = new ptl_int_nic_op_t;
//     update_event->op_type = PTL_NIC_UPDATE_CPU_CT;
//     update_event->data.ct = ct_update;
    
    trig_nic_event* event = new trig_nic_event;
    event->ptl_op = PTL_NIC_UPDATE_CPU_CT;
//     event->operation = update_event;
    event->data.ct = ct_update;

    cpu_link->Send(latency_ct_host_update,event);    
}
