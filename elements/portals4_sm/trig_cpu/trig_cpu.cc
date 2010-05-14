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


#include <sst_config.h>

#include <sst/element.h>

#include "trig_cpu.h"
#include "elements/portals4_sm/trig_nic/trig_nic_event.h"

#include "allreduce_narytree.h"
#include "allreduce_narytree_trig.h"
#include "allreduce_recdbl.h"
#include "allreduce_recdbl_trig.h"
#include "allreduce_tree.h"
#include "allreduce_tree_trig.h"

#include "bcast.h"
#include "bcast_trig.h"

#include "barrier_tree.h"
#include "barrier_tree_trig.h"
#include "barrier_recdbl.h"
#include "barrier_recdbl_trig.h"
#include "barrier_dissem.h"
#include "barrier_dissem_trig.h"

#include "test_portals.h"
#include "test_mpi.h"
#include "ping_pong.h"
#include "bandwidth.h"

SimTime_t trig_cpu::min = 10000000;
SimTime_t trig_cpu::max = 0;
SimTime_t trig_cpu::total_time = 0;
int trig_cpu::total_num = 0;

SimTime_t trig_cpu::overall_min = 1000000;
SimTime_t trig_cpu::overall_max = 0;
SimTime_t trig_cpu::overall_total_time = 0;
int trig_cpu::overall_total_num = 0;

bool trig_cpu::rand_init = false;

// Infrastructure for doing more than one allreduce in a single
// simulation
Link** trig_cpu::wake_up = NULL;
int trig_cpu::current_link = 0;
int trig_cpu::total_nodes;
int trig_cpu::num_remaining;

