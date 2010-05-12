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


// Copyright 2007 Sandia Corporation. Under the terms
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

#include <unistd.h>
#include "event_ping_pong.h"
#include <sst/cpunicEvent.h>


bool
event_ping_pong::handle_component_events(Event *event)
{

//     _EVENT_PING_PONG_DBG("Component %lu got event %d at %.9f\n", Id(), total_events, time);
    _EVENT_PING_PONG_DBG("Component %lu got event %d at FIXME\n", Id(), total_events);

    if (total_events == 0)   {
	// Measure from the first event
	start_tick= getticks();
	_EVENT_PING_PONG_DBG("First time here, setting start_tick to %d\n",
	    static_cast<int>(start_tick));
    }

    total_events++;
    if ((total_events >= max_events) && (Id() == 0))   {
	double t;
	stop_tick= getticks();
	t= (stop_tick - start_tick) * ticks_to_sec_factor;
	fprintf(stderr, "Time to send and receive %d events was %.9f seconds\n", total_events, t);
	fprintf(stderr, "    %.0f events per second\n", 2.0 * total_events / t);
    } else   {
	// Send it back
	port->Send(0.000000001000, static_cast<CPUNicEvent *>(event));
    }

    return false;
}


extern "C" {
event_ping_pong *
event_ping_pongAllocComponent(SST::ComponentId_t id, 
                              SST::Component::Params_t &params)
{
    return new event_ping_pong(id, params);
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(event_ping_pong)

// BOOST_CLASS_EXPORT_TEMPLATE4(SST::EventHandler,
//     event_ping_pong, bool, SST::Time_t, SST::Event *)
BOOST_CLASS_EXPORT_TEMPLATE3(SST::EventHandler,
    event_ping_pong, bool, SST::Event *)
#endif
