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


#ifndef _XBARSH_H
#define _XBARSH_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include <sst/factory.h>
#include <sst/simulation.h>
#include "dummy.h"
#include "xbar.h"

using namespace SST;

#if DBG_XBARSH
#define _XBARSH_DBG( fmt, args...)\
         printf( "%d:XbarShell::%s():%d: "fmt, _debug_rank,__FUNCTION__,__LINE__, ## args )
#else
#define _XBARSH_DBG( fmt, args...)
#endif

class XbarShell : public Component {

    public:
        XbarShell( ComponentId_t id, Params_t& params ) :
            Component( id ), 
            params( params )
        { 
            _XBARSH_DBG("new id=%lu\n",id);

            factory = Simulation::getSimulation()->getFactory();
            xbar = factory->Create( 0,"xbar", params );

            Params_t dummy_params;
            dummy = new Dummy(0,dummy_params);
           
            Connect(dummy,"port1",xbar,"port0");

            Link* tmp;

            // create a link that the core can use for wire up 
            tmp = LinkAdd( "port0", dummy->LinkGet("port0") );
            if ( ! tmp ) _abort(XbarShell,"\n");

            // create a link that the core can use for wire up 
            tmp = LinkAdd( "port1", xbar->LinkGet("port1") );
            if ( ! tmp ) _abort(XbarShell,"\n");
        }

    private:

        XbarShell( const XbarShell& c );
        bool clock( Cycle_t, Time_t  );
        bool processEvent( Time_t, Event*  );

        Dummy*      dummy;
        Component*  xbar;
        Params_t    params;
        Factory*    factory;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            _AR_DBG(XbarShell,"start\n");
            boost::serialization::
                void_cast_register(static_cast<XbarShell*>(NULL), 
                                   static_cast<Component*>(NULL));
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Component );
            ar & BOOST_SERIALIZATION_NVP( factory );
            ar & BOOST_SERIALIZATION_NVP( dummy );
            ar & BOOST_SERIALIZATION_NVP( xbar );
            _AR_DBG(XbarShell,"done\n");
        }
        template<class Archive>
        friend void save_construct_data(Archive & ar, 
                                        const XbarShell * t, 
                                        const unsigned int file_version)
        {
            _AR_DBG(XbarShell,"\n");
            ComponentId_t     id     = t->_id;
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP( id );
            ar << BOOST_SERIALIZATION_NVP( params );
        } 
        template<class Archive>
        friend void load_construct_data(Archive & ar, 
                                        XbarShell * t, 
                                        const unsigned int file_version)
        {
            _AR_DBG(XbarShell,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP( id );
            ar >> BOOST_SERIALIZATION_NVP( params );
            ::new(t)XbarShell( id, params );
        } 
};

#endif
