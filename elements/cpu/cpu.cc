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

#include "cpu.h"
#include <sst/timeConverter.h>
#include "myMemEvent.h"

BOOST_CLASS_EXPORT( SST::MyMemEvent )
//BOOST_IS_MPI_DATATYPE( SST::MyMemEvent )

// bool Cpu::clock( Cycle_t current, Time_t epoch )
bool Cpu::clock( Cycle_t current )
{
    //_CPU_DBG("id=%lu currentCycle=%lu \n", Id(), current );

//     printf("In CPU::clock\n");
    MyMemEvent* event = NULL; 

    if (current == 100 ) unregisterExit();

    if ( state == SEND ) { 
        if ( ! event ) event = new MyMemEvent();

        if ( who == WHO_MEM ) { 
            event->address = 0x1000; 
            who = WHO_NIC;
        } else {
            event->address = 0x10000000; 
            who = WHO_MEM;
        }

        _CPU_DBG("xxx: send a MEM event address=%#lx @ cycle %ld\n", event->address, current );
// 	mem->Send( 3 * epoch, event );
	mem->Send( (Cycle_t)3, event );
// 	printf("CPU::clock -> setting state to WAIT\n");
        state = WAIT;
    } else {
// 	printf("Entering state WAIT\n");
        if ( ( event = static_cast< MyMemEvent* >( mem->Recv() ) ) ) {
// 	    printf("Got a mem event\n");
	  _CPU_DBG("xxx: got a MEM event address=%#lx @ cycle %ld\n", event->address, current );
// 	  printf("CPU::clock -> setting state to SEND\n");
            state = SEND;
        }
    }
    return false;
}

extern "C" {
Cpu*
cpuAllocComponent( SST::ComponentId_t id, 
                   SST::Component::Params_t& params )
{
//     printf("cpuAllocComponent--> sim = %p\n",sim);
    return new Cpu( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(Cpu)

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Cpu, bool, SST::Cycle_t, SST::Time_t )
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Cpu, bool, SST::Cycle_t)
#endif
