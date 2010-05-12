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

#include "trig_cpu.h"


// #include <sst/memEvent.h>
#include "../trig_nic/trig_nic_event.h"

// BOOST_CLASS_EXPORT( SST::MyMemEvent )
//BOOST_IS_MPI_DATATYPE( SST::MyMemEvent )

bool trig_cpu::clock( Cycle_t current )
{
    int handle;  
  if (busy) {
    busy--;
    return false;
  }
  
  switch (state) {
  case 0:
    current_send = 0;
    state = 1;
    break;
  case 1:
//     printf("Posting recieves\n");
    for ( int i = 0; i < num_nodes; i++ )
	recv(i,NULL,handle);
    state = 2;
    break;
  case 2:
//     printf("Sending to %d\n",current_send);
    send(current_send++,my_id);
    if ( current_send == num_nodes ) {
//       printf("moving to state 2\n");
      state = 3;
    }
    break;
  case 3:
    if ( waitall() ) state = 4;
    break;
  case 4:
    printf("%d: unregistering exit\n",my_id);
    unregisterExit();
    state = 5;
    break;
  case 5:
    break;
  default:
    break;
  }
  
  return false;
}

bool trig_cpu::clock_allreduce( Cycle_t current )
{

    int handle;
    if (noise_count == 0 ) {
	noise_count = noise_interval-1;
	busy += noise_duration;
    }
    else {
      noise_count--;
    }
    
    if (busy) {
	busy--;
	return false;
    }
    switch (state) {
    case 0:
	current_send = 0;
	state = 1;
	start_time = getCurrentSimTimeNano();
	tree_id = my_id;
	level = 1;
	result = my_id;
	break;
    case 1:
	loop_var = 1;
	state = 2;
    case 2:
	if ( tree_id % (level*radix) == 0 ) {
	    // Not a leaf.
	    // Post receives for all of my leafs
//       for ( int i = 1; i < radix; i++ ) {
// 	recv(my_id+(level*i),NULL);
//       }
	    if ( loop_var < radix ) {
		if ( recv(my_id+(level*loop_var),NULL,handle) ) {
		    loop_var++;
		}
	    }
	    else {
		state = 3;
	    }
	}
	else {
	    // Leaf. Just send to root.
	    send((my_id/(radix*level))*(radix*level),result);
	    state = 5;
	}
	break;
    case 3:
	// Wait for my leafs
	if ( waitall() ) state = 4;
	// 
	break;
    case 4:
	// Got results, do computation, send it up another level
	level = level * radix;
	if ( level == num_nodes ) {
	    // Hit root, need to move to broadcast
	    state = 7;
	    //       printf("%d: Starting broadcast\n",my_id);
	}
	else {
	    state = 1;
	}
	break;
    case 5:
	// Need to get results from my root
	//     printf("%d: level = %d in state 4\n",my_id,level);
	if ( recv((my_id/(radix*level))*(radix*level),NULL,handle) ) {
	    state = 6;
	}
	break;
    case 6:
	if ( waitall() ) {
	    if ( level == 1 ) state = 10;
	    else state = 7;
	}
	break;
    case 7:
	loop_var = 1;
	level = level / radix;
	state = 8;
    case 8:
	// Fan result back out
	//     for ( int i = 1; i < radix; i++ ) {
	//       send(my_id + (i*level),result);
	//     }
	//     state = 9;    
	if ( loop_var < radix ) {
	    send(my_id + (loop_var*level),result);
	    loop_var++;
	}
	else {
	    state = 9;
	}
	break;
    case 9:
	if ( level == 1 ) {
	    // We're at the very bottom
	    state = 10;
	    break;
	}
	state = 7;
	break;
    case 10:
	addTimeToStats(getCurrentSimTimeNano()-start_time);
	//     printf("%d: unregistering exit\n",my_id);
	unregisterExit();
	state = 11;
	break;
    case 11:
	break;
    default:
	break;
    }
    
    return false;
}

bool trig_cpu::processEvent(Event* event) {
  trig_nic_event* ev = static_cast<trig_nic_event*>(event);

  // For now, just put the event in a queue that will be processed
  // when recv or wait is called.
  pending_msg.push(ev);
  
  return false;
}



void trig_cpu::send(int dest, uint64_t data) {
  if ( my_id == 0 ) printf("send(%d)\n",dest);
//   printf("%4d: send(%d)\n",my_id,dest);
  trig_nic_event* event = new trig_nic_event;
  event->src = my_id;
  event->dest = dest;
//   event->data = data;
//   event->data = getCurrentSimTimeNano();
//   if ( my_id == 0 ) printf("Sending msg to %d\n",dest);
  nic->Send(busy,event);
  // We are busy for msg_rate_delay
  busy += msg_rate_delay;
}

bool trig_cpu::process_pending_msg() {
  while (!pending_msg.empty()) {
    trig_nic_event* event = pending_msg.front();
    pending_msg.pop();
    
    // Search the posted receives queue for a match
    std::list<posted_recv*>::iterator iter;
    bool found = false;
    for ( iter = posted_recv_q.begin(); iter != posted_recv_q.end(); ++iter ) {
      posted_recv* pr = *iter;
      if (pr->src == event->src) {
	posted_recv_q.erase(iter);
	delete pr;
	busy += msg_rate_delay;
	found = true;
	break;
      }
    }

    if (found) {
      delete event;
      return false;
    }
    else {
      // No match, put in enexpected queue
      unex_msg_q.push_back(event);
    }
  }
  return true;
}


bool trig_cpu::recv(int src, uint64_t* buf, int& handle) {
  if ( my_id == 0 ) printf("recv(%d)  -- state = %d\n",src,state);
//   printf("%4d: recv(%d)\n",my_id,src);

  bool ret = process_pending_msg();
  // If we found something in the pending messages that match, we need
  // to return false to let the sim go busy.
  if (!ret) return ret;
  
  // See if the message is in the unexpected queue
  // Search the posted receives queue for a match
  std::list<trig_nic_event*>::iterator iter;
  bool found = false;
  for ( iter = unex_msg_q.begin(); iter != unex_msg_q.end(); ++iter ) {
    trig_nic_event* msg = *iter;
    if (msg->src == src) {
      unex_msg_q.erase(iter);
      delete msg;
      busy += msg_rate_delay;
      found = true;
      break;
    }
  }
  
  if (!found) {
    // Insert a new entry in posted recieves queue
    posted_recv_q.push_back(new posted_recv(recv_handle,src,buf));
    
    // Insert an entry into a set so we can more easily implement wait()
    outstanding_recv.insert(recv_handle);
  }
  handle = recv_handle++;
  return true;
  
}

bool trig_cpu::waitall() {
    if (!process_pending_msg()) return false;
//   if ( outstanding_msg == 0 ) return true;
  if ( posted_recv_q.size() == 0 ) return true;
  return false;
}

extern "C" {
trig_cpu*
trig_cpuAllocComponent( SST::ComponentId_t id,
                        SST::Component::Params_t& params )
{
//     printf("cpuAllocComponent--> sim = %p\n",sim);
    return new trig_cpu( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(Cpu)

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Cpu, bool, SST::Cycle_t, SST::Time_t )
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Cpu, bool, SST::Cycle_t)
#endif
