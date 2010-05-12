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


#include <paramUtil.h>

#define _EC_DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

EVENTCHANNEL()::EventChannel( Component& comp, 
            Component::Params_t params, std::string name, int numVC ) :
    m_component( comp ),
    m_log( *new Log<  >( "INFO EventChannel: ", false ) ),
    m_dbg( *new Log< EVENTCHANNEL_DBG >( "EventChannel::", false ) )
{
    _EC_DBG("\n");

    if ( params.find("info") != params.end() ) {
        if ( params["info"].compare("yes") == 0 ) {
            m_log.enable();
        }
    }

    bool dbgFlag = false;
    if ( params.find("debug") != params.end() ) {
        if ( params["debug"].compare("yes") == 0 ) {
            m_dbg.enable();
            dbgFlag = true;
        }
    }

    Event::Handler_t*   handler = new EventHandler< EventChannel, bool, Event* >
                       ( this, &EventChannel::handler );

    if ( ! handler ) {
        _abort( EventChannel, "new EventHandler failed\n" );
    }

    m_log.write("creating link \"%s\"\n", name.c_str());
    Link* link = comp.LinkAdd( name, handler );

    m_vcV.resize( numVC );

    int startCredit = 0;
    int threshold = 0;
    if ( params.find("initialCredit") != params.end() ) {
        startCredit = str2long( params["initialCredit"] );
    }

    for ( int i = 0; i < numVC; i++ ) {
        m_vcV[i] = new VirtChan( i, *link, comp.type, dbgFlag, 
                                            startCredit, threshold );
    }
    
    ClockHandler_t*     clockHandler;
    clockHandler = new EventHandler< EventChannel, bool, Cycle_t >
                                                ( this, &EventChannel::clock );

    std::string frequency = "1GHz";
    if ( params.find("clock") != params.end() ) {
        frequency = params["clock"];
    }

    m_log.write("frequency=%s startCredit=%d\n",frequency.c_str(),startCredit );

    TimeConverter* tc = comp.registerClock( frequency, clockHandler );
    if ( ! tc ) {
        _abort(XbarV2,"couldn't register clock handler");
    }
}

EVENTCHANNEL(bool)::clock( Cycle_t cycle )
{
    for ( unsigned int i = 0; i < m_vcV.size(); i++ ) {
        m_vcV[i]->clock( cycle );
    }
    return false;
}

EVENTCHANNEL( bool )::handler( Event* e )
{
    event_t* event = static_cast< event_t* >( e );
    if ( size_t(event->virtChan) > m_vcV.size() ) {
        _abort(EventChannel,"invalid vc=%d\n", event->virtChan );
    } 
    return m_vcV[ event->virtChan ]->handler( event );
}

EVENTCHANNEL( inline bool )::ready( int credit, int vc )
{
  if ( size_t(vc) > m_vcV.size() ) {
        _abort(EventChannel,"invalid vc=%d\n", vc );
    } 
    return m_vcV[ vc ]->ready( credit );
}

EVENTCHANNEL( inline bool )::send( eventT* event, int credit, int vc )
{
  if ( size_t(vc) > m_vcV.size() ) {
    _abort(EventChannel,"invalid vc=%d\n", vc );
  } 
  return m_vcV[ vc ]->send( event, credit );
}

EVENTCHANNEL( inline bool )::recv( eventT** event, int vc  )
{
  if ( size_t(vc) > m_vcV.size() ) {
        _abort(EventChannel,"invalid vc=%d\n", vc );
    } 
    return m_vcV[ vc ]->recv( event );
}

EVENTCHANNEL()::VirtChan::VirtChan( int vc, Link& link, std::string& name, 
                                bool dbgFlag, int startCredit, int threshold ) :
    m_vc( vc ),
    m_link( link ),
    m_creditAvail( startCredit ),
    m_creditFreed( 0 ),
    m_creditThreshold( threshold ),
    m_name( name ),
    m_dbg( *new Log< EVENTCHANNEL_DBG > ("EventChannel::VirtChan::", dbgFlag) )
{
    _EC_DBG("avail=%d thres=%d\n", m_creditAvail, m_creditThreshold );
}

EVENTCHANNEL( inline bool )::VirtChan::clock( Cycle_t cycle )
{
    if ( ! m_outQ.empty() ) {
        _EC_DBG("%s: cycle=%lu send, event=%p\n", m_name.c_str(),
                            cycle,m_outQ.front() );

        m_link.Send( 0, m_outQ.front() );
        m_outQ.pop_front();
    }

    if ( m_creditFreed > m_creditThreshold ) 
    {
        event_t* event = new event_t;

        event->type   = event_t::CREDIT;
        event->credit = m_creditFreed;
        event->virtChan = m_vc;

        m_creditFreed = 0;

        _EC_DBG("%s: cycle=%lu send %d credits\n", 
                    m_name.c_str(), cycle, event->credit);
        m_link.Send( 0, event );
    }
    return false;
}

EVENTCHANNEL( inline bool )::VirtChan::handler( event_t* event )
{
    if ( event->type == event_t::EVENT ) {
        _EC_DBG("%s: got event\n", m_name.c_str() );
        m_inQ.push_back( event );
    } else if ( event->type == event_t::CREDIT ) {
        m_creditAvail += event->credit;
        _EC_DBG("%s: got %d credits now avail %d\n",
                     m_name.c_str(), event->credit, m_creditAvail );
        delete event;
    } else {
        _abort( WireLink, "bad event type %d\n", event->type );
    }
    return false;
}

EVENTCHANNEL( inline bool )::VirtChan::ready( int credit )
{
   _EC_DBG("%s: credit=%d m_createAvail=%d\n", m_name.c_str(),
                                    credit, m_creditAvail);
   return ( (int)m_creditAvail >=  credit );
}

EVENTCHANNEL( inline bool )::VirtChan::send( eventT* event, int credit )
{
  if ( (int)m_creditAvail < credit ) {
        _EC_DBG("%s: failed, credit=%d createAvail=%d\n", m_name.c_str(),
                                    credit, m_creditAvail);
        return false;
    }
    _EC_DBG("%s: need credit=%d createAvail=%d\n", m_name.c_str(),
                                    credit, m_creditAvail);

    event_t* cEvent = new event_t;
    cEvent->virtChan = m_vc;
    cEvent->event  = event;
    cEvent->type   = event_t::EVENT;
    cEvent->credit = credit;

    m_outQ.push_back( cEvent );

    m_creditAvail -= credit;

    return true;
}

EVENTCHANNEL( inline bool )::VirtChan::recv( eventT** event )
{
    if ( m_inQ.empty() ) {
        return false;
    }
    *event = m_inQ.front()->event;
    
    m_creditFreed += m_inQ.front()->credit;
    _EC_DBG("%s: creditFreed=%d\n",m_name.c_str(), m_creditFreed);

    delete m_inQ.front();
    m_inQ.pop_front();

    return true;
}
