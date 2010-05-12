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


#ifndef _DUMMY_H
#define _DUMMY_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>

using namespace SST;

#if DBG_DUMMY
#define _DUMMY_DBG( fmt, args...)\
         printf( "%d:Dummy::%s():%d: "fmt, _debug_rank,__FUNCTION__,__LINE__, ## args )
#else
#define _DUMMY_DBG( fmt, args...)
#endif

class Dummy : public Component {

    public:
        Dummy( ComponentId_t id, Params_t& params ) :
            Component( id  ), 
            params( params ),
            frequency( "2.2GHz" )
        { 
            _DUMMY_DBG("new id=%lu\n",id);

            Params_t::iterator it = params.begin();
            while( it != params.end() ) {
                _DUMMY_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("clock") ) {
/*                     sscanf( it->second.c_str(), "%f", &frequency ); */
                    frequency = it->second;
                }
                ++it;
            }


/*             eventHandler = new EventHandler< Dummy, bool, Time_t, Event* > */
/*                                                 ( this, &Dummy::processEvent ); */

            eventHandler = new EventHandler< Dummy, bool, Event* >
                                                ( this, &Dummy::processEvent );

            cpu= LinkAdd( "port0" );
            nic= LinkAdd( "port1", eventHandler );

/*             clockHandler = new EventHandler< Dummy, bool, Cycle_t, Time_t > */
/*                                                 ( this, &Dummy::clock ); */

            clockHandler = new EventHandler< Dummy, bool, Cycle_t >
                                                ( this, &Dummy::clock );

            registerClock( frequency, clockHandler );
        }

    private:

        Dummy( const Dummy& c );
/*         bool clock( Cycle_t, Time_t  ); */
/*         bool processEvent( Time_t, Event*  ); */
        bool clock( Cycle_t );
        bool processEvent( Event*  );

        ClockHandler_t* clockHandler;
        Event::Handler_t* eventHandler;

        Params_t    params;
        Link*       cpu;
        Link*       nic;
	std::string frequency;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            _AR_DBG(Dummy,"start\n");
            boost::serialization::
                void_cast_register(static_cast<Dummy*>(NULL), 
                                   static_cast<Component*>(NULL));
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Component );
            ar & BOOST_SERIALIZATION_NVP( cpu );
            ar & BOOST_SERIALIZATION_NVP( nic );
            ar & BOOST_SERIALIZATION_NVP( clockHandler );
            ar & BOOST_SERIALIZATION_NVP( eventHandler );
            _AR_DBG(Dummy,"done\n");
        }
        template<class Archive>
        friend void save_construct_data(Archive & ar, 
                                        const Dummy * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Dummy,"\n");
            ComponentId_t     id     = t->_id;
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP( id );
            ar << BOOST_SERIALIZATION_NVP( params );
        } 

        template<class Archive>
        friend void load_construct_data(Archive & ar, 
                                        Dummy * t, 
                                        const unsigned int file_version)
        {
            _AR_DBG(Dummy,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP( id );
            ar >> BOOST_SERIALIZATION_NVP( params );
            ::new(t)Dummy( id, params );
        } 
};

#endif
