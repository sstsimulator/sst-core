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


#ifndef COMPONENTS_TRIG_CPU_TRIG_CPU_H
#define COMPONENTS_TRIG_CPU_TRIG_CPU_H

#include <string>
#include <list>
#include <set>
#include <queue>
#include <vector>
#include <cstdlib>
#include <stdlib.h>

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/compEvent.h>
#include <sst/link.h>
#include <sst/timeConverter.h>

#include "elements/portals4_sm/trig_nic/trig_nic_event.h"
#include "portals.h"

using namespace SST;

class portals;
class algorithm;

struct posted_recv {
    int handle;
    int src;
    void* buffer;

    posted_recv(int handle, int src, void* buffer) {
        this->handle = handle;
        this->src = src;
        this->buffer = buffer;
    }
};

struct unex_msg {
    uint8_t* data;
    int src;
    int length;

    unex_msg(uint8_t* data, int src, int length) {
        this->data = data;
        this->src = src;
        this->length = length;
    }
};

class ptl_nic_event : public CompEvent {
public:
    ptl_nic_event(ptl_int_nic_op_t* op) : CompEvent() {
	operation = op;
    }
    ptl_nic_event() : CompEvent() {}

    ptl_int_nic_op_t* operation;
};

class trig_cpu : public Component {
public:
    friend class portals;
  
    trig_cpu(ComponentId_t id, Params_t& params);
    int Setup();
    int Finish();

    void send(int dest, uint64_t data);
    bool recv(int src, uint64_t* buf, int& handle);

    void isend(int dest, void* data, int length);
    bool irecv(int src, void* buf, int& handle);

    bool process_pending_msg();
    bool waitall();
    bool waitall(int size, int* src);

    int calcXPosition( int node );
    int calcYPosition( int node );
    int calcZPosition( int node );
    int calcNodeID(int x, int y, int z);

    int getMyId(void) { return my_id; }
    int getNumNodes(void) { return num_nodes; }

    void addBusyTime(const char* time)
    {
        busy += defaultTimeBase->convertFromCoreTime(registerTimeBase(time, false)->getFactor());
    }

    portals* getPortalsHandle(void) { return ptl; }
    int getRadix(void) { return radix; }
    int getMessageSize(void) { return msg_size; }
    int getChunkSize(void) { return chunk_size; }

private:
    trig_cpu();                      // Don't implement
    trig_cpu(const trig_cpu& c);     // Don't implement
    void operator=(trig_cpu const&); // Don't implement

    void initPortals(void);

    // This is essentially the clock function, but it is fully event
    // driven
    bool event_handler(Event* ev);

    // Next handle messages from the NIC.  The first for nonportals
    // the second for portals
    bool processEvent( Event* e );
    bool processEventPortals( Event* e );

    // This is for the ptl_link, which will be going away
    bool ptlNICHandler( Event* e );
//     bool event_portals(Event* e);

    // Hooked to nic_timing_link.  This is used to throttle the
    // bandwidth and model contention on the link to the NIC
    bool event_nic_timing(Event* e);
    bool event_dma_return(Event *e);
    bool event_pio_delay(Event* e);
    
    void wakeUp();


    bool writeToNIC(trig_nic_event* ev);
    void returnCredits(int num);
    void setTimingParams(int set);
    
    // Base state
    int my_id;
    int num_nodes;
    int state;
    int top_state;
    int x_size;
    int y_size;
    int z_size;
    int count;
    int latency;

    
    portals* ptl;
    int radix;
    int msg_size;
    int chunk_size;
	
    // State needed by send/recv/wait
    uint64_t msg_rate_delay;
    uint64_t busy;
    int outstanding_msg;
    int recv_handle;
    std::list<posted_recv*> posted_recv_q;
    std::set<int> outstanding_recv;
    std::queue<trig_nic_event*> pending_msg;
    std::list<unex_msg*> unex_msg_q;

    bool waiting;
    SimTime_t wait_start_time;

    bool blocking;
    int nic_credits;
    trig_nic_event* blocked_event;
    int blocked_busy;
  
    bool pio_in_progress;
    bool use_portals;

    int timing_set;
    int delay_host_pio_write;
    int delay_bus_xfer;
    int latency_dma_mem_access;
    int added_pio_latency;
    int recv_overhead;
    
    // Noise variables
    SimTime_t noise_interval;
    SimTime_t noise_duration;
    SimTime_t noise_count;
    int noise_runs;
    int current_run;

    Params_t    params;
    Link*       nic;
    Link*       self;
    Link*       ptl_link;
    Link*       nic_timing_link;
    bool        nic_timing_wakeup_scheduled;
    Link*       dma_return_link;
    Link*       pio_delay_link;
    int         dma_return_count;

  std::string frequency;

    algorithm *coll_algo;

    // Buffers to hold data going to NIC.  Need these to properly
    // model contention and to properly throttle bandwidth
    std::queue<trig_nic_event*> wc_buffers;
    int wc_buffers_max;
    std::queue<trig_nic_event*> dma_buffers;
    
public:
    static void
    addTimeToStats(SimTime_t time)
    {
        if ( time < min ) min = time;
        if ( time > max ) max = time;
        total_time += time;
        total_num++;
    }

    static void
    addTimeToOverallStats(SimTime_t time)
    {
        if ( time < overall_min ) overall_min = time;
        if ( time > overall_max ) overall_max = time;
        overall_total_time += time;
        overall_total_num++;
    }

    static void
    resetStats()
    {
        min = 1000000;
        max = 0;
        total_time = 0;
        total_num = 0;
    }

    static void
    printStats()
    {
        printf("Max time: %lu ns\n", (unsigned long) max);
        printf("Min time: %lu ns\n", (unsigned long) min);
        printf("Avg time: %lu ns\n", (unsigned long) (total_time/total_num));
        printf("Total num: %d\n", total_num);
    }

    static void
    printOverallStats()
    {
        printf("Overall Max time: %lu ns\n", (unsigned long) overall_max);
        printf("Overall Min time: %lu ns\n", (unsigned long) overall_min);
        printf("Overall Avg time: %lu ns\n", (unsigned long) (overall_total_time/overall_total_num));
        printf("Overall Total num: %d\n", overall_total_num);
    }

    static int
    getRand(int max)
    {
        if (!rand_init) {
            srand(0);
            rand_init = true;
        }
        if ( max == 0 ) return 0;
        return rand() % max;
    }

    static void
    addWakeUp(Link* link)
    {
        wake_up[current_link++] = link;
    }

    static void
    setTotalNodes(int total)
    {
        if ( wake_up == NULL ) wake_up = new Link*[total];
        total_nodes = total;
    }

    static void
    resetBarrier()
    {
        num_remaining = total_nodes;
    }

    static void
    barrier()
    {
        num_remaining--;
        if ( num_remaining == 0 ) {
            // Everyone has entered barrier, wake everyone up to start
            // over
            for ( int i = 0; i < total_nodes; i++ ) {
                wake_up[i]->Send(10,NULL);
            }
            resetBarrier();
            printStats();
            addTimeToOverallStats(max);
            resetStats();
        }
    }

private:
    static SimTime_t min;
    static SimTime_t max;
    static SimTime_t total_time;
    static int total_num;

    static SimTime_t overall_min;
    static SimTime_t overall_max;
    static SimTime_t overall_total_time;
    static int overall_total_num;

    static bool rand_init;

    // Infrastructure for doing more than one allreduce in a single
    // simulation
    static Link** wake_up;
    static int current_link;
    static int total_nodes;
    static int num_remaining;
};

#endif // COMPONENTS_TRIG_CPU_TRIG_CPU_H
