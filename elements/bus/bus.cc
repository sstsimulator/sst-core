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

#include "bus.h"
#include <paramUtil.h>

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

Bus::Bus( ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_readBusy( false ),
    m_writeBusy( false ),
    m_dbg( *new Log< BUS_DBG >( "Bus::", false ) ),
    m_log( *new Log<>( "INFO Bus: ", false ) )
{
    m_atBat.valid =  false;
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
    //paramsPrint( params );

    DBG("new id=%lu\n",id);

    initDevices( params );

    std::string             frequency;
    if ( params.find("clock") != params.end() ) {
        frequency = params["clock"];
    }

    m_log.write("frequency=%s\n", frequency.c_str() );

    ClockHandler_t* clockHandler = new EventHandler< Bus, bool, Cycle_t >
                                                ( this, &Bus::clock );

    if ( ! clockHandler ) {
        _abort(Bus,"couldn't create clock handler");
    }

    TimeConverter* tc = registerClock( frequency, clockHandler );
    if ( ! tc ) {
        _abort(Bus,"couldn't register clock handler");
    }
}

void Bus::initDevices( Params_t&  params )
{
    std::string devList = "";
    if ( params.find( "deviceList" ) != params.end() ) {
        devList = params[ "deviceList" ].c_str();
    }
    m_log.write("deviceList \"%s\"\n", devList.c_str() );
    
    while (1) {
        size_t pos = devList.find_first_not_of( ' ' );
        devList =  devList.substr( pos );

        pos = devList.find_first_of( ' ' );

        std::string name = devList.substr( 0, pos);
        initDevice( name , params );
        if ( pos == std::string::npos ) {
            break;
        }
        devList =  devList.substr( pos );
    }
}

void Bus::initDevice( std::string name, Params_t&  params )
{
    Params_t portParams;
    addr_t addr = 0;
    length_t length = 0;

    DBG("%s\n",name.c_str());

    findParams( name + ".", params, portParams );

    if ( portParams.find( "address" ) != portParams.end()) {
        addr = str2long( portParams["address"] ); 
    } 

    if ( portParams.find( "length" ) != portParams.end()) {
        length = str2long( portParams["length"] ); 
    }

    m_log.write("create Device \"%s\" addr=%#lx length=%#lx\n",
                                    name.c_str(), addr, length );
    
    Device* device = new Device( *this, name, portParams );

    if ( m_memMap.insert( addr, length, device ) ) {
        _abort(Bus,"couldn't init device, bad region?, addr=%#lx %#lx\n",
                               (unsigned long) addr, (unsigned long) length);
    }
}

bool Bus::doRead( event_t* event, Device* src, Device* dst )
{
    if ( ! m_readBusy && dst->send( event, src ) ) {
        DBG("\n");
        m_readBusy = true;
        return true;
    }
    return false;
}

bool Bus::doWrite( event_t* event, Device* src, Device* dst )
{
    if ( ! m_writeBusy && dst->send( event, src ) ) {
        DBG("\n");
        m_writeBusy = true;
        return true;
    }
    return false;
}

bool Bus::clock( Cycle_t current )
{
    Cycle_t oldest = (Cycle_t)-1;
    memMap_t::iterator winner = m_memMap.end();
    
    // for each device:
    //  1) recv request messages 
    //  2) arbitrate who's going next
    //  3) process response message
    for ( memMap_t::iterator iter = m_memMap.begin();
                                iter != m_memMap.end(); iter++ )
    {
        (*iter)->clock( current );

        Cycle_t tmp = (*iter)->timeStamp( );
        if ( tmp < oldest ) {
            oldest = tmp;
            winner = iter;
        }
        switch( (*iter)->doResp() ) {
            case event_t::READ:
                DBG("read Ready %d\n", m_writeBusy);
                m_readBusy = false;
                break;
            case event_t::WRITE:
                DBG("write Ready %d\n", m_readBusy);
                m_writeBusy = false;
                break;
        }
    }

    if ( winner != m_memMap.end() && ! m_atBat.valid ) {
        DBG("set at bat %lu\n",current);
        m_atBat.event = (*winner)->getReq( );
        Device** dev  = m_memMap.find( m_atBat.event->addr );
        if ( dev == NULL ) {
            _abort( Bus, "m_memMap.find( %#lx ) failed\n", 
                                    (unsigned long) m_atBat.event->addr );
        }
        m_atBat.valid          = true;
        m_atBat.dstDev         = *dev;
        m_atBat.srcDev         = *winner;
    }

    if ( m_atBat.valid ) {
        if ( m_atBat.event->reqType == event_t::READ ) {
            if ( doRead( m_atBat.event, m_atBat.srcDev, m_atBat.dstDev ) ) {
               m_atBat.valid = false; 
            }
        }  else {
            if ( doWrite( m_atBat.event, m_atBat.srcDev, m_atBat.dstDev ) ) {
               m_atBat.valid = false; 
            }
        }
    }

    return false;
}

Bus::Device::Device( Component& comp, std::string name, Params_t params ) :
    m_dbg( *new Log< BUS_DBG >( "Bus::Device::" ) )
{
    //paramsPrint( const_cast<Component::Params_t&>(params) );

    if ( params.find( "debug" ) != params.end() ) {
        if ( params[ "debug" ].compare( "yes" ) == 0 ) {
            m_dbg.enable();
        }
    }

    m_devChan = new devChan_t( comp, params, name );
    m_req.second = (Cycle_t)-1;
    m_name = name;
}

void Bus::Device::clock( Cycle_t cycle )
{
    if ( m_req.second != (Cycle_t)-1 ) return;

    if ( m_devChan->recv( &m_req.first, event_t::REQUEST ) ) {
        DBG("%s got new request cycle=%lu\n", m_name.c_str(), cycle);
        m_req.second = cycle;
    }
}

bool Bus::Device::send( event_t* event, cookie_t cookie = 0  )
{
    DBG( "%s cookie=%#lx\n", m_name.c_str(), cookie );
    return m_devChan->send( event, cookie );
}

Cycle_t Bus::Device::timeStamp( )
{
    return m_req.second;
}

Bus::event_t* Bus::Device::getReq( )
{
    m_req.second = (Cycle_t)-1;
    return m_req.first;
}

Bus::event_t::reqType_t Bus::Device::doResp( )
{
    Bus::event_t::reqType_t retVal = event_t::INV_REQ;
    Device* srcDev;
    event_t* event;
    if ( m_devChan->recv( &event, event_t::RESPONSE, &srcDev ) ) {
        retVal = event->reqType;
        DBG("%s returning RESPONSE %d\n",m_name.c_str(),retVal);
        if ( ! srcDev->send( event ) ) {
            _abort(Bus::Device::doResp,"send failed\n");
        }
    }
    return retVal;
}