trig_cpu::trig_cpu(ComponentId_t id, Params_t& params) :
    Component( id ),
    delay_host_pio_write(8),
    delay_bus_xfer(16),
    latency_dma_mem_access(1), // latency built in already
    params( params ),
    nic_timing_wakeup_scheduled(false),
    dma_return_count(0),
    frequency( "1GHz" ),
    wc_buffers_max(8)
{
    if ( params.find("nodes") == params.end() ) {
        _abort(RtrIF,"couldn't find number of nodes\n");
    }
    num_nodes = strtol( params[ "nodes" ].c_str(), NULL, 0 );
    
    if ( params.find("id") == params.end() ) {
        _abort(RtrIF,"couldn't find node id\n");
    }
    my_id = strtol( params[ "id" ].c_str(), NULL, 0 );

    if ( params.find("timing_set") == params.end() ) {
	_abort(trig_nic,"couldn't find timing set\n");
    }
    timing_set = strtol( params[ "timing_set" ].c_str(), NULL, 0 );

    setTimingParams(timing_set);
    if ( params.find("msgrate") == params.end() ) {
        _abort(RtrIF,"couldn't find msgrate\n");
    }
    std::string msg_rate = params[ "msgrate" ];

    if ( params.find("xDimSize") == params.end() ) {
        _abort(RtrIF,"couldn't find xDimSize\n");
    }
    x_size = strtol( params[ "xDimSize" ].c_str(), NULL, 0 );

    if ( params.find("yDimSize") == params.end() ) {
        _abort(RtrIF,"couldn't find yDimSize\n");
    }
    y_size = strtol( params[ "yDimSize" ].c_str(), NULL, 0 );

    if ( params.find("zDimSize") == params.end() ) {
        _abort(RtrIF,"couldn't find zDimSize\n");
    }
    z_size = strtol( params[ "zDimSize" ].c_str(), NULL, 0 );

    if ( params.find("latency") == params.end() ) {
        _abort(RtrIF,"couldn't find latency\n");
    }
    latency = strtol( params[ "latency" ].c_str(), NULL, 0 );
    //     latency = latency / 2;

    if ( params.find("radix") == params.end() ) {
	_abort(RtrIF,"couldn't find radix\n");
    }
    radix = strtol( params[ "radix" ].c_str(), NULL, 0 );

    if ( params.find("msg_size") == params.end() ) {
	_abort(RtrIF,"couldn't find msg_size\n");
    }
    msg_size = strtol( params[ "msg_size" ].c_str(), NULL, 0 );

    if ( params.find("chunk_size") == params.end() ) {
	_abort(RtrIF,"couldn't find chunk_size\n");
    }
    chunk_size = strtol( params[ "chunk_size" ].c_str(), NULL, 0 );

    TimeConverter* tc = registerTimeBase( frequency );

    if ( params.find("noiseRuns") == params.end() ) {
	_abort(RtrIF,"couldn't find noiseRuns\n");
    }
    noise_runs = strtol( params[ "noiseRuns" ].c_str(), NULL, 0 );

    if ( noise_runs == 0 ) {
        noise_interval = 0;
        noise_duration = 0;
    }
    else {
        if ( params.find("noiseInterval") == params.end() ) {
	    _abort(RtrIF,"couldn't find noiseInterval\n");
	}
	noise_interval =
	    defaultTimeBase->convertFromCoreTime(registerTimeBase(params["noiseInterval"],
                                                                  false)->getFactor());
	
	if ( params.find("noiseDuration") == params.end() ) {
	    _abort(RtrIF,"couldn't find noiseDuration\n");
	}
	noise_duration =
	    defaultTimeBase->convertFromCoreTime(registerTimeBase(params["noiseDuration"],
                                                                  false)->getFactor());
    }

    if (params.find("collective") == params.end()) {
        _abort(RtrIF, "couldn't find collective\n");
    }
    std::string collective = params["collective"];
    if (params.find("algorithm") == params.end()) {
        _abort(RtrIF, "couldn't find algorithm\n");
    }
    std::string algorithm = params["algorithm"];

    initPortals();
    use_portals = true;

    if (collective == "allreduce") {
        if (algorithm == "tree") {
            use_portals = false;
            coll_algo = new allreduce_tree(this);
        } else if (algorithm == "narytree") {
            use_portals = false;
            coll_algo = new allreduce_narytree(this);
        } else if (algorithm == "recursive_doubling") {
            use_portals = false;
            coll_algo = new allreduce_recdbl(this);
        } else if (algorithm == "tree_triggered") {
	    use_portals = true;
            coll_algo = new allreduce_tree_triggered(this);
        } else if (algorithm == "narytree_triggered") {
	    use_portals = true;
            coll_algo = new allreduce_narytree_triggered(this);
        } else if (algorithm == "recursive_doubling_triggered") {
	    use_portals = true;
            coll_algo = new allreduce_recdbl_triggered(this);
        } else {
            _abort(trig_cpu, "Invalid algorithm %s:%s\n", collective.c_str(), algorithm.c_str());
        }

    } else if (collective == "bcast") {
        if (algorithm == "tree") {
            use_portals = false;
	    coll_algo = new bcast_tree(this);
        } else if (algorithm == "tree_triggered") {
	    coll_algo = new bcast_tree_triggered(this);
        } else {
            _abort(trig_cpu, "Invalid algorithm %s:%s\n", collective.c_str(), algorithm.c_str());
        }

    } else if (collective == "barrier") {
        if (algorithm == "tree") {
	    use_portals = false;
	    coll_algo = new barrier_tree(this);
        } else if (algorithm == "recursive_doubling") {
            use_portals = false;
            coll_algo = new barrier_recdbl(this);
        } else if (algorithm == "dissemination") {
            use_portals = false;
            coll_algo = new barrier_dissemination(this);
        } else if (algorithm == "tree_triggered") {
	    coll_algo = new barrier_tree_triggered(this);
        } else if (algorithm == "recursive_doubling_triggered") {
            coll_algo = new barrier_recdbl_triggered(this);
        } else if (algorithm == "dissemination_triggered") {
            coll_algo = new barrier_dissemination_triggered(this);
        } else {
            _abort(trig_cpu, "Invalid algorithm %s:%s\n", collective.c_str(), algorithm.c_str());
        }

    } else if ( collective == "test_portals" ) {
        coll_algo = new test_portals(this);

    } else if ( collective == "test_mpi" ) {
	use_portals = false;
        coll_algo = new test_mpi(this);

    } else if ( collective == "ping_pong" ) {
      coll_algo = new ping_pong(this);

    } else if ( collective == "bandwidth" ) {
        coll_algo = new bandwidth(this);

    } else {
        _abort(RtrIF, "Invalid collective: %s\n", collective.c_str());
    }
    
    registerExit();
    
    Event::Handler_t*   handler = new EventHandler<
        trig_cpu, bool, Event* >
//         ( this, use_portals ? &trig_cpu::processEventPortals : &trig_cpu::processEvent );
        ( this, &trig_cpu::processEventPortals );
    nic = LinkAdd( "nic", handler );
    nic->setDefaultTimeBase(defaultTimeBase);

    Event::Handler_t* selfLinkHandler = new EventHandler<
	trig_cpu, bool, Event* >
	(this,&trig_cpu::event_handler );
    self = selfLink("self", selfLinkHandler);
    // For now, self links won't set themselves to the
    // defaultTimeBase.  May not be able to fix that.
    self->setDefaultTimeBase(defaultTimeBase);

    Event::Handler_t* nic_timing_handler = new EventHandler<
	trig_cpu, bool, Event* >
	(this,&trig_cpu::event_nic_timing );
    nic_timing_link = selfLink("nic_timing_link", nic_timing_handler);
    // For now, self links won't set themselves to the
    // defaultTimeBase.  May not be able to fix that.
    nic_timing_link->setDefaultTimeBase(registerTimeBase("1ns",false));

    Event::Handler_t* dma_return_handler = new EventHandler<
	trig_cpu, bool, Event* >
	(this,&trig_cpu::event_dma_return );
    dma_return_link = selfLink("dma_return_link", dma_return_handler);
    // For now, self links won't set themselves to the
    // defaultTimeBase.  May not be able to fix that.
    dma_return_link->setDefaultTimeBase(registerTimeBase("1ns",false));

    Event::Handler_t* pio_delay_handler = new EventHandler<
	trig_cpu, bool, Event* >
	(this,&trig_cpu::event_pio_delay );
    pio_delay_link = selfLink("pio_delay_link", pio_delay_handler);
    // For now, self links won't set themselves to the
    // defaultTimeBase.  May not be able to fix that.
    pio_delay_link->setDefaultTimeBase(registerTimeBase("1ns",false));


    outstanding_msg = 0;
    top_state = ((noise_runs == 0) ? 0 : 2);
    current_run = 0;

    // Now, convert the msgrate to a delay.  First, get a
    // TimeConverter from the
    // string (should be in hertz).  Then, the factor is the number of
    // core time steps.  Then convert that number to the number of
    // clock cycles using this component's TimeConverter.
    msg_rate_delay =
        tc->convertFromCoreTime(registerTimeBase(msg_rate,false)->getFactor());

//     if ( use_portals ) {
//         ptl = new portals(this);
      
//         Event::Handler_t* ptlLinkHandler = new EventHandler<
//             trig_cpu, bool, Event* >
//             (this,&trig_cpu::ptlNICHandler );
//         ptl_link = selfLink("ptl", ptlLinkHandler);
//         ptl_link->setDefaultTimeBase(registerTimeBase("1ns",false));
//     }
} 

