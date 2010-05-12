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

#include <iostream>
#include <sstream>

#include "xbarV2.h"
#include <paramUtil.h>

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

XbarV2::XbarV2( ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_numPorts( 2 ),
    m_dbg( *new Log< XBARV2_DBG >( "XbarV2::", false ) ),
    m_log( *new Log<>( "INFO XbarV2: ", false ) )
{
    if ( params.find( "info" ) != params.end() ) {
        if ( params[ "info" ].compare( "yes" ) == 0 ) {
            m_log.enable();
        }
    }

    if ( params.find( "debug" ) != params.end() ) {
        if ( params[ "debug" ].compare( "yes" ) == 0 ) {
            m_dbg.enable();
        }
    }

    DBG("new id=%lu\n",id);

    if ( params.find("numPorts") != params.end() ) {
        m_numPorts = str2long( params["numPorts"] );
    }

    m_portInfoV.resize( m_numPorts );
    m_entryV.resize( m_numPorts );

    for ( int i = 0; i < m_numPorts; i++ ) {
        m_entryV[ i ].resize( m_numPorts );
        for ( int j = 0; j < m_numPorts; j++ ) {
            m_entryV[ i ][ j ].event = NULL;
        }
    }

    for ( int port = 0; port < m_numPorts; port++ ) {
        initPort( port, params );
    }

    ClockHandler_t* clockHandler = new EventHandler< XbarV2, bool, Cycle_t >
                                                ( this, &XbarV2::clock );

    if ( ! clockHandler ) {
        _abort(XbarV2,"couldn't create clock handler");
    }

    std::string             frequency;
    if ( params.find("clock") != params.end() ) {
        frequency = params["clock"];
    }

    m_log.write("numPorts=%d frequency=%s\n", m_numPorts, frequency.c_str());

    TimeConverter* tc = registerClock( frequency, clockHandler );
    if ( ! tc ) {
        _abort(XbarV2,"couldn't register clock handler");
    }

    DBG("Done registering clock\n",id);
}

XbarV2::Port::Port( Component& comp, memMap_t& memMap, entryV_t& entryV,
            int numPorts, int portNum, addr_t addr, length_t length,
            bool enableDbg ) :
    m_numPorts( numPorts ),
    m_portNum( portNum ),
    m_memMap( memMap ),
    m_curEvent( NULL ),
    m_entryV( entryV ),
    m_dbg( *new Log< XBARV2_DBG >( "XbarV2::Port::", enableDbg ) )
{
    std::ostringstream name;
    name << "port" << portNum;

    DBG("port=%d name=%s addr=%#lx length=%#lx\n",
                            portNum, name.str().c_str(), addr, length );

    Params_t params;
    m_memChan = new memChan_t( comp, params, name.str() );
}

void XbarV2::Port::doInput( Cycle_t cycle )
{
    //DBG( "port=%d cycle=%lu\n", m_portNum, cycle );
    for ( int dstPort = 0; dstPort < m_numPorts ; ++dstPort )
    {
        // if we don't have an event on deck try and get one
        if ( m_curEvent == NULL && 
                ! m_memChan->recv( &m_curEvent, &m_curCookie ) )
        {
            return;
        } 

        Entry& entry = m_entryV[m_portNum][dstPort];

        // if dst port doesn't have a pending event
        if ( entry.event == NULL )
        {
            if ( m_curEvent->msgType == event_t::REQUEST ) {
                Port* tmp = findDstPort( m_curEvent->addr );
                if ( tmp && tmp->m_portNum == dstPort ) { 
                    m_curCookie = this;
                } else {
                    // bad programmer 
                    continue;
                }
            } else {
                m_curCookie = NULL;
            }
            DBG("srcPort=%d dstPort=%d addr=%lx cookie=%p\n", 
                        m_portNum, dstPort, m_curEvent->addr, m_curCookie );

            entry.event     = m_curEvent;
            entry.timeStamp = cycle;
            entry.cookie    = m_curCookie;

            m_curEvent = NULL;
        } 
    }
}

void XbarV2::Port::doOutput( )
{
    static Entry* entry = NULL;

    if ( entry == NULL ) {
        event_t* event = NULL;
        cookie_t cookie = NULL;
        Cycle_t oldest = (Cycle_t)-1;
        for ( int srcPort = 0; srcPort < m_numPorts; srcPort++ )
        {
            if ( m_entryV[ srcPort ][m_portNum].event && 
                m_entryV[ srcPort ][m_portNum].timeStamp < oldest ) 
            {
                entry = &m_entryV[ srcPort ][m_portNum];
                oldest = m_entryV[ srcPort ][m_portNum].timeStamp;
                DBG("srcPort=%d dstPort=%d oldest=%lx\n", 
                                        srcPort, m_portNum, oldest );
            }
        }
    }

    if ( entry )  
    {
        DBG("port=%d sending cookie=%p \n", m_portNum, entry->cookie );
        if ( m_memChan->send( entry->event,  entry->cookie ) ) {
            entry->event = NULL;
            entry = NULL;
        }
    }
}

XbarV2::Port::Port* XbarV2::Port::findDstPort( addr_t addr )
{
    Port** ptr = m_memMap.find( addr);
    if ( ptr ) return *ptr;
    return NULL;
}

void XbarV2::initPort( int portNum, Params_t&  params )
{
    Params_t portParams;
    std::ostringstream name;
    addr_t addr = 0;
    length_t length = 0;
    bool enableDbg = false;

    name << "port" << portNum;

    DBG("%s\n",name.str().c_str());

    if ( params.find( "debugPort" ) != params.end() ) {
        if ( params[ "debugPort" ].compare( "yes" ) == 0 ) {
            enableDbg = true;
        }
    }

    findParams( name.str() + ".", params, portParams );

    if ( portParams.find( "address" ) != portParams.end()) {
        addr = str2long( portParams["address"] ); 
    } 

    if ( portParams.find( "length" ) != portParams.end()) {
        length = str2long( portParams["length"] ); 
    }

    m_log.write("name=%s addr=%#lx length=%#lx\n",
                                    name.str().c_str(), addr, length );

    m_portInfoV[ portNum ] = new Port( *this, m_memMap, m_entryV,
                           m_numPorts, portNum, addr, length, enableDbg );

    if ( length ) {
        if ( m_memMap.insert( addr, length, m_portInfoV[ portNum ] ) ) {
            _abort(XbarV2,"couldn't init port, bad region?, addr=%#lx %#lx\n",
                               (unsigned long) addr, (unsigned long) length);
        }
    }
}

bool XbarV2::clock( Cycle_t current )
{
    //DBG( "id=%lu currentCycle=%lu\n", Id(), current );

    for ( int srcPort = 0; srcPort < m_numPorts; srcPort++ )
    {
        m_portInfoV[ srcPort ]->doInput( current );
    }

    for ( int dstPort = 0; dstPort < m_numPorts; dstPort++ )
    {
        m_portInfoV[ dstPort ]->doOutput();
    }
    return false;
}
