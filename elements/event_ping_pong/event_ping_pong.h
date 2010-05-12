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


#ifndef _EVENT_PING_PONG_H
#define _EVENT_PING_PONG_H

#include "sst_config.h"

#include "cycle.h"
#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include <sst/cpunicEvent.h>

using namespace SST;


#if DBG_EVENT_PING_PONG
#define _EVENT_PING_PONG_DBG( fmt, args...)\
         fprintf(stderr, "%d:event_ping_pong::%s():%d: "fmt, _debug_rank, __FUNCTION__, __LINE__, ## args)
#else
#define _EVENT_PING_PONG_DBG(fmt, args...)
#endif

class event_ping_pong : public Component {
    public:
        event_ping_pong(ComponentId_t id, Params_t &params) :
            Component(id),
            params(params)
        {
            printf("Event ping pong component %lu is on rank %d\n", id, _debug_rank);
            _EVENT_PING_PONG_DBG("new id=%lu\n", id);
	    
            Params_t::iterator it= params.begin();
	    // Defaults
	    max_events= 1000;
	    calibration_loop= 2;

            while (it != params.end())   {
                _EVENT_PING_PONG_DBG("key=%s value=%s\n", it->first.c_str(),
		    it->second.c_str());
                if (!it->first.compare("max_events"))   {
		    sscanf(it->second.c_str(), "%d", &max_events);
		    _EVENT_PING_PONG_DBG("Setting max_events to %d\n", max_events);
		}

                if (!it->first.compare("calibration_loop"))   {
		    sscanf(it->second.c_str(), "%d", &calibration_loop);
		    _EVENT_PING_PONG_DBG("Setting calibration_loop to %d\n", calibration_loop);
		}
                ++it;
            }

	    // Calibrate the clock 
	    if (id == 0)   {
		printf("We will send %d events from component 0 to 1\n", max_events);
		printf("Calibrating clock...\n");

#ifdef HAVE_CLOCK_GETTIME
		struct timespec tm1, tm2;
		double t;

		clock_gettime(CLOCK_REALTIME, &tm1);
		start_tick= getticks();
		for (int i= 0; i < 1000 * calibration_loop; i++)   {
		    usleep(1000);
		}
		stop_tick= getticks();
		clock_gettime(CLOCK_REALTIME, &tm2);
		t= (tm2.tv_sec + tm2.tv_nsec / 1000000000.0) - (tm1.tv_sec + tm1.tv_nsec / 1000000000.0);

		ticks_to_sec_factor= t / elapsed(stop_tick, start_tick);
		fprintf(stderr, "    ... was this %.3f seconds? Wanted %ds. Counted %llu ticks\n",
		    t, calibration_loop, stop_tick - start_tick);
#else
                fprintf(stderr, "clock_gettime not supported - no alternative provided\n");
                abort();
#endif 
	    }


	    // Install a handler for the receiver
	    Event::Handler_t *handler;
/* 	    handler= new EventHandler<event_ping_pong, bool, Time_t, Event *> */
	    handler= new EventHandler<event_ping_pong, bool, Event *>
		(this, &event_ping_pong::handle_component_events);

	    // Create the link between component 0 and 1
	    port= LinkAdd("my_link", handler);
	    if (port == NULL)   {
		_EVENT_PING_PONG_DBG("This component expects a link to another component named \"my_link\"\n");
		_ABORT(event_ping_pong, "Check the input XML file!\n");
	    } else   {
		_EVENT_PING_PONG_DBG("Added the \"my_link\" link\n");
	    }

	    _EVENT_PING_PONG_DBG("Component is initialized and ready\n");
        }

	// This happens after the wire-up
        int Setup() {
            _EVENT_PING_PONG_DBG("Initializing.\n");
	    total_events= 0;
	    start_tick= 0;

	    // Component 0 sends the first event...
	    if (Id() == 0)   {
		CPUNicEvent* event;
		event= new CPUNicEvent();
		_EVENT_PING_PONG_DBG("Component %lu: sending first event to other component\n", Id());
		port->Send(0.000000001000, event);
	    }

            return 0;
        }

        int Finish() {
            _EVENT_PING_PONG_DBG("Finishing.\n");
            return 0;
        }

    private:

        event_ping_pong(const event_ping_pong &c);

        bool clock(Cycle_t, Time_t);
/* 	bool handle_component_events(Time_t, Event *); */
	bool handle_component_events(Event *);

        Params_t params;
	Link *port;
	int total_events;
	int max_events;
	int calibration_loop;
	double ticks_to_sec_factor;
	ticks start_tick;
	ticks stop_tick;

#if WANT_CHECKPOINT_SUPPORT
        BOOST_SERIALIZE {
            _AR_DBG(event_ping_pong, "start\n" );
            BOOST_VOID_CAST_REGISTER(event_ping_pong *, Component *);
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(port);
            _AR_DBG(event_ping_pong, "done\n" );
        }

        SAVE_CONSTRUCT_DATA(event_ping_pong) {
            _AR_DBG(event_ping_pong, "\n");

	    ComponentId_t   id= t->_id;
            Params_t        params= t->params;

	    ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        } 

        LOAD_CONSTRUCT_DATA(event_ping_pong) {
            _AR_DBG(event_ping_pong, "\n");

	    ComponentId_t   id;
            Params_t        params;

	    ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);

            ::new(t)event_ping_pong(id, params);
        }
#endif
};

#endif
