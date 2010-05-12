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


#ifndef _BUS_H
#define _BUS_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/log.h>
#include <memoryChannel.h>
#include <memMap.h>

#ifndef BUS_DBG
#define BUS_DBG 0
#endif

using namespace SST;

class Bus : public Component
{
    private: // types
        class Device;
        typedef MemoryChannel< uint64_t, Device* >     devChan_t;
        typedef devChan_t::event_t      event_t;
        typedef devChan_t::addr_t       addr_t;
        typedef devChan_t::cookie_t     cookie_t;
        typedef uint64_t                length_t;
        typedef MemMap< addr_t, length_t, Device* >  memMap_t; 

        class Device {

            public:
                Device( Component& comp, std::string name, Params_t ); 
                void clock( Cycle_t );
                bool send( event_t*, cookie_t );
                Cycle_t timeStamp();
                event_t* getReq();
                Bus::event_t::reqType_t doResp();

            private:
                typedef std::pair< event_t*, cookie_t >  respE_t;
                typedef std::pair< event_t*, Cycle_t >   reqE_t;

                reqE_t                  m_req;
                devChan_t*              m_devChan;
                Log< BUS_DBG >&         m_dbg;
                std::string             m_name;
        }; 

        struct AtBat {
            bool        valid;
            event_t*    event;
            Device*     srcDev;
            Device*     dstDev;
        }; 

    public: // functions

        Bus( ComponentId_t id, Params_t& params );
        void initDevices( Params_t& params );
        void initDevice( std::string name, Params_t& params );

    private: // functions

	    Bus();
        Bus( const Bus& c );

        bool clock( Cycle_t );
        bool doRead( event_t*, Device* src, Device* dst );
        bool doWrite( event_t*, Device* src, Device* dst );
	
    private: // data

        bool                    m_readBusy;
        bool                    m_writeBusy;
        memMap_t                m_memMap;
        AtBat                   m_atBat;
        Log< BUS_DBG >&         m_dbg;
        Log<>&                  m_log;
};

#endif
