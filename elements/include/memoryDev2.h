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


#define _MD_DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

MEMORYDEV()::MemoryDev( Component& comp,
                Component::Params_t params, std::string name )  :
    dev_t( comp, params, name ),
    m_dbg( *new Log< MEMORYDEV_DBG >( "MemoryDev::", false ) )
{
    if ( params.find("debug") != params.end() ) {
        if ( params["debug"].compare("yes") == 0 ) {
            m_dbg.enable();
        }
    }
    _MD_DBG("\n");
}


MEMORYDEV( inline bool )::read( addr_t addr, cookie_t cookie = NULL )
{
    _MD_DBG("\n");
    return send( addr, NULL, cookie, event_t::READ );
}

MEMORYDEV( inline bool )::write( addr_t addr, cookie_t cookie = NULL )
{
    _MD_DBG("\n");
    return send( addr, NULL, cookie, event_t::WRITE );
}

MEMORYDEV( inline bool )::read( addr_t addr, data_t* data, 
                                cookie_t cookie = NULL )
{
    _MD_DBG("\n");
    return send( addr, data, cookie, event_t::READ );
}

MEMORYDEV( inline bool ) ::write( addr_t addr, data_t* data,
                                cookie_t cookie = NULL )
{
    _MD_DBG("\n");
    return send( addr, data, cookie, event_t::WRITE );
}

MEMORYDEV( inline bool )::send( addr_t addr, data_t* data, 
                    cookie_t cookie, typename event_t::reqType_t type )
{
    _MD_DBG("addr=%#lx cookie=%#lx type=%d \n", addr, cookie, type );
    if ( ! dev_t::ready( event_t::REQUEST ) ) {
        return false;
    }
    event_t* event = new event_t;
    event->addr = addr;
    event->reqType = type; 
    event->msgType = event_t::REQUEST; 
    dev_t::send( event, new foo_t( cookie, data ) );
    return true; 
}


MEMORYDEV( inline bool )::popCookie( cookieT& cookie )
{
    foo_t*  foo;
    bool ret;
    event_t* event;

    if ( ( ret = dev_t::recv( &event, &foo ) ) ) {
        cookie = foo->first;
        _MD_DBG("cookie=%#lx data*=%p\n", cookie, foo->second );
        if ( foo->second ) {
            //*foo->second = event->data;
        }
        delete foo;
        delete event;
    }

    return ret;
}