void
trig_cpu::initPortals() {
    ptl = new portals(this);
    
//     Event::Handler_t* ptlLinkHandler = new EventHandler<
//     trig_cpu, bool, Event* >
// 	(this,&trig_cpu::ptlNICHandler );
//     ptl_link = selfLink("ptl", ptlLinkHandler);
//     ptl_link->setDefaultTimeBase(registerTimeBase("1ns",false));
}

int
trig_cpu::Setup()
{
    busy = 0;
    recv_handle = 0;

    if (my_id == 0 ) {
	setTotalNodes(num_nodes);
	resetBarrier();
    }
    
    noise_count = getRand(noise_interval);
    waiting = false;
    self->Send(1,NULL);
    count = 0;
    addWakeUp(self);

    nic_credits = 128;
    blocking = false;

    pio_in_progress = false;

    
    if ( sizeof(ptl_header_t) > 32 ) {
	printf("Portals header (ptl_header_t) is bigger than 32 bytes, aborting...\n");
	abort();
    }

    if ( !use_portals ) {
      trig_nic_event* event = new trig_nic_event;
      event->src = my_id;
      event->ptl_op = PTL_NIC_INIT_FOR_SEND_RECV;

      nic->Send(1,event);

      
    }
    return 0;
}

int
trig_cpu::Finish()
{
    if (my_id == 0 ) printOverallStats();
    return 0;    
}

