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


#ifndef _EVENT_CHANNEL_H
#define _EVENT_CHANNEL_H

#include <sst/component.h>
#include <sst/compEvent.h>
#include <sst/link.h>
#include <sst/log.h>

#ifndef EVENTCHANNEL_DBG
#define EVENTCHANNEL_DBG 0 
#endif

using namespace SST;

template < typename eventT >
class ChannelEvent : public CompEvent {
    public:
        typedef enum { CREDIT, EVENT } type_t;
        type_t      type;
        uint32_t    credit;
        eventT*     event;
        int         virtChan;
};

template < typename eventT >
class EventChannel 
{
        typedef ChannelEvent<eventT>    event_t;

    public: // functions

        virtual ~EventChannel() {;}
        EventChannel( Component&, Component::Params_t, 
                                    std::string name, int numVC = 1 );

        virtual bool ready( int credits, int vc = 0 );
        virtual bool send( eventT*, int credits, int vc = 0 );
        virtual bool recv( eventT**, int vc = 0);

    private: // functions

        EventChannel( const EventChannel< eventT >& );
        bool handler( Event* );
        bool clock( Cycle_t );

    private:
        class VirtChan {
                typedef std::deque< ChannelEvent< eventT >* > que_t;
            public: 
                VirtChan( int vc, Link& link, std::string& name, bool dbgFlag,
                        int startCredit = 1, int threshold = 0 );
                bool clock( Cycle_t );
                bool handler( event_t* );
                bool ready( int credits );
                bool send( eventT*, int credits );
                bool recv( eventT** );

            private:
                int         m_vc;
                Link&       m_link;
                uint32_t    m_creditAvail; 
                uint32_t    m_creditFreed; 
                uint32_t    m_creditThreshold;
                que_t       m_inQ;
                que_t       m_outQ;
                std::string& m_name;

                Log< EVENTCHANNEL_DBG >&    m_dbg;
        };

    private: // data

        Component&                  m_component;
        std::vector< VirtChan* >    m_vcV;

        Log<>&                      m_log;        
        Log< EVENTCHANNEL_DBG >&    m_dbg;
};

#define EVENTCHANNEL( retType ) \
template < typename eventT >\
retType EventChannel< eventT >\

#include <eventChannel2.h>

#endif
