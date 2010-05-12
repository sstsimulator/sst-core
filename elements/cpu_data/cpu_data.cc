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
#include "cpu_data.h"

#include <sst/memEvent.h>


bool Cpu_data::clock( Cycle_t current)
{
    //_CPU_DATA_DBG("id=%lu currentCycle=%lu \n", Id(), current );

    MemEvent* event = NULL; 

    if (Id() == 2)
        mycore_temperature = 360;
    else
	mycore_temperature = 300;

    if ( state == SEND ) { 
        if ( ! event ) event = new MemEvent();

        if ( who == WHO_MEM ) { 
            event->address = 0x1000; 
            who = WHO_NIC;
        } else {
            event->address = 0x10000000; 
            who = WHO_MEM;
        }

        _CPU_DATA_DBG("send a MEM event address=%#lx\n", event->address );
	//printf("send a MEM event address=%#lx at cycle=%lu\n", event->address, current );
	

        mem->Send( (Cycle_t)3, event );
        state = WAIT;


    } else {
        if ( ( event = static_cast< MemEvent* >( mem->Recv() ) ) ) {
            _CPU_DATA_DBG("got a MEM event address=%#lx\n", event->address);
	    //printf("got a MEM event address=%#lx at cycle=%lu\n", event->address, current );


            state = SEND;
	    if (Id() == 2){
	       counts++;
	       num_il1_read++;
	       num_branch_read = num_branch_read + 2;
	       num_RAS_read = num_RAS_read + 2;
	    }
	    else{
		counts = counts+2;
		num_il1_read = num_il1_read +2;
	    }
        }
    }
    return false;
}

bool Cpu_data::pushData( Cycle_t current)
{    		
	  if(isTimeToPush(current, pushIntrospector.c_str())){
	    //_CPU_DATA_DBG("id=%lu currentCycle=%lu \n", Id(), current );

	    //Here you can push power statistics by 1) set up values in the mycounts structure
	    //and 2) call the gerPower function. See cpu_PowerAndData for example
	}
	return false;
}








extern "C" {
Cpu_data* cpu_dataAllocComponent( SST::ComponentId_t id,
                                    SST::Component::Params_t& params )
{
    return new Cpu_data( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT2
BOOST_CLASS_EXPORT(Cpu_data)

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Cpu_data, bool, SST::Cycle_t, SST::Time_t )
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Cpu_data, bool, SST::Cycle_t)
#endif

