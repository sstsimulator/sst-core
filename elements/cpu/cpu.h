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

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include <sst/timeConverter.h>

using namespace SST;

#if DBG_CPU
#define _CPU_DBG( fmt, args...)\
         printf( "%d:Cpu::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _CPU_DBG( fmt, args...)
#endif

class Cpu : public Component {
        typedef enum { WAIT, SEND } state_t;
        typedef enum { WHO_NIC, WHO_MEM } who_t;
    public:
        Cpu( ComponentId_t id, Params_t& params ) :
            Component( id ),
            params( params ),
            state(SEND),
            who(WHO_MEM), 
            frequency( "2.2GHz" )
        {
            _CPU_DBG( "new id=%lu\n", id );

            registerExit();

            Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _CPU_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("clock") ) {
/*                     sscanf( it->second.c_str(), "%f", &frequency ); */
		    frequency = it->second;
                }    
                ++it;
            } 
            
            mem = LinkAdd( "MEM" );
/*             handler = new EventHandler< Cpu, bool, Cycle_t, Time_t > */
/*                                                 ( this, &Cpu::clock ); */
            handler = new EventHandler< Cpu, bool, Cycle_t >
                                                ( this, &Cpu::clock );
            _CPU_DBG("-->frequency=%s\n",frequency.c_str());
            TimeConverter* tc = registerClock( frequency, handler );
	    printf("CPU period: %ld\n",tc->getFactor());
            _CPU_DBG("Done registering clock\n");
            
        }
        int Setup() {
            _CPU_DBG("\n");
            return 0;
        }
        int Finish() {
            _CPU_DBG("\n");
            return 0;
        }

    private:

        Cpu( const Cpu& c );
	Cpu() {}

/*         bool clock( Cycle_t, Time_t ); */
        bool clock( Cycle_t );
        ClockHandler_t* handler;
        bool handler1( Time_t time, Event *e );

        Params_t    params;
        Link*       mem;
        state_t     state;
        who_t       who;
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
/*         SAVE_CONSTRUCT_DATA( Cpu ) { */
/* 	    printf("cpu::save_construct_data\n"); */
/*             _AR_DBG( Cpu, "\n" ); */

/*             ComponentId_t   id     = t->_id; */
/* /\*             Clock*          clock  = t->_clock; *\/ */
/*             Simulation*     sim  = t->simulation; */
/*             Params_t        params = t->params; */

/*             ar << BOOST_SERIALIZATION_NVP( id ); */
/*             ar << BOOST_SERIALIZATION_NVP( sim ); */
/*             ar << BOOST_SERIALIZATION_NVP( params ); */
/*         }  */
/*         LOAD_CONSTRUCT_DATA( Cpu ) { */
/* 	    printf("cpu::load_construct_data\n"); */
/*             _AR_DBG( Cpu, "\n" ); */

/*             ComponentId_t   id; */
/*             Simulation*     sim; */
/*             Params_t        params; */

/*             ar >> BOOST_SERIALIZATION_NVP( id ); */
/*             ar >> BOOST_SERIALIZATION_NVP( sim ); */
/*             ar >> BOOST_SERIALIZATION_NVP( params ); */

/*             ::new(t)Cpu( id, sim, params ); */
/*         } */
#endif
};

#endif