int
trig_cpu::calcXPosition( int node )
{
    return node % x_size; 
}

int
trig_cpu::calcYPosition( int node )
{
    return ( node / x_size ) % y_size; 
}

int
trig_cpu::calcZPosition( int node )
{
    return node / ( x_size * y_size ); 
}

int
trig_cpu::calcNodeID(int x, int y, int z)
{
    return ( z * ( x_size*y_size ) ) + ( y * x_size ) + x;
}

void
trig_cpu::setTimingParams(int set) {
    if ( set == 1 ) {
	delay_host_pio_write = 75;
	added_pio_latency = 0;
	recv_overhead = 100;
    }
    if ( set == 2 ) {
	delay_host_pio_write = 100;
	added_pio_latency = 0;
	recv_overhead = 175;
    }
    if ( set == 3 ) {
	delay_host_pio_write = 200;
	added_pio_latency = 100;
	recv_overhead = 300;
    }

//     printf("delay_host_pio_write = %d\n",delay_host_pio_write);
//     printf("added_pio_latency = %d\n",added_pio_latency);
}

bool
trig_cpu::event_pio_delay(Event* e)
{
  trig_nic_event* ev = static_cast<trig_nic_event*>(e);
//   printf("%5d: putting event type %d into wc_buffers @ %llu\n",my_id,ev->ptl_op,getCurrentSimTimeNano());
//   if (ev->ptl_op == PTL_NIC_ME_APPEND) printf("   ME Append\n");
  wc_buffers.push(ev);

  // If this is the only entry, then we need to wakeup the interface
  // to transfer the data.  Otherwise, it will wakeup itself
  if ( !nic_timing_wakeup_scheduled ) {
    nic_timing_link->Send(1,NULL);
    nic_timing_wakeup_scheduled = true;
  }
  return false;
}



// This attempts to write to the write combined buffers to the NIC.
// If they are full, then the CPU will stall until there's space
bool
trig_cpu::writeToNIC(trig_nic_event* ev)
{
//     printf("trig_cpu::writeToNIC()\n");
    // Need to see if we have any credits to use
    if ( nic_credits != 0 ) {
        // Put this through the delay link
        pio_delay_link->Send(delay_host_pio_write,ev);
// 	// Put the event into the write combining buffers
// 	wc_buffers.push(ev);
	nic_credits--;

	return true;
    }
    else {
//       printf("No credits available\n");
	// No credits, need to wait until we get some.
        blocking = true;
	waiting = false;

//  	printf("%5d: Ran out of credits @ %llu\n",my_id,getCurrentSimTimeNano());
	
	// Need to keep track of what is blocking so we can send it
	// once we get credits.  It's done this way to make the "user"
	// code easier to write.
	blocked_event = ev;
 	blocked_busy = busy;
	return false;
    }
}

void
trig_cpu::returnCredits(int num)
{
    nic_credits += num;
    if ( blocking ) {
	blocking = false;
	writeToNIC(blocked_event);
	busy += blocked_busy;
	wakeUp();
//  	printf("Blocking, but got credit back @ %llu\n",getCurrentSimTimeNano());
    }
}


bool
trig_cpu::event_nic_timing(Event* e)
{
  // Need to arbitrate between PIO and DMA traffic.  For now just give
  // DMA traffic priority, we shouldn't see them both at the same time
  // for the current sims
  
  if ( dma_buffers.size() != 0 ) {
    nic->Send(0,dma_buffers.front());
    dma_buffers.pop();
  }
  else if ( wc_buffers.size() != 0 ) {
//     if ( my_id == 0 && wc_buffers.front()->ptl_op == PTL_NO_OP )
//     if ( my_id == 0 )
//       printf("%5d:  Writing event type %d from WCs to bus @ %llu\n",my_id,wc_buffers.front()->ptl_op,getCurrentSimTimeNano());

    nic->Send(added_pio_latency,wc_buffers.front());
    wc_buffers.pop();
  }

    // If there's still events left, make myself up again
  if ( wc_buffers.size() != 0 || dma_buffers.size() != 0 ) {
    nic_timing_link->Send(delay_bus_xfer,NULL);
    nic_timing_wakeup_scheduled = true;
  }
  else {
    nic_timing_wakeup_scheduled = false;
  }
  return false;
}

