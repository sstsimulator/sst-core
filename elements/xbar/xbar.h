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


#ifndef _XBAR_H
#define _XBAR_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>

using namespace SST;

#if DBG_XBAR
#define _XBAR_DBG( fmt, args...)\
         printf( "%d:Xbar::%s():%d: "fmt, _debug_rank,__FUNCTION__,__LINE__, ## args )
#else
#define _XBAR_DBG( fmt, args...)
#endif

class Xbar : public Component {

    public:
        Xbar( ComponentId_t id, Params_t& params ) :
            Component( id ), 
            params( params ),
            frequency( "2.2GHz" )
        { 
            _XBAR_DBG("new id=%lu\n",id);

/*             Params_t::iterator it = params.begin(); */
/*             while( it != params.end() ) { */
/*                 _XBAR_DBG("key=%s value=%s\n", */
/*                             it->first.c_str(),it->second.c_str()); */
/*                 if ( ! it->first.compare("clock") ) { */
/*                     sscanf( it->second.c_str(), "%f", &frequency ); */
/*                 } */
/*                 ++it; */
/*             } */
	    if ( params.find("clock") != params.end() ) {
/* 		sscanf( params["clock"].c_str(), "%f", &frequency ); */
		frequency = params["clock"];
	    }

/*             eventHandler = new EventHandler< Xbar, bool, Time_t, Event* > */
/*                                                 ( this, &Xbar::processEvent ); */

            eventHandler = new EventHandler< Xbar, bool, Event* >
                                                ( this, &Xbar::processEvent );

            cpu = LinkAdd( "port0" );
            nic = LinkAdd( "port1", eventHandler );
	    selfPush = selfLink("selfPush",new EventHandler<Xbar, bool, Event*>(this,&Xbar::selfEvent));
	    selfPull = selfLink("selfPull");
	    
/*             clockHandler = new EventHandler< Xbar, bool, Cycle_t, Time_t > */
/*                                                 ( this, &Xbar::clock ); */

            clockHandler = new EventHandler< Xbar, bool, Cycle_t >
                                                ( this, &Xbar::clock );

            TimeConverter* tc = registerClock( frequency, clockHandler );
            _XBAR_DBG("Done registering clock\n",id);
        }

    private:

	Xbar() {}
        Xbar( const Xbar& c );
/*         bool clock( Cycle_t, Time_t  ); */
        bool clock( Cycle_t );
/*         bool processEvent( Time_t, Event*  ); */
        bool processEvent( Event*  );
	bool selfEvent( Event*);
	
        ClockHandler_t* clockHandler;
        Event::Handler_t* eventHandler;

        Params_t    params;
        Link*       cpu;
        Link*       nic;
	Link*       selfPush;
	Link*       selfPull;
	std::string frequency;

#if WANT_CHECKPOINT_SUPPORT
        BOOST_SERIALIZE {
	    printf("xbaf::serialize()\n");
            _AR_DBG(Xbar,"start\n");
            BOOST_VOID_CAST_REGISTER( Xbar*, Component* );
	    printf("  base serializing: Component\n");
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Component );
	    printf("  serializing: cpu\n");
            ar & BOOST_SERIALIZATION_NVP( cpu );
	    printf("  serializing: nic\n");
            ar & BOOST_SERIALIZATION_NVP( nic );
	    printf("  serializing: clockHandler\n");
            ar & BOOST_SERIALIZATION_NVP( clockHandler );
	    printf("  serializing: eventHandler\n");
            ar & BOOST_SERIALIZATION_NVP( eventHandler );
            _AR_DBG(Xbar,"done\n");
        }
/*         SAVE_CONSTRUCT_DATA( Xbar ) { */
/*             _AR_DBG(Xbar,"\n"); */
/*             ComponentId_t     id     = t->_id; */
/*             Simulation*       sim    = t->simulation; */
/*             Params_t          params = t->params; */
/*             ar << BOOST_SERIALIZATION_NVP( id ); */
/*             ar << BOOST_SERIALIZATION_NVP( sim ); */
/*             ar << BOOST_SERIALIZATION_NVP( params ); */
/*         } */
/*         LOAD_CONSTRUCT_DATA( Xbar ) { */
/*             _AR_DBG(Xbar,"\n"); */
/*             ComponentId_t     id; */
/*             Simulation*       sim; */
/*             Params_t          params; */
/*             ar >> BOOST_SERIALIZATION_NVP( id ); */
/*             ar >> BOOST_SERIALIZATION_NVP( sim ); */
/*             ar >> BOOST_SERIALIZATION_NVP( params ); */
/*             ::new(t)Xbar( id, sim, params ); */
/*         } */
#endif
};

#endif
