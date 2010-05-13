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

#include "routermodel.h"
#include <sst/cpunicEvent.h>
#include <sst/eventHandler1Arg.h>

int router_model_debug;

bool
Routermodel::handle_port_events(Event *event, int in_port)
{

SimTime_t current_time;
SimTime_t delay;
std::vector<uint8_t>::iterator itNum;
uint8_t out_port;


    current_time= getCurrentSimTime();
    _ROUTER_MODEL_DBG(3, "Router %s got an event from port %d at time %lld\n", component_name.c_str(),
	in_port, (long long int)current_time);
    CPUNicEvent *e= static_cast<CPUNicEvent *>(event);
    port[in_port].cnt_in++;

    /* Diagnostic: print the route this event is taking */
    printf("Event route (currently at %d): ", e->hops);
    for(itNum = e->route.begin(); itNum < e->route.end(); itNum++)   {
	printf("%d, ", *itNum);
    }
    printf(" destination\n");

    out_port= e->route[e->hops];
    e->hops++;
    delay= hop_delay;

    _ROUTER_MODEL_DBG(3, "Sending message out on port %d\n", out_port);
    port[out_port].link->Send(delay, e);
    _ROUTER_MODEL_DBG(3, "Returning from handle_port_events\n");


#ifdef rrr
    if (e->routeX > 0)   {
	// Keep going East
	e->routeX--;
	e->hops++;
	if (current_time > next_East_slot)   {
	    // Output channel is clear; send now
	    next_East_slot= current_time;
	}
	// Cumulative delay on output ports of routers
	delay= next_East_slot - current_time;
	e->router_delay += delay;
	// How long will this message occupy the output channel?
	link_time= (e->msg_len / link_bw) * 1000000000.0;
	_ROUTER_MODEL_DBG(3, "Router %s: Sending message East. x %d, y %d, hops %d, delay %12d\n",
	    component_name.c_str(), e->routeX, e->routeY, e->hops, (int)delay);
	east_port->Send(delay, e);
	next_East_slot += link_time;
    }
#endif

    return false;

}  /* end of handle_port_events() */


Link *
Routermodel::initPort(int port, char *link_name)
{

Event::Handler_t *tmpHandler;

    tmpHandler= new EventHandler1Arg< Routermodel, bool, Event *, int >
	(this, &Routermodel::handle_port_events, port);

    if (!tmpHandler)   {
        _abort(Routermodel,"Couldn't create eventHandler\n");
    }

    return LinkAdd(link_name, tmpHandler);

}  /* end of initPort() */



extern "C" {
Routermodel *
routermodelAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Routermodel(id, params);
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(Routermodel)

// BOOST_CLASS_EXPORT_TEMPLATE4(SST::EventHandler,
//     Routermodel, bool, SST::Time_t, SST::Event *)

BOOST_CLASS_EXPORT_TEMPLATE3(SST::EventHandler,
    Routermodel, bool, SST::Event *)
#endif
