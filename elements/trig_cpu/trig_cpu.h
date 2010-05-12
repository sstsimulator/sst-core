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


#ifndef _CPU_H
#define _CPU_H

#include <string>
#include <list>
#include <set>
#include <queue>
#include <stdlib.h>
#include <cstdlib>

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include <sst/timeConverter.h>
#include "../trig_nic/trig_nic_event.h"

using namespace SST;

struct posted_recv {
  int handle;
  int src;
  //  int tag; // not currently used
  uint64_t* buffer;

  posted_recv(int handle, int src, uint64_t* buffer) {
    this->handle = handle;
    this->src = src;
    this->buffer = buffer;
  }

};

  SimTime_t min = 1000000;
  SimTime_t max = 0;
  SimTime_t total_time = 0;
  int total_num = 0;
  bool rand_init = false;

  void addTimeToStats(SimTime_t time) {
  
    if ( time < min ) min = time;
    else if ( time > max ) max = time;
    total_time += time;
    total_num++;
  }

  void printStats() {
    printf("Max time: %lu ns\n",max);
    printf("Min time: %lu ns\n",min);
    printf("Avg time: %lu ns\n",total_time/total_num);
    printf("Total num: %d\n",total_num);
    return;
  }

  int getRand(int max) {
    if (!rand_init) {
      srand(0);
      rand_init = true;
    }
    if ( max == 0 ) return 0;
    return rand() % max;
    return 0;
  }

class trig_cpu : public Component {

 public:
  trig_cpu( ComponentId_t id, Params_t& params ) :
    Component( id ), params( params ), frequency( "2GHz" )
  {
	
    if ( params.find("nodes") == params.end() ) {
      _abort(RtrIF,"couldn't find number of nodes\n");
    }
    num_nodes = strtol( params[ "nodes" ].c_str(), NULL, 0 );
    
    if ( params.find("id") == params.end() ) {
      _abort(RtrIF,"couldn't find node id\n");
    }
    my_id = strtol( params[ "id" ].c_str(), NULL, 0 );

    if ( params.find("msgrate") == params.end() ) {
      _abort(RtrIF,"couldn't find msgrate\n");
    }
    std::string msg_rate = params[ "msgrate" ];


    if ( params.find("radix") == params.end() ) {
	_abort(RtrIF,"couldn't find radix\n");
    }
    radix = strtol( params[ "radix" ].c_str(), NULL, 0 );
    
    registerExit();
    
    Event::Handler_t*   handler = new EventHandler<
      trig_cpu, bool, Event* >
      ( this, &trig_cpu::processEvent );
    nic = LinkAdd( "nic", handler );
    
    ClockHandler_t* clock_handler;
    clock_handler = new EventHandler< trig_cpu, bool, Cycle_t >
      ( this, &trig_cpu::clock_allreduce );
    TimeConverter* tc = registerClock( frequency, clock_handler );
    outstanding_msg = 0;
    state = 0;

    // Now, convert the msgrate to a delay.  First, get a
    // TimeConverter from the
    // string (should be in hertz).  Then, the factor is the number of
    // core time steps.  Then convert that number to the number of
    // clock cycles using this component's TimeConverter.
    msg_rate_delay =
      tc->convertFromCoreTime(registerTimeBase(msg_rate,false)->getFactor());
 
  }

  int Setup() {
    busy = 0;
    recv_handle = 0;

/*     noise_interval = */
/*       defaultTimeBase->convertFromCoreTime(registerTimeBase("1kHz",false)->getFactor()); */
/*     noise_duration = */
/*             defaultTimeBase->convertFromCoreTime(registerTimeBase("25us",false)->getFactor()); */
    noise_interval = 0;
    noise_duration = 0;
    noise_count = getRand(noise_interval);
    return 0;
  }

  int Finish() {
    if (my_id == 0 ) printStats();
/*     if (my_id == 0 ) printf("Total time: %lu ns\n",getCurrentSimTimeNano()); */
/*     if (my_id == 0 ) { */
/*       printf("Max time: %lu ns\n",max); */
/*       printf("Min time: %lu ns\n",min); */
/*       printf("Avg time: %lu ns\n",total_time/total_num); */
/*     } */
    return 0;
  }

protected:
	void send(int dest, uint64_t data);
	bool recv(int src, uint64_t* buf, int& handle);
        bool process_pending_msg();
	bool waitall();
	bool waitall(int size, int* src);
	


private:
	// Base state
	int state;
	int my_id;
	int num_nodes;

	int size_x;
	int size_y;
	int size_z;
	
	// State needed by send/recv/wait
	uint64_t msg_rate_delay;
	uint64_t busy;
	int outstanding_msg;
	int recv_handle;
	std::list<posted_recv*> posted_recv_q;
	std::set<int> outstanding_recv;
	std::queue<trig_nic_event*> pending_msg;
	std::list<trig_nic_event*> unex_msg_q;

	// Noise variables
	SimTime_t noise_interval;
	SimTime_t noise_duration;
	SimTime_t noise_count;
	
	// State needed to run the "program"
	int current_send;
	SimTime_t start_time;
	int radix;
	int tree_id;
	int level;
	int result;
	int loop_var;

	
        trig_cpu( const trig_cpu& c );
	trig_cpu() {}
	
        bool clock( Cycle_t );
        bool clock_allreduce( Cycle_t );
        bool processEvent( Event* e );

        Params_t    params;
        Link*       nic;
	std::string frequency;

	
#if WANT_CHECKPOINT_SUPPORT
        BOOST_SERIALIZE {
	    printf("cpu::serialize()\n");
            _AR_DBG( Cpu, "start\n" );
	    printf("  doing void cast\n");
            BOOST_VOID_CAST_REGISTER( Cpu*, Component* );
	    printf("  base serializing: component\n");
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Component );
	    printf("  serializing: mem\n");
            ar & BOOST_SERIALIZATION_NVP( mem );
	    printf("  serializing: handler\n");
            ar & BOOST_SERIALIZATION_NVP( handler );
            _AR_DBG( Cpu, "done\n" );
        }
#endif
};

#endif