// This is the main handler, and it will call the appropriate handler
// underneath
bool
trig_cpu::event_handler(Event* ev)
{
//     printf("trig_cpu::event_handler()\n");
    
    bool done = false;

    // Need to see if there is any left over work to do

    // One thing that can happen is we block on a PIO to the NIC.  If
    // we do, we need to complete it before we proceed
//     if ( blocking && !waiting ) {
// 	blocking = false;
// 	writeToNIC(blocked_event);
// 	busy += blocked_busy;
//     }
//     else
    if ( pio_in_progress ) {
	// Need to progress the PIO.  A return value of true means
	// it's finally done.
	if ( ptl->progressPIO() ) {
	    pio_in_progress = false;
	    busy+= recv_overhead;
	}
    }
    else {
// 	printf("top_state = %d\n",top_state);
	switch (top_state) {
	case 0:
	    // No noise
	    done = (*coll_algo)(ev);
// 	    printf("done = %d\n",done);
	    if (done) {
		top_state = 1;
		barrier();
		return false;
	    }
	    break;
	case 1:
// 	  printf("Unregister exit 1\n");
	    unregisterExit();
	    return false;
	case 2:
	    // Noise runs, have to do the specified number
	    if ( current_run < noise_runs ) {
		top_state = 3;
	    }
	    else {
		// Done with runs
// 	  printf("Unregister exit 2\n");
		unregisterExit();
		return false;
	    }
	case 3:
	    done = (*coll_algo)(ev);
	    if (done) {
		current_run++;
		top_state = 2;
		barrier();
		return false;
	    }
	    break;
	    
	}
    }
  
    if (done) {
// 	printf("NOT DONE\n");
	return false;
    }
    // Figure out how long to wait until the next time we do
    // something.  We do this by using a combination of busy and the
    // noise parameters.

    // Normally, we will just return after the busy period, but if
    // noise is supposed to start during that time, we will also need
    // to add the noise duration.
    int noise_rem = noise_count - busy;
    
    if ( noise_rem <= 0 && noise_interval != 0) {
        // Noise should start before we return, so add noise to busy and
        // reset the noise_count
        busy += noise_duration;
        noise_count = noise_interval - noise_duration;
        noise_rem = noise_count;
    }
    else if ( waiting || blocking ) {
	// This means nothing interesting happened and won't until we
	// get a new message.  We will simply wait until we have a new
	// message, but we have to keep some extra state around to
	// make sure the noise gets added in correctly.
// 	if (my_id == 1024 && waiting ) printf("%5d: waiting, outstanding recvs = %d\n",my_id,posted_recv_q.size());
	wait_start_time = getCurrentSimTime();
	return false;  // Won't wake up until a new message arrives
    }
    else if ( busy == 0 ) {
        // We need to start again next cycle, so set busy to 1
        busy = 1;
    }
//     printf("Busy for %d\n",busy);
    self->Send(busy,NULL);
    busy = 0;
    if ( noise_interval != 0 ) noise_count = noise_rem;
    else noise_count = 0;
    return false;
}

bool
trig_cpu::processEventPortals(Event* event)
{
//     printf("trig_cpu::processEventPortals()\n");
    trig_nic_event* ev = static_cast<trig_nic_event*>(event);
    // For now just add the portals stuff here
    ptl->processMessage(ev);
    return false;
}

bool
trig_cpu::ptlNICHandler(Event* event)
{
// //     printf("trig_cpu::ptlNICHandler()\n");
//     ptl_nic_event* ev = static_cast<ptl_nic_event*>(event);
//     ptl->processNICOp(ev->operation);
    return false;
}

