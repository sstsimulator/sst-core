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
#include <sst/log.h>
#include <memoryChannel.h>
#include <memMap.h>

#ifndef XBARV2_DBG
#define XBARV2_DBG 0
#endif

using namespace SST;

class XbarV2 : public Component
{
        class Port;
        typedef MemoryChannel< uint64_t, Port* >       memChan_t;
        typedef memChan_t::addr_t       addr_t;
        typedef memChan_t::event_t      event_t;
        typedef memChan_t::cookie_t     cookie_t;
        typedef uint64_t                length_t;

        typedef MemMap< addr_t, length_t, Port* >  memMap_t;

        struct Entry {
            event_t*    event;
            Cycle_t     timeStamp;
            cookie_t    cookie;
        }; 

        typedef std::vector< std::vector< Entry > >    entryV_t;

        class Port {

            public:
                Port( Component&, memMap_t&, entryV_t&,  
                            int numPorts, int portNum, 
                            addr_t addr = 0, length_t length = 0,
                            bool enableDbg = false );
                void doInput( Cycle_t );
                void doOutput( );

            private:
                Port();
                Port( const Port& );

                Port* findDstPort( addr_t addr );

            private:

                memChan_t*              m_memChan;
                int                     m_portNum;
                int                     m_numPorts;
                memMap_t&               m_memMap;
                event_t*                m_curEvent;
                cookie_t                m_curCookie;
                entryV_t&               m_entryV;
                Log< XBARV2_DBG >&      m_dbg;
        };

    public: // functions

        XbarV2( ComponentId_t id, Params_t& params );

    private: // functions

	    XbarV2();
        XbarV2( const XbarV2& c );

        bool clock( Cycle_t );
        void initPort( int port, Params_t& );
	
    private: // data

        std::vector<Port *>     m_portInfoV;
        int                     m_numPorts;
        memMap_t                m_memMap;
        entryV_t                m_entryV;
        Log< XBARV2_DBG >&      m_dbg;
        Log<>&                  m_log;
};

#endif