void
trig_cpu::wakeUp()
{
//     if ( waiting ) {
        //     if (my_id == 0) printf("Received message while waiting\n");

	waiting = false;
	busy = 0;

        if ( noise_interval == 0 ) {
            self->Send(1,NULL);
            return;
        }
        // See if we need to add any noise before we wake up the main
        // "thread"
        SimTime_t elapsed_time = getCurrentSimTime() - wait_start_time;
        if ( elapsed_time < noise_count ) {
            noise_count -= elapsed_time;
            self->Send(1,NULL);
        }
        else if ( elapsed_time < (noise_count + noise_duration) ) {
            // This means we are in the middle of noise, figure out how much
            // is left
            SimTime_t noise_left = noise_count + noise_duration - elapsed_time;
            noise_count = noise_interval - noise_duration;
            self->Send(noise_left,NULL);
        }
        else if ( elapsed_time < (noise_count + noise_interval) ) {
            // Noise happened, but is done
            noise_count = noise_count + noise_interval - elapsed_time;
            self->Send(1,NULL);
        }
        else {
            // Need to determin if we are in noise or not.  Figure out how
            // far we are from the start of a noise interval;
            SimTime_t from_interval_start =
                (elapsed_time - (noise_count + noise_interval)) % noise_interval;
            if ( from_interval_start < noise_duration ) {
                // In noise
                self->Send(noise_duration - from_interval_start, NULL);
                noise_count = noise_interval - noise_duration;
            }
            else {
                // Not in noise
                self->Send(1,NULL);
                noise_count = noise_interval - from_interval_start;
            }
        }
//     }
}

bool
trig_cpu::event_dma_return(Event *e)
{
  trig_nic_event* ev = static_cast<trig_nic_event*>(e);
  dma_buffers.push(ev);

  // Need to wake up the nic_timing handler if it is not already
  // scheduled to be
  if ( !nic_timing_wakeup_scheduled ) {
    nic_timing_link->Send(delay_bus_xfer,NULL);
    nic_timing_wakeup_scheduled = true;
  }
  return false;
}


bool
trig_cpu::processEvent(Event* event)
{
    trig_nic_event* ev = static_cast<trig_nic_event*>(event);
  
    //   if ( my_id == 0 )
    //       printf("%5d: event received by 0 @ %lu\n",ev->src,getCurrentSimTimeNano());
    // For now, just put the event in a queue that will be processed
    // when recv or wait is called.
    pending_msg.push(ev);

    // If we are waiting, then we need to wake up the main processing
    // code
    if ( waiting ) {
        wakeUp();
    }
    return false;
}

void
trig_cpu::send(int dest, uint64_t data)
{
    //   if ( my_id == 0 ) printf("send(%d) @ %lu\n",dest,getCurrentSimTimeNano());
    //   printf("%4d: send(%d)\n",my_id,dest);
    trig_nic_event* event = new trig_nic_event;
    event->src = my_id;
    event->dest = dest;

    nic->Send(busy,event);
    // We are busy for msg_rate_delay
    busy += msg_rate_delay;
}

void
trig_cpu::isend(int dest, void* data, int length)
{
    if ( my_id == 1024 )printf("%5d: isend(%d, %p, %d)\n",my_id,dest,data,length);
  // Need to create an MD
  ptl_md_t md;
  md.start = data;
  md.length = length;
  md.eq_handle = PTL_EQ_NONE; 
  md.ct_handle = PTL_CT_NONE; 

  // Now send it
  ptl->PtlPut(&md,0,length,0,dest,0,0,0,NULL,0);
}

bool
trig_cpu::process_pending_msg()
{
  while (!pending_msg.empty()) {
        trig_nic_event* event = pending_msg.front();

	// Need to extract the portals header to get the length
	ptl_header_t header;
	memcpy(&header,event->ptl_data,sizeof(ptl_header_t));
	// Figure out how many packets there are
	int packets;
	packets = (((header.length - 32) + 63) / 64) + 1;
//  	printf("%5d: packets = %d length = %d\n",my_id,packets,header.length);
	// Make sure it's all there, otherwise we won't process yet
	bool all_there = false;
	if ( (int) pending_msg.size() >= packets ) all_there = true;

	if ( !all_there ) {
	  // If it's not all there, we'll act as if the queue is empty
	  return true;
	}
// 	printf("Received a message\n");
        pending_msg.pop();

	// Copy it all into one place
	//	memcpy(pr->buffer,&event->ptl_data[9],copy_length);

	int src = event->src;
	
	// Already have the first packet
	uint8_t* msg = new uint8_t[header.length];
	int copy_length = header.length <= 32 ? header.length : 32;
	memcpy(msg,&event->ptl_data[8],copy_length);
	int rem_length = header.length - copy_length;
	int curr_offset = copy_length;

	delete event;
	for ( int i = 1; i < packets; i++ ) {
	  event = pending_msg.front();
	  pending_msg.pop();
	  copy_length = rem_length <= 64 ? rem_length : 64;
	  memcpy(&msg[curr_offset],event->ptl_data,copy_length);
	  rem_length -= copy_length;
	  curr_offset += copy_length;
	  delete event;
	}

	posted_recv* found_pr = NULL;
        // Search the posted receives queue for a match
        std::list<posted_recv*>::iterator iter;
        bool found = false;
        for ( iter = posted_recv_q.begin(); iter != posted_recv_q.end(); ++iter ) {
            posted_recv* pr = *iter;
            if (pr->src == src) {
                posted_recv_q.erase(iter);
                found_pr = pr;
                busy += recv_overhead;
                found = true;
                break;
            }
        }

        if (found) {
	  // Need to do all the copying and pop all the packets in the
	  // message.
	  // Copy the message to the final buffer
// 	  printf("%5d: copying message to buffer\n",my_id);
	  memcpy(found_pr->buffer,msg,header.length);
	  delete[] msg;
	  delete found_pr;
	  return false;
        }
        else {
            // No match, put in enexpected queue
	  unex_msg_q.push_back(new unex_msg(msg,src,header.length));
        }
    }
    return true;
}


bool
trig_cpu::recv(int src, uint64_t* buf, int& handle)
{
//     bool ret = process_pending_msg();
//     // If we found something in the pending messages that match, we need
//     // to return false to let the sim go busy.
//     if (!ret) return ret;
  
//     // See if the message is in the unexpected queue
//     // Search the posted receives queue for a match
//     std::list<trig_nic_event*>::iterator iter;
//     bool found = false;
//     for ( iter = unex_msg_q.begin(); iter != unex_msg_q.end(); ++iter ) {
//         trig_nic_event* msg = *iter;
//         if (msg->src == src) {
//             unex_msg_q.erase(iter);
//             delete msg;
//             busy += msg_rate_delay;
//             found = true;
//             break;
//         }
//     }
  
//     if (!found) {
//         // Insert a new entry in posted recieves queue
//         posted_recv_q.push_back(new posted_recv(recv_handle,src,buf));
    
//         // Insert an entry into a set so we can more easily implement wait()
//         outstanding_recv.insert(recv_handle);
//     }
//     handle = recv_handle++;
    return true;
  
}

void foobar(void);

bool
trig_cpu::irecv(int src, void* buf, int& handle)
{
    if ( my_id == 1024 ) printf("%5d: irecv(%d, %p)\n",my_id,src,buf);
  bool ret = process_pending_msg();
    // If we found something in the pending messages that match, we need
    // to return false to let the sim go busy.
    if (!ret) return ret;

    // See if the message is in the unexpected queue
    // Search the posted receives queue for a match
    std::list<unex_msg*>::iterator iter;
    bool found = false;
    for ( iter = unex_msg_q.begin(); iter != unex_msg_q.end(); ++iter ) {
        unex_msg* msg = *iter;
        if (msg->src == src) {
            unex_msg_q.erase(iter);
            busy += recv_overhead;
            found = true;
// 	    printf("%5d: Found unexpected from %d\n",my_id,src);

	    // Need to copy over the data
	    memcpy(buf,msg->data,msg->length);
	    
	    delete[] msg->data;
	    delete msg;
            break;
        }
    }
  
    if (!found) {
        // Insert a new entry in posted recieves queue
//       printf("%5d: Posting a receive from %d\n",my_id,src);
      posted_recv_q.push_back(new posted_recv(recv_handle,src,buf));
    
        // Insert an entry into a set so we can more easily implement wait()
        outstanding_recv.insert(recv_handle);
    }
    handle = recv_handle++;
    return true;
  
}

bool
trig_cpu::waitall()
{
  if (!process_pending_msg()) return false;
    //   if ( outstanding_msg == 0 ) return true;
    if ( posted_recv_q.size() == 0 ) {
        waiting = false;
        return true;
    }
    //     if (my_id == 0) printf("waiting = true, busy = %d\n",busy);
//     printf("Waiting...\n");
    waiting = true;
    return false;
}
